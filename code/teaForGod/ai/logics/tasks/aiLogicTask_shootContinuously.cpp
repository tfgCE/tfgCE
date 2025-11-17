#include "aiLogicTask_shootContinuously.h"

#include "aiLogicTask_shoot.h"

#include "..\aiLogic_npcBase.h"

#include "..\..\aiCommonVariables.h"
#include "..\..\aiRayCasts.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\ai\aiTaskHandle.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleSound.h"
#include "..\..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\..\framework\presence\relativeToPresencePlacement.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

LATENT_FUNCTION(TeaForGodEmperor::AI::Logics::Tasks::shoot_continuously)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("shoot continuosly"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_PARAM_OPTIONAL(int, shootCount, 1);
	LATENT_PARAM_OPTIONAL(float, shootingInterval, 0.3f);
	LATENT_PARAM_OPTIONAL(float, shootingSetInterval, 1.0f); // if more than 1 shoot count this is used between sets
	LATENT_PARAM_OPTIONAL(float, projectileSpeed, 0.0f); // if zero/not provided will use further one
	LATENT_PARAM_OPTIONAL(Array<NPCBase::ShotInfo> const*, useShotInfos, nullptr); // if we want to use something else
	LATENT_PARAM_OPTIONAL(bool, useSecondaryPerception, false); // only if available
	LATENT_PARAM_OPTIONAL(bool, checkClearTrace, true); // check if we may hit (only on the first shoot)
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(bool, unableToShoot);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(Vector3, enemyTargetingOffsetOS);
	LATENT_COMMON_VAR(bool, ignoreViolenceDisallowed);
	LATENT_COMMON_VAR(bool, confused);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(int, trueShootIndex);
	LATENT_VAR(int, shootIndex);
	LATENT_VAR(bool, doFullInterval);
	LATENT_VAR(bool, shootNow);
	LATENT_VAR(int, perceptionSocketIdx);

	auto * imo = mind->get_owner_as_modules_owner();
	auto* npcBase = fast_cast<NPCBase>(mind->get_logic());

	LATENT_BEGIN_CODE();

	trueShootIndex = 0;

	while (true)
	{
		doFullInterval = false;
		if (!unableToShoot || confused)
		{
			shootNow = true;
			if (npcBase && npcBase->get_max_attack_distance().is_set())
			{
				if (enemyPlacement.calculate_string_pulled_distance(imo->get_presence()->get_centre_of_presence_os(), Transform(enemyTargetingOffsetOS, Quat::identity)) > npcBase->get_max_attack_distance().get())
				{
					shootNow = false;
					doFullInterval = true;
				}
			}
			if (shootNow)
			{
				if (useSecondaryPerception)
				{
					perceptionSocketIdx = npcBase->get_secondary_perception_socket_index();
				}
				else
				{
					perceptionSocketIdx = npcBase->get_perception_socket_index();
				}
			
				if (! checkClearTrace ||
					(npcBase &&
					 enemyPlacement.get_target() &&
					 !unableToShoot &&
					 check_clear_ray_cast(CastInfo(), imo->get_presence()->get_placement().to_world(imo->get_appearance()->calculate_socket_os(perceptionSocketIdx)).get_translation(), imo,
						enemyPlacement.get_target(),
						enemyPlacement.get_in_final_room(), enemyPlacement.get_placement_in_owner_room())))
				{
					doFullInterval = true;
					for (shootIndex = 0;
						 shootIndex < shootCount && ! confused;
						 ++shootIndex, ++trueShootIndex)
					{
						if (!Functions::perform_shoot(mind, trueShootIndex, enemyPlacement, enemyTargetingOffsetOS, projectileSpeed, ignoreViolenceDisallowed, useShotInfos))
						{
							doFullInterval = false;
							shootIndex = shootCount;
						}

						if (shootIndex < shootCount - 1)
						{
							LATENT_WAIT(shootingInterval);
						}
					}
				}
				else
				{
					trueShootIndex = 0;
				}
			}
		}
		if (!doFullInterval)
		{
			LATENT_WAIT(0.2f);
		}
		else
		{
			if (shootCount > 1)
			{
				LATENT_WAIT(shootingSetInterval);
			}
			else
			{
				LATENT_WAIT(shootingInterval);
			}
		}
	}

	LATENT_ON_BREAK();
	//
	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

