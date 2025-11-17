#include "aiLogicTask_stayCloseInFront.h"

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
DEFINE_STATIC_NAME(goingFurther); // is going further
DEFINE_STATIC_NAME(goFurtherInterval); // interval triggered

//

enum StayCloseState
{
	StayClose,
	GoFurtherStart,
	GoFurtherMove
};

LATENT_FUNCTION(Tasks::stay_close_in_front)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("stay close in front"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_PARAM_OPTIONAL(SafePtr<::Framework::IModulesOwner>, stayCloseTo, SafePtr<::Framework::IModulesOwner>());
	LATENT_PARAM_OPTIONAL(float, inFrontDist, 0.6f);

	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Range, stayCloseGoFurtherDist);
	LATENT_COMMON_VAR(Range, stayCloseGoFurtherTime);
	LATENT_COMMON_VAR(Range, stayCloseGoFurtherInterval);
	LATENT_COMMON_VAR(Name, stayCloseGait);
	LATENT_COMMON_VAR(float, stayCloseGaitDist);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);

	LATENT_VAR(::Framework::CooldownsSmall, cooldowns);
	LATENT_VAR(bool, pathJustSet);
	LATENT_VAR(bool, isClose);
	LATENT_VAR(bool, requiresGoFurtherInterval);
	LATENT_VAR(bool, useGoFurther);
	
	LATENT_VAR(StayCloseState, state);

	LATENT_VAR(::Framework::PresencePath, stayCloseToPath);

	auto* imo = mind->get_owner_as_modules_owner();
#ifdef AN_USE_AI_LOG
	auto * self = mind->get_logic();
#endif

	cooldowns.advance(LATENT_DELTA_TIME);

#ifdef AN_DEVELOPMENT
	{
		Optional<float> gf = cooldowns.get(NAME(goingFurther));
		if (gf.is_set())
		{
			ai_state(self, 2, TXT("gf: %.3f"), gf.get());
		}
		else
		{
			ai_state_clear(self, 2);
		}
	}
#endif

	LATENT_BEGIN_CODE();

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("stay close"));
	ai_log_no_colour(mind->get_logic());

	if (auto* sct = stayCloseTo.get())
	{
		stayCloseToPath.find_path(imo, sct);
	}

	isClose = false;
	requiresGoFurtherInterval = true;
	useGoFurther = !stayCloseGoFurtherDist.is_empty() && !stayCloseGoFurtherTime.is_empty();
	
	state = StayCloseState::StayClose;
	ai_state(self, 1, TXT("stay close"));

	while (stayCloseTo.is_set())
	{
		// decide on states
		if (state == StayCloseState::StayClose)
		{
			if (useGoFurther && cooldowns.is_available(NAME(goFurtherInterval)) && isClose)
			{
				if (requiresGoFurtherInterval)
				{
					cooldowns.set(NAME(goFurtherInterval), Random::get(stayCloseGoFurtherInterval));
					requiresGoFurtherInterval = false;
				}
				else
				{
					state = StayCloseState::GoFurtherStart;
					ai_state(self, 1, TXT("go further (start)"));
				}
			}
			else if (!pathTask.is_set() && cooldowns.is_available(NAME(newPath)))
			{
				LATENT_LOG(TXT("stay close"));
				::Framework::Nav::PlacementAtNode stayCloseToNavLoc = mind->access_navigation().find_nav_location(stayCloseTo.get(), Vector3::yAxis * inFrontDist);
				pathTask = mind->access_navigation().find_path_to(stayCloseToNavLoc, Framework::Nav::PathRequestInfo(imo).with_dev_info(TXT("Tasks::stay_close_in_front stay close")));
			}
		}
		if (state == StayCloseState::GoFurtherStart)
		{
			if (!pathTask.is_set())
			{
				LATENT_LOG(TXT("go further"));
				::Framework::Nav::PlacementAtNode stayCloseToNavLoc = mind->access_navigation().find_nav_location(stayCloseTo.get(), Vector3::yAxis * inFrontDist);
				Framework::RelativeToPresencePlacement r2pp;
				r2pp.be_temporary_snapshot();
				if (r2pp.find_path(imo, stayCloseTo.get()))
				{
					pathTask = mind->access_navigation().get_random_path(Random::get(stayCloseGoFurtherDist), r2pp.vector_from_target_to_owner(stayCloseTo->get_presence()->get_placement().get_axis(Axis::Forward)), Framework::Nav::PathRequestInfo(imo).with_dev_info(TXT("Tasks::stay_close_in_front go further start")));
				}
				else
				{
					pathTask = mind->access_navigation().get_random_path(Random::get(stayCloseGoFurtherDist), imo->get_presence()->get_placement().get_axis(Axis::Forward), Framework::Nav::PathRequestInfo(imo).with_dev_info(TXT("Tasks::stay_close_in_front go further start no path")));
				}
			}
		}
		if (state == StayCloseState::GoFurtherMove)
		{
			if (cooldowns.is_available(NAME(goingFurther)))
			{
				state = StayCloseState::StayClose;
				ai_state(self, 1, TXT("stay close"));
				if (!stayCloseGoFurtherInterval.is_empty())
				{
					cooldowns.set(NAME(goFurtherInterval), Random::get(stayCloseGoFurtherInterval));
				}
			}
		}

		// path task handling
		pathJustSet = false;
		if (pathTask.is_set())
		{
			LATENT_LOG(TXT("wait for nav task to end"));

			auto* pt = pathTask.get();
			if (pt && pt->is_done())
			{
				if (pt->has_succeed())
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

				if (state == StayCloseState::GoFurtherStart)
				{
					state = StayCloseState::GoFurtherMove;
					ai_state(self, 1, TXT("go further (move)"));
					cooldowns.set(NAME(goingFurther), Random::get(stayCloseGoFurtherTime));
				}
			}
		}

		// path following
		isClose = false;
		if (path.is_set())
		{
			auto& locomotion = mind->access_locomotion();
			float pathDistance = path->calculate_distance(imo->get_presence()->get_in_room(), imo->get_presence()->get_placement().get_translation());
			if (state == StayCloseState::StayClose)
			{
				isClose = pathDistance < 0.5f;
			}
			if (path.get() && pathDistance > 0.15f)
			{
				if (!locomotion.is_following_path(path.get()))
				{
					LATENT_LOG(TXT("move"));
					locomotion.follow_path_2d(path.get(), NP, Framework::MovementParameters().full_speed());
					locomotion.turn_follow_path_2d();
					if (stayCloseGait.is_valid() && (pathDistance > stayCloseGaitDist || state == StayCloseState::GoFurtherMove))
					{
						locomotion.access_movement_parameters().gait(stayCloseGait);
					}
					else
					{
						locomotion.access_movement_parameters().default_gait();
					}
				}
			}
			else
			{
				locomotion.dont_move();
				if (state == StayCloseState::StayClose)
				{
					if (stayCloseToPath.get_target_presence())
					{
						Vector3 targetPointWS = stayCloseToPath.get_target_presence()->get_placement().location_to_world(Vector3::yAxis * 20.0f);
						Transform targetPlacementWS = Transform(targetPointWS, Quat::identity);
						targetPlacementWS = stayCloseToPath.from_target_to_owner(targetPlacementWS);
						locomotion.turn_towards_2d(targetPlacementWS.get_translation(), 5.0f);
					}
				}
				else
				{
					locomotion.dont_turn();
					if (state == StayCloseState::GoFurtherMove)
					{
						cooldowns.clear(NAME(goingFurther));
					}
				}
			}

			if (state == StayCloseState::StayClose)
			{
				if (pathJustSet)
				{
					if (pathDistance > 7.0f)
					{
						// block for a while
						cooldowns.set(NAME(newPath), Random::get_float(4.0f, 6.0f));
					}
				}
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
	ai_state_clear(self, 1);
	ai_state_clear(self, 2);

	LATENT_END_CODE();
	LATENT_RETURN();
}

