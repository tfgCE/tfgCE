#include "aiLogic_exmRemoteControl.h"

#include "..\..\..\game\gameDirector.h"
#include "..\..\..\modules\custom\mc_switchSidesHandler.h"
#include "..\..\..\modules\gameplay\moduleEXM.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"

#include "..\..\..\..\core\containers\arrayStack.h"

#include "..\..\..\..\framework\ai\aiMessage.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleSound.h"
#include "..\..\..\..\framework\sound\soundScene.h"
#include "..\..\..\..\framework\world\room.h"
#include "..\..\..\..\framework\world\world.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// sounds
DEFINE_STATIC_NAME(remoteControl);

// ai messages
DEFINE_STATIC_NAME(who);

// tags
DEFINE_STATIC_NAME(remoteControlTarget);

// variables
DEFINE_STATIC_NAME(distanceInRoom);
DEFINE_STATIC_NAME(distanceBeyondRoom);

//

REGISTER_FOR_FAST_CAST(EXMRemoteControl);

EXMRemoteControl::EXMRemoteControl(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
}

EXMRemoteControl::~EXMRemoteControl()
{
}

void EXMRemoteControl::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	distanceInRoom = _parameters.get_value(NAME(distanceInRoom), distanceInRoom);
	distanceBeyondRoom = _parameters.get_value(NAME(distanceBeyondRoom), distanceBeyondRoom);
}

void EXMRemoteControl::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	for(int i = 0; i < objectsToControl.get_size(); ++ i)
	{
		auto& otc = objectsToControl[i];
		otc.delay -= _deltaTime;
		if (otc.delay <= 0.0f)
		{
			if (auto* object = otc.imo.get())
			{
				if (auto* ssh = object->get_custom<CustomModules::SwitchSidesHandler>())
				{
					ssh->get_switch_sides_to(pilgrimOwner.get());
				}
				if (auto* o = object->get_as_object())
				{
					if (o->get_tags().get_tag(NAME(remoteControlTarget)))
					{
						if (auto* message = owner->get_in_world()->create_ai_message(NAME(remoteControl)))
						{
							message->access_param(NAME(who)).access_as<SafePtr<Framework::IModulesOwner>>() = pilgrimOwner;
							message->to_ai_object(object);
						}
					}
				}
			}
			objectsToControl.remove_fast_at(i);
			--i;
		}
	}

	if (pilgrimOwner.get() && pilgrimModule && exmModule && owner.get() && hand != Hand::MAX && objectsToControl.is_empty())
	{
		if (auto* gd = GameDirector::get())
		{
			//if (!gd->game_is_no_violence_forced())
			{
				if (pilgrimModule->has_controls_use_exm_been_pressed(hand) &&
					pilgrimModule->is_active_exm_equipped(hand, owner.get()))
				{
					bool control = exmModule->mark_exm_active_blink();

					if (control)
					{
						Vector3 startLoc = owner->get_presence()->get_placement().get_translation();
						static int const MAX_ROOM_TO_SCAN = 64;
						ARRAY_STACK(Framework::Sound::ScannedRoom, rooms, MAX_ROOM_TO_SCAN);
						Framework::Sound::ScannedRoom::find(rooms, owner->get_presence()->get_in_room(), startLoc,
							Framework::Sound::ScanRoomInfo()
							.with_concurrent_rooms_limit(1) // one is enough, get closest
							.with_depth_limit(6)
							.with_max_distance(distanceInRoom)
							.with_max_distance_beyond_first_room(distanceBeyondRoom));
						for_every(room, rooms)
						{
							float roomEntranceDistance = room->distance;
							Vector3 roomEntranceLoc = startLoc;
							if (auto* door = room->enteredThroughDoor)
							{
								roomEntranceLoc = room->enteredThroughDoor->get_hole_centre_WS();
							}
							//FOR_EVERY_OBJECT_IN_ROOM(object, room->room)
							for_every_ptr(object, room->room->get_objects())
							{
								// only objects with AI can hear it
								if (object != owner.get() && object->get_ai())
								{
									if (object->get_custom<CustomModules::SwitchSidesHandler>() ||
										object->get_tags().get_tag(NAME(remoteControlTarget)))
									{
										float distance = roomEntranceDistance + (object->get_presence()->get_centre_of_presence_WS() - roomEntranceLoc).length();
										objectsToControl.push_back();
										auto& otc = objectsToControl.get_last();
										otc.imo = object;
										otc.delay = distance / distanceInRoom;
									}
								}
							}
						}
						if (auto* s = owner->get_sound())
						{
							s->play_sound(NAME(remoteControl));
						}
					}
				}
			}
		}
	}
}
