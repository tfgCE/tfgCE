#include "aiLogicTask_projectileHit.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\ai\aiLatentTask.h"
#include "..\..\..\..\framework\ai\aiTaskHandle.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\world\room.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

DEFINE_STATIC_NAME(evade);
DEFINE_STATIC_NAME(hit);
DEFINE_STATIC_NAME(inRoom);
DEFINE_STATIC_NAME(inRoomLocation);
DEFINE_STATIC_NAME(inRoomNormal);

//

bool TeaForGodEmperor::AI::Logics::Tasks::handle_projectile_hit_message(::Framework::AI::MindInstance* _mind, ::Framework::AI::LatentTaskInfoWithParams & _nextTask, Framework::AI::Message const & _message, float _distance, LATENT_FUNCTION_VARIABLE(_avoidTaskFunction), int _priority, bool _allowToInterrupt)
{
	auto const * inRoomParam = _message.get_param(NAME(inRoom));
	auto const * inRoomLocationParam = _message.get_param(NAME(inRoomLocation));
	auto const * hitIMO = _message.get_param(NAME(hit));
	if (inRoomParam && inRoomLocationParam)
	{
		auto * imo = _mind->get_owner_as_modules_owner();
		if (imo == hitIMO->get_as<SafePtr<Framework::IModulesOwner>>().get())
		{
			return false;
		}
		if (auto* inRoom = inRoomParam->get_as<SafePtr<Framework::Room>>().get())
		{
			if (inRoom == imo->get_presence()->get_in_room())
			{
				Vector3 hitLocation = inRoomLocationParam->get_as<Vector3>();
				if ((imo->get_presence()->get_placement().location_to_world(imo->get_presence()->get_centre_of_presence_os().get_translation()) - hitLocation).length_squared() < sqr(0.6f))
				{
					if (_nextTask.propose(AI_LATENT_TASK_FUNCTION(_avoidTaskFunction), _priority, _allowToInterrupt))
					{
						ADD_LATENT_TASK_INFO_PARAM(_nextTask, ::Framework::Room*, inRoom);
						ADD_LATENT_TASK_INFO_PARAM(_nextTask, Vector3, hitLocation);
						return true;
					}
				}
			}
		}
	}

	return false;
}

LATENT_FUNCTION(TeaForGodEmperor::AI::Logics::Tasks::avoid_projectile_hit_3d)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("avoid projectile hit 3d"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_PARAM_NOT_USED(::Framework::Room*, room);
	LATENT_PARAM(Vector3, hitLocation);
	LATENT_END_PARAMS();

	Framework::ModulePresence const * presence;
	Vector3 currentLocation;
	Vector3 avoidDiff;
	::Framework::AI::Locomotion * locomotion;

	LATENT_BEGIN_CODE();

	LATENT_WAIT(Random::get_float(0.0f, 0.2f));

	locomotion = &mind->access_locomotion();

	presence = mind->get_owner_as_modules_owner()->get_presence();
	currentLocation = presence->get_placement().get_translation();
	avoidDiff = (currentLocation - hitLocation).normal();
	locomotion->dont_keep_distance();
	locomotion->move_to_3d(currentLocation + avoidDiff.normal() * Random::get_float(1.0f, 2.5f), 0.3f, Framework::MovementParameters().full_speed().gait(NAME(evade)));
	LATENT_WAIT(Random::get_float(0.2f, 0.5f));

	LATENT_END();

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

