#include "aiMessage.h"

#include "..\interfaces\aiObject.h"

#include "..\module\modulePresence.h"

#include "..\world\world.h"

#include "..\..\core\other\simpleVariableStorage.h"

//

using namespace Framework;
using namespace AI;

//

Message::Message()
: next(nullptr)
, messageTo(MessageTo::Undefined)
{
}

Message * Message::create_response(Name const & _name) const
{
	an_assert(originatedInQueue);
	return originatedInQueue->create_new(_name);
}

void Message::to_no_one()
{
	messageTo = MessageTo::Undefined;
}

void Message::to_world(World* _world)
{
	messageTo = MessageTo::World;
	messageToWorld = _world;
}

void Message::to_sub_world(SubWorld* _subWorld)
{
	messageTo = MessageTo::SubWorld;
	messageToSubWorld = _subWorld;
}

void Message::to_ai_object(IAIObject* _object)
{
	messageTo = MessageTo::AIObject;
	messageToAIObject = _object;
}

void Message::to_room(Room* _room)
{
	messageTo = MessageTo::Room;
	messageToRoom = _room;
}

void Message::to_all_in_range(Room* _room, Vector3 const & _location, float _inRange)
{
	messageTo = MessageTo::InRange;
	messageToRoom = _room;
	messageToLocation = _location;
	messageToRange = _inRange;
}

void Message::on_get()
{
	an_assert(next == nullptr);
	base::on_get();
	name = Name::invalid();
}

void Message::on_release()
{
	an_assert(next == nullptr);
	base::on_release();
	params.clear();
	to_no_one();
}

Param & Message::access_param(Name const & _name)
{
	return params.access(_name);
}

Param const * Message::get_param(Name const & _name) const
{
	return params.get(_name);
}

void Message::set_params_from(SimpleVariableStorage const& _store)
{
	for_every(svi, _store.get_all())
	{
		auto& param = access_param(svi->get_name());
		param.set(*svi);
	}
}

bool Message::distribute_to_process(float _deltaTime)
{
	if (UNLIKELY delay > 0.0f)
	{
		delay -= _deltaTime;
		return false;
	}

	switch (messageTo)
	{
	case MessageTo::AIObject:
		if (messageToAIObject && messageToAIObject->does_handle_ai_message(get_name()))
		{
			if (MessagesToProcess* messagesToProcess = messageToAIObject->access_ai_messages_to_process())
			{
				messagesToProcess->push_back(this);
			}
		}
		break;
	case MessageTo::World:
		if (messageToWorld)
		{
			for_every_ptr(object, messageToWorld->get_objects())
			{
				if (object->does_handle_ai_message(get_name()))
				{
					if (MessagesToProcess* messagesToProcess = object->access_ai_messages_to_process())
					{
						messagesToProcess->push_back(this);
					}
				}
			}
		}
		break;
	case MessageTo::SubWorld:
		if (messageToSubWorld)
		{
			for_every_ptr(object, messageToSubWorld->get_objects())
			{
				if (object->does_handle_ai_message(get_name()))
				{
					if (MessagesToProcess* messagesToProcess = object->access_ai_messages_to_process())
					{
						messagesToProcess->push_back(this);
					}
				}
			}
		}
		break;
	case MessageTo::Room:
		if (messageToRoom)
		{
			for_every_ptr(object, messageToRoom->get_objects())
			{
				if (object->does_handle_ai_message(get_name()))
				{
					if (MessagesToProcess* messagesToProcess = object->access_ai_messages_to_process())
					{
						messagesToProcess->push_back(this);
					}
				}
			}
		}
		break;
	case MessageTo::InRange:
		if (messageToRoom)
		{
			float rangeSqr = sqr(messageToRange);
			for_every_ptr(object, messageToRoom->get_objects())
			{
				Vector3 loc = object->get_presence()->get_centre_of_presence_WS();
				if ((loc - messageToLocation).length_squared() < rangeSqr)
				{
					if (object->does_handle_ai_message(get_name()))
					{
						if (MessagesToProcess* messagesToProcess = object->access_ai_messages_to_process())
						{
							messagesToProcess->push_back(this);
						}
					}
				}
			}
			// and nearby doors
			for_every_ptr(door, messageToRoom->get_doors())
			{
				if (!door->is_visible()) continue;

				auto* oDoor = door->get_door_on_other_side();
				auto* oRoom = door->get_room_on_other_side();
				if (oDoor && oRoom && oRoom->is_world_active())
				{
					float inFront = door->get_plane().get_in_front(messageToLocation);
					if (inFront >= 0.0f)
					{
						Vector3 onDoorWS;
						float doorDist = door->calculate_dist_of(messageToLocation, inFront, OUT_ onDoorWS, 0.0f);
						if (doorDist < messageToRange)
						{
							onDoorWS = door->get_other_room_transform().location_to_local(onDoorWS);
							Plane doorPlane = oDoor->get_plane();
							float oRangeSqr = sqr(messageToRange - doorDist);
							for_every_ptr(object, oRoom->get_objects())
							{
								Vector3 loc = object->get_presence()->get_centre_of_presence_WS();
								if (doorPlane.get_in_front(loc) >= 0.0f &&
									(loc - onDoorWS).length_squared() < oRangeSqr)
								{
									if (object->does_handle_ai_message(get_name()))
									{
										if (MessagesToProcess* messagesToProcess = object->access_ai_messages_to_process())
										{
											messagesToProcess->push_back(this);
										}
									}
								}
							}
						}
					}
				}
			}
		}
		break;
	case MessageTo::Undefined:
		// ignore
		break;
	default:
		todo_important(TXT("not recognised or not implemented yet"));
		break;
	}

	return true;
}
