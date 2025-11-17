#include "aiMindExecution.h"

#include "aiMindInstance.h"
#include "aiITask.h"

#include "..\module\moduleAI.h"
#include "..\modulesOwner\modulesOwner.h"

#include "..\world\world.h"

#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AI;

//

MindExecution::MessageHandler::MessageHandler()
{
}

MindExecution::MessageHandler::MessageHandler(IMessageHandler* _messageHandler, MessageHandlerFunc _func)
: messageHandler(_messageHandler)
, func(_func)
{
}

//

void MindExecution::MessageHandlers::handle_message(Message const & _message) const
{
	for_every(messageHandler, messageHandlers)
	{
		if (messageHandler->func)
		{
			messageHandler->func(messageHandler->messageHandler, _message);
		}
		else
		{
			messageHandler->messageHandler->handle_message(_message);
		}
	}
}

void MindExecution::MessageHandlers::register_message_handler(IMessageHandler* _messageHandler, MessageHandlerFunc _messageHandlingFunc)
{
	an_assert(!messageHandlers.does_contain(MessageHandler(_messageHandler, _messageHandlingFunc)), TXT("should appear just once!"));
	messageHandlers.push_back(MessageHandler(_messageHandler, _messageHandlingFunc));
}

void MindExecution::MessageHandlers::unregister_message_handler(IMessageHandler* _messageHandler, MessageHandlerFunc _messageHandlingFunc)
{
	int idx = 0;
	for_every(messageHandler, messageHandlers)
	{
		if (messageHandler->messageHandler == _messageHandler &&
			messageHandler->func == _messageHandlingFunc)
		{
			messageHandlers.remove_at(idx);
			break;
		}
		++idx;
	}
}

void MindExecution::MessageHandlers::unregister_message_handlers(IMessageHandler* _messageHandler)
{
	for(int idx = 0; idx < messageHandlers.get_size(); ++ idx)
	{
		if (messageHandlers[idx].messageHandler == _messageHandler)
		{
			messageHandlers.remove_at(idx);
			--idx;
			break;
		}
	}
}

//

MindExecution::MindExecution(MindInstance* _mind)
: mind(_mind)
{
	// we will always have some messages to process
	messagesToProcess.make_space_for(8);
}

MindExecution::~MindExecution()
{
	reset();
}

void MindExecution::reset()
{
	for_every_ptr(task, tasks)
	{
		task->release_task();
	}
	tasks.clear();
	for_every_ptr(messageHandler, messageHandlersByName)
	{
		messageHandler->release();
	}
	messageHandlersByName.clear();
}

void MindExecution::advance(float _deltaTime)
{
	scoped_call_stack_info_str_printf(TXT("MindExecution::advance o%p"), mind->get_owner_as_modules_owner());

	MEASURE_PERFORMANCE(mindExecution_advance);

	if (!hunches.is_empty())
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
			if (auto* w = imo->get_in_world())
			{
				float worldTime = w->get_world_time();
				for (int i = 0; i < hunches.get_size(); ++ i)
				{
					auto* hunch = hunches[i].get();
					hunch->update_world_time(worldTime);
					if (!hunch->is_valid() ||
						hunch->is_consumed())
					{
						hunches.remove_at(i);
						--i;
					}
				}
			}
		}
	}

	struct TaskID
	{
		ITask* task;
		int id;
		TaskID() {}
		TaskID(ITask* _task, int _id) : task(_task), id(_id) {}
	};
	// copy array as we might be removing or adding tasks and we don't want to miss anything
	ARRAY_STACK(TaskID, tasksCopy, tasks.get_size());
	for_every_ptr(task, tasks)
	{
		tasksCopy.push_back(TaskID(task, task->get_id()));
	}

	PERFORMANCE_GUARD_LIMIT(0.005f, String::printf(TXT("MindExecution::advance %S (tasks:%i)"), mind->get_owner_as_modules_owner()->ai_get_name().to_char(), tasks.get_size()).to_char());

	{
		scoped_call_stack_info(mind->get_owner_as_modules_owner()->ai_get_name().to_char());

		// advance but only if it is still in tasks array and have a matching id - this could be different if we were reusing tasks (tasks here could be copying/removing them)
		for_every(task, tasksCopy)
		{
			if (task->task &&
				tasks.does_contain(task->task) &&
				task->task->get_id() == task->id)
			{
				MEASURE_PERFORMANCE(mindExecution_task);
				if (task->task->advance(_deltaTime))
				{
					task->task->release_task();
					tasks.remove(task->task);
				}
			}
		}

		rareAdvanceWait.clear();
		for_every_ptr(task, tasks)
		{
			if (task->get_rare_advance_wait().is_set())
			{
				float taskWait = task->get_rare_advance_wait().get();
				rareAdvanceWait = min(taskWait, rareAdvanceWait.get(taskWait));
			}
			else
			{
				// if at least one doesn't want to wait, quit
				rareAdvanceWait.clear();
				break;
			}
		}

		// if we won't clear, we will end up with infinite rare advance as if we wait 2 seconds, we will get back here to wait for 2 more seconds and so on
		for_every_ptr(task, tasks)
		{
			task->clear_rare_advance_wait();
		}

		if (auto* ai = mind->get_owner_as_modules_owner()->get_ai())
		{
			if (rareAdvanceWait.is_set())
			{
				ai->access_rare_logic_advance().set_long_wait(rareAdvanceWait.get());
			}
			else
			{
				ai->access_rare_logic_advance().stop_long_wait();
			}
		}
	}
}

void MindExecution::start_task(ITask* _task)
{
	tasks.push_back(_task);
	todo_note(TXT("should call \"on start task\"?"));
}

void MindExecution::stop_task(ITask* _task)
{
	if (_task)
	{
		if (mind && ! mind->is_ending_logic())
		{
			_task->on_break();
		}
		_task->release_task();
		tasks.remove(_task);
	}
}

bool MindExecution::does_handle_message(Name const& _messageName) const
{
	if (MessageHandlers* const * pMessageHandlers = messageHandlersByName.get_existing(_messageName))
	{
		if (!(*pMessageHandlers)->messageHandlers.is_empty())
		{
			return true;
		}
	}
	if (MessageHandlers* const * pMessageHandlers = messageHandlersByName.get_existing(Name::invalid()))
	{
		if (!(*pMessageHandlers)->messageHandlers.is_empty())
		{
			return true;
		}
	}

	return false;
}

void MindExecution::process_messages()
{
	bool resumeAdvancement = false;
	for_every_ptr(message, messagesToProcess)
	{
		if (MessageHandlers** pMessageHandlers = messageHandlersByName.get_existing(message->get_name()))
		{
			(*pMessageHandlers)->handle_message(*message);
			resumeAdvancement = true;
		}
		if (MessageHandlers** pMessageHandlers = messageHandlersByName.get_existing(Name::invalid()))
		{
			(*pMessageHandlers)->handle_message(*message);
			resumeAdvancement = true;
		}
	}
	messagesToProcess.clear();
	if (resumeAdvancement)
	{
		mind->get_owner_as_modules_owner()->resume_advancement();
	}
}

void MindExecution::register_message_handler(Name const & _messageName, IMessageHandler* _messageHandler, MessageHandlerFunc _messageHandlingFunc)
{
	if (!messageHandlersByName.has_key(_messageName))
	{
		messageHandlersByName[_messageName] = MessageHandlers::get_one();
	}
	messageHandlersByName[_messageName]->register_message_handler(_messageHandler, _messageHandlingFunc);
}

void MindExecution::register_message_handler(IMessageHandler* _messageHandler, MessageHandlerFunc _messageHandlingFunc)
{
	register_message_handler(Name::invalid(), _messageHandler, _messageHandlingFunc);
}

void MindExecution::unregister_message_handler(Name const & _messageName, IMessageHandler* _messageHandler, MessageHandlerFunc _messageHandlingFunc)
{
	if (MessageHandlers** pMessageHandlers = messageHandlersByName.get_existing(_messageName))
	{
		(*pMessageHandlers)->unregister_message_handler(_messageHandler, _messageHandlingFunc);
	}
}

void MindExecution::unregister_all_message_handlers(IMessageHandler* _messageHandler)
{
	for_every_ptr(mh, messageHandlersByName)
	{
		mh->unregister_message_handlers(_messageHandler);
	}
}

void MindExecution::unregister_message_handler(IMessageHandler* _messageHandler, MessageHandlerFunc _messageHandlingFunc)
{
	unregister_message_handler(Name::invalid(), _messageHandler, _messageHandlingFunc);
}

bool MindExecution::has_hunch(Name const& _name, bool _consume)
{
	for_every_ref(hunch, hunches)
	{
		if (hunch->get_name() == _name)
		{
			if (_consume)
			{
				hunch->consume();
			}
			return true;
		}
	}
	return false;
}

