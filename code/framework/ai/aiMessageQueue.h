#pragma once

#include "..\..\core\concurrency\spinLock.h"
#include "..\..\core\types\name.h"

namespace Framework
{
	interface_class IAIObject;
	class Room;
	class SubWorld;

	namespace AI
	{
		class Message;

		struct MessageQueue
		{
		public:
			MessageQueue();
			~MessageQueue();
			
			bool is_empty() const { return first == nullptr; }

			void clear();
			void move_to(MessageQueue & _queue);

			void invalidate_to(Room* _room);
			void invalidate_to(IAIObject* _object);
			void invalidate_to(SubWorld* _subWorld);

			void distribute_to_process(float _deltaTime, MessageQueue& _distributed); // distribute to recipients (and move to another queue all distributed)

			Message* create_new(Name const & _name); // this is only one here thread safe - other ones should not require being thread safe

		private:
			Concurrency::SpinLock createLock = Concurrency::SpinLock(TXT("Framework.AI.MessageQueue.createLock"));
			Message* first;
			Message* last;
		};
	};
};
