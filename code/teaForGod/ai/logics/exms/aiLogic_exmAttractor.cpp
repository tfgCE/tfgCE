#include "aiLogic_exmAttractor.h"

#include "..\..\..\game\gameDirector.h"
#include "..\..\..\game\playerSetup.h"
#include "..\..\..\modules\custom\health\mc_health.h"
#include "..\..\..\modules\gameplay\moduleEXM.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\..\modules\gameplay\equipment\me_gun.h"

#include "..\..\..\..\core\containers\arrayStack.h"
#include "..\..\..\..\core\memory\safeObject.h"
#include "..\..\..\..\core\types\hand.h"

#include "..\..\..\..\framework\ai\aiMessage.h"
#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleSound.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\sound\soundScene.h"
#include "..\..\..\..\framework\world\room.h"
#include "..\..\..\..\framework\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai message
DEFINE_STATIC_NAME(attract);

// ai message params
DEFINE_STATIC_NAME(who);
DEFINE_STATIC_NAME(what);
DEFINE_STATIC_NAME(room);
DEFINE_STATIC_NAME(location);
DEFINE_STATIC_NAME(throughDoor);

//

REGISTER_FOR_FAST_CAST(EXMAttractor);

EXMAttractor::EXMAttractor(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
}

EXMAttractor::~EXMAttractor()
{
}

void EXMAttractor::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (pilgrimOwner.get() && pilgrimModule && owner.get() && hand != Hand::MAX)
	{
		if (pilgrimModule->has_controls_use_exm_been_pressed(hand) &&
			pilgrimModule->is_active_exm_equipped(hand, owner.get()))
		{
			if (exmModule)
			{
				if (exmModule->mark_exm_active_blink())
				{
					if (auto* s = owner->get_sound())
					{
						s->play_sound(NAME(attract));
					}
					if (auto* gd = GameDirector::get())
					{
						gd->game_clear_force_no_violence();
					}
					static int const MAX_ROOM_TO_SCAN = 64;
					ARRAY_STACK(Framework::Sound::ScannedRoom, rooms, MAX_ROOM_TO_SCAN);
					Framework::Sound::ScannedRoom::find(rooms, owner->get_presence()->get_in_room(), owner->get_presence()->get_placement().get_translation(),
						Framework::Sound::ScanRoomInfo()
						.with_concurrent_rooms_limit(1) // one is enough, get closest
						.with_depth_limit(6)
						.with_max_distance(200.0f)
						.with_max_distance_beyond_first_room(50.0f));
					for_every(room, rooms)
					{
						//FOR_EVERY_OBJECT_IN_ROOM(object, room->room)
						for_every_ptr(object, room->room->get_objects())
						{
							// only objects with AI can hear it
							if (object != owner.get() && object->get_ai())
							{
								if (auto * message = owner->get_in_world()->create_ai_message(NAME(attract)))
								{
									message->access_param(NAME(who)).access_as<SafePtr<Framework::IModulesOwner>>() = pilgrimOwner;
									message->access_param(NAME(what)).access_as<SafePtr<Framework::IModulesOwner>>() = owner;
									message->access_param(NAME(room)).access_as<SafePtr<Framework::Room>>() = room->room;
									message->access_param(NAME(location)).access_as<Vector3>() = room->entrancePoint;
									message->access_param(NAME(throughDoor)).access_as<SafePtr<Framework::DoorInRoom>>() = room->enteredThroughDoor;
									message->to_ai_object(object);
								}
							}
						}
					}
				}
			}
		}
	}
}
