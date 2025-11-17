#include "aiMessageHandler.h"

#include "aiMindInstance.h"

using namespace Framework;
using namespace AI;

REGISTER_FOR_FAST_CAST(MessageHandler);

MessageHandler MessageHandler::s_empty;

MessageHandler::MessageHandler()
{
	SET_EXTRA_DEBUG_INFO(funcs, TXT("AI::MessageHandler.funcs"));
}

MessageHandler::MessageHandler(MessageHandler const & _messageHandler)
: funcs(_messageHandler.funcs)
, generalFunc(_messageHandler.generalFunc)
{
	SET_EXTRA_DEBUG_INFO(funcs, TXT("AI::MessageHandler.funcs"));
	use_with(_messageHandler.mind);
}

MessageHandler& MessageHandler::operator = (MessageHandler const & _messageHandler)
{
	funcs = _messageHandler.funcs;
	generalFunc = _messageHandler.generalFunc;
	use_with(_messageHandler.mind);
	return *this;
}

MessageHandler::~MessageHandler()
{
	use_with(nullptr);
}

void MessageHandler::use_with(MindInstance* _mind)
{
	if (mind == _mind)
	{
		return;
	}
	if (mind)
	{
		mind->access_execution().unregister_all_message_handlers(this);
	}
	mind = _mind;
	if (mind)
	{
		// register with new mind
		if (generalFunc.func)
		{
			mind->access_execution().register_message_handler(this);
		}
		for_every(func, funcs)
		{
			mind->access_execution().register_message_handler(func->messageName, this);
		}
	}
}

void MessageHandler::set(Name const & _messageName, HandleMessage _handle)
{
	an_assert(mind);
	for_every(func, funcs)
	{
		if (func->messageName == _messageName)
		{
			if (_handle)
			{
				func->func = _handle;
			}
			else
			{
				if (mind)
				{
					mind->access_execution().unregister_message_handler(_messageName, this);
				}
				funcs.remove_at(for_everys_index(func));
			}
			return;
		}
	}
	if (_handle)
	{
		RegisteredHandleMessage func;
		func.messageName = _messageName;
		func.func = _handle;
		funcs.push_back(func);
		if (mind)
		{
			mind->access_execution().register_message_handler(_messageName, this);
		}
	}
}

void MessageHandler::set(HandleMessage _handle)
{
	an_assert(mind);
	generalFunc.func = _handle;
	if (mind)
	{
		mind->access_execution().register_message_handler(this);
	}
}

void MessageHandler::handle_message(Message const & _message)
{
	for_every(func, funcs)
	{
		if (func->messageName == _message.get_name())
		{
			func->func(_message);
			return;
		}
	}
	if (generalFunc.func)
	{
		generalFunc.func(_message);
	}
}

