#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\fastCast.h"

namespace Framework
{
	namespace AI
	{
		class Message;
		interface_class IMessageHandler;

		typedef void(*MessageHandlerFunc)(IMessageHandler* _messageHandler, Message const & _message);

		interface_class IMessageHandler
		{
			FAST_CAST_DECLARE(IMessageHandler);
			FAST_CAST_END();
		public:
			IMessageHandler();
			virtual ~IMessageHandler() {}

			virtual void handle_message(Message const & _message) {}
		};
	};
};
