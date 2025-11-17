#pragma once

#include "aiIMessageHandler.h"

#include "..\..\core\containers\arrayStatic.h"
#include "..\..\core\types\name.h"

#include <functional>

namespace Framework
{
	namespace AI
	{
		class MindInstance;

		typedef std::function<void(Message const & _message)> HandleMessage;

		struct MessageHandler
		: public IMessageHandler
		{
			FAST_CAST_DECLARE(MessageHandler);
			FAST_CAST_BASE(IMessageHandler)
			FAST_CAST_END();
		public:
			static MessageHandler const & empty() { return s_empty; }

			MessageHandler();
			MessageHandler(MessageHandler const & _messageHandler);
			MessageHandler& operator =(MessageHandler const & _messageHandler);
			virtual ~MessageHandler();

			void use_with(MindInstance* _mind); // registers/unregisters at mind

			void set(Name const & _messageName, HandleMessage _handle);
			void set(HandleMessage _handle); // universal, if not handled with name

		public:
			implement_ void handle_message(Message const & _message);

		private:
			static MessageHandler s_empty;
			MindInstance* mind = nullptr;

			struct RegisteredHandleMessage
			{
				Name messageName;
				HandleMessage func = nullptr;
			};
			ArrayStatic<RegisteredHandleMessage, 16> funcs;
			RegisteredHandleMessage generalFunc;
		};
	};
};

DECLARE_REGISTERED_TYPE(Framework::AI::MessageHandler);
