#include "aiMessageQueue.h"

#include "aiMessage.h"

#include "..\..\core\concurrency\scopedSpinLock.h"

using namespace Framework;
using namespace AI;

MessageQueue::MessageQueue()
: first(nullptr)
, last(nullptr)
{
}

MessageQueue::~MessageQueue()
{
	clear();
}

Message* MessageQueue::create_new(Name const & _name)
{
	Message* newMessage = Message::get_one();
	newMessage->set_queue_origin(this);
	newMessage->name_message(_name);
	{
		Concurrency::ScopedSpinLock scopedCreateLock(createLock);
		if (last)
		{
			last->next = newMessage;
		}
		else
		{
			first = newMessage;
		}
		last = newMessage;
	}
	return newMessage;
}

void MessageQueue::clear()
{
	Message* message = first;
	while (message)
	{
		Message* next = message->next;
		message->next = nullptr;
		message->release();
		message = next;
	}
	first = nullptr;
	last = nullptr;
}

void MessageQueue::move_to(MessageQueue & _queue)
{
	if (! last)
	{
		return;
	}
	if (_queue.last)
	{
		_queue.last->next = first;
	}
	else
	{
		_queue.first = first;
	}
	_queue.last = last;
	first = nullptr;
	last = nullptr;
}

void MessageQueue::distribute_to_process(float _deltaTime, MessageQueue & _distributed)
{
	Message* message = first;
	Message* prev = nullptr;
	while (message)
	{
		Message* next = message->next;
		if (message->distribute_to_process(_deltaTime))
		{
			message->next = nullptr;
			if (_distributed.first)
			{
				// reverse order
				message->next = _distributed.first;
				_distributed.first = message;
			}
			else
			{
				_distributed.first = message;
				_distributed.last = message;
			}
			// remove from current queue
			if (prev)
			{
				prev->next = next;
			}
			else
			{
				first = next;
			}
		}
		else
		{
			prev = message;
		}
		message = next;
	}
	last = prev;
}

void MessageQueue::invalidate_to(Room* _room)
{
	Message* message = first;
	while (message)
	{
		if (message->messageTo == MessageTo::Room && message->messageToRoom == _room)
		{
			message->to_no_one();
		}
		message = message->next;
	}
}

void MessageQueue::invalidate_to(IAIObject* _object)
{
	Message* message = first;
	while (message)
	{
		if (message->messageTo == MessageTo::AIObject && message->messageToAIObject == _object)
		{
			message->to_no_one();
		}
		message = message->next;
	}
}

void MessageQueue::invalidate_to(SubWorld* _subWorld)
{
	Message* message = first;
	while (message)
	{
		if (message->messageTo == MessageTo::SubWorld && message->messageToSubWorld == _subWorld)
		{
			message->to_no_one();
		}
		message = message->next;
	}
}


