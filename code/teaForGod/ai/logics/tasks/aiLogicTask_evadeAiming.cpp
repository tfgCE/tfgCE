#include "aiLogicTask_evadeAiming.h"

#include "..\..\..\modules\gameplay\modulePilgrim.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\ai\aiLatentTask.h"
#include "..\..\..\..\framework\ai\aiTaskHandle.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\presence\presencePath.h"
#include "..\..\..\..\framework\presence\relativeToPresencePlacement.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

DEFINE_STATIC_NAME(evade);

//

bool TeaForGodEmperor::AI::Logics::Tasks::is_aiming_at_me(Framework::PresencePath const & _pathToEnemy, Framework::IModulesOwner * _me, float _maxOffPath, float _maxAngleOff, float _maxDistance)
{
	if (auto* enemy = _pathToEnemy.get_target())
	{
		if (auto* pilgrimTarget = enemy->get_gameplay_as<TeaForGodEmperor::ModulePilgrim>())
		{
			float distance;
			if (pilgrimTarget->is_aiming_at(_me, &_pathToEnemy, OUT_ &distance, _maxOffPath, _maxAngleOff))
			{
				if (_maxDistance <= 0.0f || distance < _maxDistance)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool TeaForGodEmperor::AI::Logics::Tasks::is_aiming_at_me(Framework::RelativeToPresencePlacement const & _pathToEnemy, Framework::IModulesOwner * _me, float _maxOffPath, float _maxAngleOff, float _maxDistance)
{
	if (auto* enemy = _pathToEnemy.get_target())
	{
		if (auto* pilgrimTarget = enemy->get_gameplay_as<TeaForGodEmperor::ModulePilgrim>())
		{
			float distance;
			if (pilgrimTarget->is_aiming_at(_me, &_pathToEnemy, OUT_ &distance, _maxOffPath, _maxAngleOff))
			{
				if (_maxDistance <= 0.0f || distance < _maxDistance)
				{
					return true;
				}
			}
		}
	}

	return false;
}

LATENT_FUNCTION(TeaForGodEmperor::AI::Logics::Tasks::evade_aiming_3d)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("evade aiming 3d"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_PARAM(::Framework::RelativeToPresencePlacement*, evadeObject);
	LATENT_END_PARAMS();

	LATENT_BEGIN_CODE();

	if (mind)
	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.stop();

		if (auto* target = evadeObject->get_target())
		{
			auto * presence = mind->get_owner_as_modules_owner()->get_presence();
			Vector3 currentLocation = presence->get_placement().get_translation();
			Vector3 evadeInDir = Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f)).normal();

			if (evadeObject->is_active())
			{
				Vector3 evadeObjectLocation = evadeObject->location_from_target_to_owner(target->ai_get_placement().get_translation());
				evadeInDir = evadeInDir.drop_using(evadeObjectLocation - currentLocation); // move perpendiculary to target
			}
			evadeInDir = (evadeInDir * 0.3f + 0.7f * evadeInDir.drop_using_normalised(presence->get_environment_up_dir())).normal(); // prefer leveled movement
			locomotion.move_to_3d(currentLocation + evadeInDir * Random::get_float(1.5f, 2.5f), 0.7f, Framework::MovementParameters().full_speed().gait(NAME(evade)));
			locomotion.turn_towards_3d(target, 1.0f);
		}
	}

	LATENT_WAIT(Random::get_float(0.4f, 1.2f));
	LATENT_END();

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

