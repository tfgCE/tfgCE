#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\containers\array.h"
#include "..\..\core\containers\map.h"

#include "aiHunch.h"
#include "aiMessage.h"
#include "aiIMessageHandler.h"

namespace Framework
{
	namespace AI
	{
		class ITask;
		class MindInstance;
		struct MindInstanceContext;
		interface_class IMessageHandler;

		/**
		 *	Responsible for handling tasks and messages.
		 */
		class MindExecution
		{
		public:
			MindExecution(MindInstance* _mind);
			~MindExecution();

			void reset();

			void advance(float _deltaTime);

			void start_task(ITask* _task);
			void stop_task(ITask* _task); // only to end when task has not yet ended

			bool does_handle_message(Name const& _messageName) const;
			MessagesToProcess & access_messages_to_process() { return messagesToProcess; }
			Hunches & access_hunches() { return hunches; }

			bool does_require_process_messages() const { return ! messagesToProcess.is_empty(); }
			void process_messages();
			
			void register_message_handler(Name const & _messageName, IMessageHandler* _messageHandler, MessageHandlerFunc _messageHandlingFunc = nullptr);
			void register_message_handler(IMessageHandler* _messageHandler, MessageHandlerFunc _messageHandlingFunc = nullptr);
			
			void unregister_message_handler(Name const & _messageName, IMessageHandler* _messageHandler, MessageHandlerFunc _messageHandlingFunc = nullptr);
			void unregister_message_handler(IMessageHandler* _messageHandler, MessageHandlerFunc _messageHandlingFunc = nullptr);
			void unregister_all_message_handlers(IMessageHandler* _messageHandler);

			bool has_hunch(Name const& _name, bool _consume = false);

		private:
			MindInstance* mind;

			Array<ITask*> tasks;

			MessagesToProcess messagesToProcess;
			Hunches hunches;
			
			Optional<float> rareAdvanceWait;

		private:
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
		public:
#endif
			struct MessageHandler
			{
				// handler or function
				IMessageHandler* messageHandler;
				MessageHandlerFunc func;
				MessageHandler();
				MessageHandler(IMessageHandler* _messageHandler, MessageHandlerFunc _func);
				bool operator ==(MessageHandler const & _a) const { return messageHandler == _a.messageHandler && func == _a.func; }
			};
			// public to allow debugging, spin lock stuff
			struct MessageHandlers
			: public SoftPooledObject<MessageHandlers>
			{
				Array<MessageHandler> messageHandlers;

				void handle_message(Message const & _message) const;
				void register_message_handler(IMessageHandler* _messageHandler, MessageHandlerFunc _messageHandlingFunc = nullptr);
				void unregister_message_handler(IMessageHandler* _messageHandler, MessageHandlerFunc _messageHandlingFunc = nullptr);
				void unregister_message_handlers(IMessageHandler* _messageHandler);

				override_ void on_release() { messageHandlers.clear(); }
			};

		private:
			Map<Name, MessageHandlers*> messageHandlersByName; // none name is for "all"
		};
	};
};

#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
TYPE_AS_CHAR(Framework::AI::MindExecution::MessageHandlers);
#endif