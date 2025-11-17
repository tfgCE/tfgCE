#include "aiLogicTask_scanAndWander.h"

#include "..\aiLogic_npcBase.h"

#include "aiLogicTask_lookAt.h"
#include "aiLogicTask_scan.h"
#include "aiLogicTask_wander.h"

#include "..\..\aiCommonVariables.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\..\framework\general\cooldowns.h"
#include "..\..\..\..\framework\latent\latent.h"
#include "..\..\..\..\framework\module\moduleAI.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\world\inRoomPlacement.h"
#include "..\..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Tasks;

//

// ai hunch names
DEFINE_STATIC_NAME(wander);

// ai messages
DEFINE_STATIC_NAME(enemyPlacement);
	DEFINE_STATIC_NAME(who);
	DEFINE_STATIC_NAME(enemy);
	DEFINE_STATIC_NAME(room);
	DEFINE_STATIC_NAME(placement);

//

LATENT_FUNCTION(Tasks::scan_and_wander)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("scan and wander"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(bool, perceptionStartOff);
	LATENT_COMMON_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	LATENT_COMMON_VAR(Vector3, enemyTargetingOffsetOS);
	LATENT_COMMON_VAR(bool, unableToScan);
	LATENT_COMMON_VAR(Framework::InRoomPlacement, investigate);
	LATENT_COMMON_VAR(int, investigateIdx);
	LATENT_COMMON_VAR(bool, encouragedInvestigate);
	LATENT_COMMON_VAR(bool, forceInvestigate);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, task);
	LATENT_VAR(float, scanTime);
	LATENT_VAR(float, wanderTime);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);
	LATENT_VAR(Framework::CooldownsSmall, cooldowns);
	LATENT_VAR(Framework::RelativeToPresencePlacement, lookAtEnemyPlacement);

	LATENT_VAR(int, isInvestigatingIdx);

	auto* imo = mind->get_owner_as_modules_owner();

	scanTime = max(0.0f, scanTime - LATENT_DELTA_TIME);
	wanderTime = max(0.0f, wanderTime - LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("scan and wander"));
	ai_log_no_colour(mind->get_logic());

	messageHandler.use_with(mind);
	{
		auto* framePtr = &_frame;
		messageHandler.set(NAME(enemyPlacement), [framePtr, mind, imo, &task, &enemy, &cooldowns, &lookAtEnemyPlacement, &enemyTargetingOffsetOS, &unableToScan](Framework::AI::Message const& _message)
			{
				if (!unableToScan &&
					cooldowns.is_available(NAME(enemyPlacement)))
				{
					cooldowns.add(NAME(enemyPlacement), Random::get_float(2.0f, 5.0f));

					if (auto* enemyPtr = _message.get_param(NAME(enemy)))
					{
						if (auto* enemyP = enemyPtr->get_as<SafePtr<Framework::IModulesOwner>>().get())
						{
							if (enemy == enemyP)
							{
								if (auto* inRoomPtr = _message.get_param(NAME(room)))
								{
									if (auto* inRoom = inRoomPtr->get_as<SafePtr<Framework::Room>>().get())
									{
										if (inRoom == imo->get_presence()->get_in_room())
										{
											if (auto* placementPtr = _message.get_param(NAME(placement)))
											{
												ai_log_colour(mind->get_logic(), Colour::cyan);
												ai_log(mind->get_logic(), TXT("[scan and wander] look at enemy placement"));
												ai_log_no_colour(mind->get_logic());

												lookAtEnemyPlacement.find_path(imo, inRoom, inRoomPtr->get_as<Transform>());
												{
													::Framework::AI::LatentTaskInfoWithParams lookAtTaskInfo;
													lookAtTaskInfo.clear();
													lookAtTaskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::look_at));
													ADD_LATENT_TASK_INFO_PARAM_OPTIONAL_NOT_PROVIDED(lookAtTaskInfo);
													ADD_LATENT_TASK_INFO_PARAM_OPTIONAL_NOT_PROVIDED(lookAtTaskInfo);
													ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, ::Framework::RelativeToPresencePlacement*, &lookAtEnemyPlacement);
													ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, Vector3*, &enemyTargetingOffsetOS);
													task.start_latent_task(mind, lookAtTaskInfo);
												}

												framePtr->end_waiting();
												AI_LATENT_STOP_LONG_RARE_ADVANCE();
											}
										}
									}
								}
							}
						}
					}
				}
			}
		);
	}

	// if we're supposed to wait, wait idle
	if (perceptionStartOff)
	{
		{
			auto& locomotion = mind->access_locomotion();
			locomotion.dont_move();
			locomotion.dont_turn();
		}
		while (perceptionStartOff)
		{
			LATENT_WAIT(Random::get_float(0.2f, 0.6f));
		}
	}

	isInvestigatingIdx = investigateIdx;

	if ((forceInvestigate || encouragedInvestigate) && investigate.is_valid())
	{
		ai_log(mind->get_logic(), TXT("start with wander (force investigate)"));
		goto WANDER;
	}
	if (mind->access_execution().has_hunch(NAME(wander), true))
	{
		ai_log(mind->get_logic(), TXT("start with wander (hunch)"));
		goto WANDER;
	}
	if (Random::get_chance(investigate.is_valid()? 0.6f : 0.3f))
	{
		ai_log(mind->get_logic(), TXT("start with wander (random chance)"));
		goto WANDER;
	}

	while (true)
	{
		LATENT_CLEAR_LOG();
		//SCAN:
		{
			{	// do not scan if at a transport route (which is also a room that we may leave)
				if (Mobility::may_leave_room(imo, imo->get_presence()->get_in_room()))
				{
					if (mind->access_navigation().is_at_transport_route())
					{
						LATENT_LOG(TXT("transport route that we may leave - skip scanning and wander now"));
						goto WANDER;
					}
				}
			}
			if (investigate.is_valid() && (forceInvestigate || encouragedInvestigate))
			{
				goto WANDER;
			}
			if (investigate.is_valid() || Investigate::is_in_worth_scanning(imo))
			{
				LATENT_LOG(TXT("scan %S"), investigate.is_valid()? TXT("to investigate") : TXT("not to investigate"));
				if (!unableToScan &&
					!task.is_running(scan))
				{
					::Framework::AI::LatentTaskInfoWithParams taskInfo;
					taskInfo.clear();
					taskInfo.propose(AI_LATENT_TASK_FUNCTION(scan));
					task.start_latent_task(mind, taskInfo);
				}
				scanTime = Random::get_float(3.0f, 10.0f);
				while (scanTime > 0.0f && !unableToScan && !forceInvestigate && !encouragedInvestigate && isInvestigatingIdx == investigateIdx)
				{
					LATENT_YIELD();
				}
			}
		}

		WANDER:
		{
			if (Mobility::should_remain_stationary(imo) &&
				! (investigate.is_valid() && (forceInvestigate || encouragedInvestigate)))
			{
				LATENT_LOG(TXT("remain stationary"));
				// if to remain stationary, just stay here
				LATENT_WAIT(Random::get_float(1.0f, 2.0f));
			}
			else if (!task.is_running(wander))
			{
				LATENT_LOG(TXT("wander"));
				{
					::Framework::AI::LatentTaskInfoWithParams taskInfo;
					taskInfo.clear();
					taskInfo.propose(AI_LATENT_TASK_FUNCTION(wander));
					task.start_latent_task(mind, taskInfo);
				}
				LATENT_WAIT_FOR_TASK(Random::get_float(3.0f, 10.0f), task);
			}
			else
			{
				// wait a bit more wandering
				LATENT_WAIT_FOR_TASK(Random::get_float(1.0f, 3.0f), task);
			}
		}

		// yield so we won't end up with infinite loop (possible!)
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	task.stop();

	LATENT_END_CODE();

	// update allow to interrupt state
	thisTaskHandle->allow_to_interrupt(task.can_start_new());

	LATENT_RETURN();
}

