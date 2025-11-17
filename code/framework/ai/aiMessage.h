#pragma once

#include "..\..\core\memory\softPooledObject.h"
#include "..\..\core\types\name.h"
#include "..\..\core\math\math.h"
#include "..\..\core\casts.h"
#include "..\interfaces\aiObject.h"
#include "aiParam.h"

class SimpleVariableStorage;

namespace Framework
{
	class Room;
	class World;
	class SubWorld;
	interface_class IAIObject;

	namespace AI
	{
		struct MessageQueue;
		class Message;
		
		namespace MessageTo
		{
			enum Type
			{
				Undefined,
				World,
				SubWorld,
				Room,
				AIObject,
				InRange, // in room and adjacent rooms (one depth only!) in certain range
			};
		};

		class Message
		: public SoftPooledObject<Message>
		{
			typedef SoftPooledObject<Message> base;
		public:
			Message();
			virtual ~Message() {}

			Message * create_response(Name const & _name) const;

			Name const & get_name() const { return name; }
			Param & access_param(Name const & _name); // creates if doesn't exist
			Param const * get_param(Name const & _name) const;
			
			void set_params_from(SimpleVariableStorage const& _store);

			void name_message(Name const & _name) { name = _name; }

			void delayed(float _delay) { delay = _delay; }

			void to_no_one();
			void to_world(World* _world);
			void to_sub_world(SubWorld* _subWorld);
			void to_ai_object(IAIObject* _object);
			void to_room(Room* _room);
			void to_all_in_range(Room* _room, Vector3 const & _location, float _inRange);

		protected: friend class SoftPooledObject<Message>;
			override_ void on_get();
			override_ void on_release();

		private:
			AI::MessageQueue* originatedInQueue = nullptr;
			float delay = 0.0;
			Message* next;
			Name name;
			MessageTo::Type messageTo;
			// it should be safe to use pointers as message lifetime is one frame
			World* messageToWorld;
			SubWorld* messageToSubWorld;
			Room* messageToRoom;
			IAIObject* messageToAIObject;
			Vector3 messageToLocation;
			float messageToRange;

			Params params;

			void set_queue_origin(AI::MessageQueue* _originatedInQueue) { originatedInQueue = _originatedInQueue; }

			bool distribute_to_process(float _deltaTime); // returns true if distributed

			friend struct MessageQueue;
		};
	};
};

TYPE_AS_CHAR(Framework::AI::Message);