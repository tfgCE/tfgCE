#include "aiLogicTask_stayClose.h"

#include "..\aiLogic_npcBase.h"

#include "aiLogicTask_lookAt.h"

#include "..\..\aiCommonVariables.h"

#include "..\..\..\..\core\system\core.h"

#include "..\..\..\..\framework\ai\aiLatentTask.h"
#include "..\..\..\..\framework\ai\aiLogic.h"
#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\general\cooldowns.h"
#include "..\..\..\..\framework\module\moduleAI.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\nav\navTask.h"
#include "..\..\..\..\framework\nav\tasks\navTask_FindPath.h"
#include "..\..\..\..\framework\nav\navLatentUtils.h"
#include "..\..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\..\framework\world\inRoomPlacement.h"
#include "..\..\..\..\framework\world\room.h"
#include "..\..\..\..\framework\world\roomUtils.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Tasks;

//

// cooldowns
DEFINE_STATIC_NAME(newPath);

//

LATENT_FUNCTION(Tasks::stay_close)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("stay close"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_PARAM_OPTIONAL(SafePtr<::Framework::IModulesOwner>, stayCloseTo, SafePtr<::Framework::IModulesOwner>());

	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);

	LATENT_VAR(::Framework::CooldownsSmall, cooldowns);
	LATENT_VAR(bool, pathJustSet);

	auto* imo = mind->get_owner_as_modules_owner();

	cooldowns.advance(LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("stay close"));
	ai_log_no_colour(mind->get_logic());

	while (stayCloseTo.is_set())
	{
		LATENT_CLEAR_LOG();
		if (!pathTask.is_set() && cooldowns.is_available(NAME(newPath)))
		{
			::Framework::Nav::PlacementAtNode stayCloseToNavLoc = mind->access_navigation().find_nav_location(stayCloseTo.get());
			pathTask = mind->access_navigation().find_path_to(stayCloseToNavLoc, Framework::Nav::PathRequestInfo(imo).with_dev_info(TXT("Tasks::stay_close")));
		}

		pathJustSet = false;
		if (pathTask.is_set())
		{
			LATENT_LOG(TXT("wait for nav task to end"));

			LATENT_NAV_TASK_WAIT_IF_SUCCEED(pathTask)
			{
				if (auto* task = fast_cast<Framework::Nav::Tasks::PathTask>(pathTask.get()))
				{
					LATENT_LOG(TXT("got a path"));
					path = task->get_path();
				}
				else
				{
					LATENT_LOG(TXT("what?"));
					path.clear();
				}
			}
			else
			{
				LATENT_LOG(TXT("failed"));
				path.clear();
			}
			pathTask = nullptr;
			cooldowns.set(NAME(newPath), Random::get_float(2.0f, 3.0f));
			pathJustSet = true;
		}

		if (path.is_set())
		{
			auto& locomotion = mind->access_locomotion();
			float pathDistance = 0.0f;
			if (! locomotion.check_if_path_is_ok(path.get()))
			{
				locomotion.dont_move();
				path.clear();
			}
			else if (path.get() && !path->is_close_to_the_end(imo->get_presence()->get_in_room(), imo->get_presence()->get_placement().get_translation(), 0.5f, NP, &pathDistance))
			{
				if (!locomotion.is_following_path(path.get()))
				{
					LATENT_LOG(TXT("move"));
					locomotion.follow_path_2d(path.get(), NP, Framework::MovementParameters().full_speed());
					locomotion.turn_follow_path_2d();
				}
			}
			else
			{
				locomotion.dont_move();
			}

			if (pathJustSet && pathDistance > 5.0f)
			{
				cooldowns.set(NAME(newPath), Random::get_float(4.0f, 7.0f));
			}
		}

		LATENT_WAIT(Random::get_float(0.1f, 0.3f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	{
		auto & locomotion = mind->access_locomotion();
		locomotion.stop();
	}

	LATENT_END_CODE();
	LATENT_RETURN();
}

