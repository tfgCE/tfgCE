#include "aiLogic_turret.h"

#include "tasks\aiLogicTask_attackOrIdle.h"
#include "tasks\aiLogicTask_hideDisappear.h"
#include "tasks\aiLogicTask_perception.h"

#include "utils\aiLogicUtil_shootingAccuracy.h"

#include "..\aiCommonVariables.h"
#include "..\aiRayCasts.h"
#include "..\perceptionRequests\aipr_FindAnywhere.h"

#include "..\..\game\gameDirector.h"
#include "..\..\game\gameSettings.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\music\musicPlayer.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiPerceptionRequest.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\nav\navPath.h"
#include "..\..\..\framework\nav\navTask.h"
#include "..\..\..\framework\nav\tasks\navTask_FindPath.h"
#include "..\..\..\framework\nav\navLatentUtils.h"
#include "..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\inRoomPlacement.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\roomUtils.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// sounds / temporary objects
DEFINE_STATIC_NAME(shoot);
DEFINE_STATIC_NAME(muzzleFlash); 
DEFINE_STATIC_NAME(attack);

// task feedback
DEFINE_STATIC_NAME(taskFeedback);
DEFINE_STATIC_NAME(enemyLost);

// emissive layers
//DEFINE_STATIC_NAME(attack);

// tasks
DEFINE_STATIC_NAME(attackSequence);
DEFINE_STATIC_NAME(idleSequence);

// vars
DEFINE_STATIC_NAME(turretTargetMS);
DEFINE_STATIC_NAME(turretTargetingSocket);
DEFINE_STATIC_NAME(projectileSpeed);
DEFINE_STATIC_NAME(projectileCount);
DEFINE_STATIC_NAME(projectileInterval);
DEFINE_STATIC_NAME(projectileSeriesInterval);
DEFINE_STATIC_NAME(timeToStartShooting);
DEFINE_STATIC_NAME(timeToGiveUpShooting);
DEFINE_STATIC_NAME(timeToShootWhenDoesntSeeEnemy);

// params
DEFINE_STATIC_NAME(turretIdleBehaviour);
DEFINE_STATIC_NAME(turretScanInterval);
DEFINE_STATIC_NAME(turretScanStep);
DEFINE_STATIC_NAME(turretScanPOI);

// idle behaviour values
DEFINE_STATIC_NAME(scan);
DEFINE_STATIC_NAME_STR(scanDoors, TXT("scan doors"));
DEFINE_STATIC_NAME_STR(scanFacingDoors, TXT("scan facing doors"));
DEFINE_STATIC_NAME_STR(scanPOIs, TXT("scan POIs"));

// collision flags
DEFINE_STATIC_NAME(worldSolidFlags);

// tags
DEFINE_STATIC_NAME(window);

//

REGISTER_FOR_FAST_CAST(Turret);

Turret::Turret(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	turretTargetMSVarID = NAME(turretTargetMS);
	idleBehvaiour = NAME(scan);
}

Turret::~Turret()
{
}

void Turret::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		auto & vars = imo->access_variables();
		if (!turretTargetMSVar.is_valid())
		{
			turretTargetMSVar = vars.find<Vector3>(turretTargetMSVarID);
		}

		turretTargetMSVar.access<Vector3>() = turretTargetMS;
	}
}

LATENT_FUNCTION(Turret::remain_idle)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("remain idle"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(int, doorIdx);

	auto* imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Turret>(mind->get_logic());

	LATENT_BEGIN_CODE();

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("remain idle"));
	ai_log_no_colour(mind->get_logic());

	if (self->idleBehvaiour == NAME(scan))
	{
		while (true)
		{
			self->turretTargetMS = Vector3(0.0f, 10.0f, 0.0f).rotated_by_pitch(self->get_perception_vertical_fov().max * Random::get_float(0.5f, 1.2f)).rotated_by_yaw(Rotator3::get_yaw(self->turretTargetMS) + Random::get(self->scanStep));
			LATENT_WAIT(Random::get(self->scanInterval));
		}
	}
	else if (self->idleBehvaiour == NAME(scanDoors) ||
			 self->idleBehvaiour == NAME(scanFacingDoors))
	{
		doorIdx = 0;
		while (true)
		{
			if (auto* inRoom = imo->get_presence()->get_in_room())
			{
				if (! inRoom->get_doors().is_empty())
				{
					scoped_call_stack_info(TXT("check room"));
					scoped_call_stack_info(self->idleBehvaiour.to_char());
					scoped_call_stack_ptr(inRoom);
					Vector3 ourLoc = imo->get_presence()->get_placement().get_translation();
					int triesLeft = 20;
					while (triesLeft)
					{
						doorIdx = doorIdx % inRoom->get_doors().get_size();
						{
							bool skipThisDoor = true;
							if (auto* dir = inRoom->get_doors()[doorIdx])
							{
								if (self->idleBehvaiour == NAME(scanFacingDoors) &&
									Vector3::dot(dir->get_dir_inside(), ourLoc - dir->get_hole_centre_WS()) < 0.0f)
								{
									// the door is not facing us
									skipThisDoor = true;
								}
								else
								{
									if (auto* d = dir->get_door())
									{
										if (auto* dt = d->get_door_type())
										{
											if (!d->get_tags().has_tag(NAME(window)))
											{
												skipThisDoor = false;
											}
										}
									}
								}
							}
							if (skipThisDoor)
							{
								++doorIdx;
							}
							else
							{
								break;
							}
						}
						--triesLeft;
					}
					doorIdx = doorIdx % inRoom->get_doors().get_size();
					Vector3 doorAtWS = inRoom->get_doors()[doorIdx]->get_hole_centre_WS();
					self->turretTargetMS = imo->get_appearance()->get_ms_to_ws_transform().location_to_local(doorAtWS);
					++doorIdx;
				}
			}
			LATENT_WAIT(Random::get(self->scanInterval));
		}
	}
	else if (self->idleBehvaiour == NAME(scanPOIs))
	{
		while (true)
		{
			if (auto* inRoom = imo->get_presence()->get_in_room())
			{
				ARRAY_STACK(Framework::PointOfInterestInstance*, pois, 256);
				Random::Generator rg;
				inRoom->for_every_point_of_interest(self->scanPOI, [&pois, &rg](Framework::PointOfInterestInstance * _fpoi) {pois.push_back_or_replace(_fpoi, rg); });
				if (!pois.is_empty())
				{
					Vector3 targetLocWS = pois[rg.get_int(pois.get_size())]->calculate_placement().get_translation();
					self->turretTargetMS = imo->get_appearance()->get_ms_to_ws_transform().location_to_local(targetLocWS);
				}
			}
			LATENT_WAIT(Random::get(self->scanInterval));
		}
	}
	else
	{
		while (true)
		{
			self->turretTargetMS = Vector3(0.0f, 1000.0f, 0.0f);
			LATENT_WAIT(Random::get(self->scanInterval));
		}
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Turret::attack_enemy)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("attack enemy"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(Vector3, enemyTargetingOffsetOS);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(float, timeToIdle);
	LATENT_VAR(float, timeToShoot);
	LATENT_VAR(float, timeToGiveUpShooting);
	LATENT_VAR(int, consequtiveShots);
	LATENT_VAR(int, projectileCount);
	LATENT_VAR(float, projectileInterval);
	LATENT_VAR(float, projectileSeriesInterval);
	LATENT_VAR(float, timeToShootWhenDoesntSeeEnemy);

	auto* imo = mind->get_owner_as_modules_owner();
#ifdef AN_USE_AI_LOG
	an_assert(fast_cast<NPCBase>(mind->get_logic()));
#endif
	auto * self = fast_cast<Turret>(mind->get_logic());

	timeToIdle -= LATENT_DELTA_TIME;
	timeToShoot -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("attack enemy (%S)"), enemy.get() ? enemy->ai_get_name().to_char() : TXT("??"));
	ai_log_no_colour(mind->get_logic());

	timeToShoot = imo->get_variables().get_value<float>(NAME(timeToStartShooting), 1.25f) * GameSettings::get().difficulty.aiReactionTime;
	timeToGiveUpShooting = imo->get_variables().get_value<float>(NAME(timeToGiveUpShooting), 2.0f);
	if (auto* range = imo->get_variables().get_existing<Range>(NAME(timeToGiveUpShooting)))
	{
		timeToGiveUpShooting = Random::get(*range);
	}
	timeToIdle = timeToGiveUpShooting;
	consequtiveShots = 0;
	projectileCount = imo->get_variables().get_value<int>(NAME(projectileCount), 4);
	projectileInterval = imo->get_variables().get_value<float>(NAME(projectileInterval), 0.15f);
	projectileSeriesInterval = imo->get_variables().get_value<float>(NAME(projectileSeriesInterval), 1.0f) * (0.5f + 0.5f * GameSettings::get().difficulty.aiReactionTime);
	timeToShootWhenDoesntSeeEnemy = imo->get_variables().get_value<float>(NAME(timeToShootWhenDoesntSeeEnemy), -1.0f) * (0.5f + 0.5f * GameSettings::get().difficulty.aiReactionTime); // if negative, will use normal one

	while (timeToIdle > 0.0f)
	{
		{
			bool canShootNow = consequtiveShots != 0; // continue shooting
			bool follow = false;
			Vector3 targetAt = imo->get_appearance()->get_ms_to_ws_transform().location_to_world(self->turretTargetMS);
			if (enemyPlacement.is_active())
			{
				follow = true;
				targetAt = enemyPlacement.get_placement_in_owner_room().location_to_world(enemyTargetingOffsetOS);
				if (enemyPlacement.get_target())
				{
					canShootNow = true;
					timeToIdle = timeToGiveUpShooting;
				}
			}
			if (follow)
			{
				Vector3 newTargetMS = imo->get_appearance()->get_ms_to_ws_transform().location_to_local(targetAt);
				self->turretTargetMS = blend_to_using_speed(self->turretTargetMS, newTargetMS, 1.5f, LATENT_DELTA_TIME);
				float dist = (self->turretTargetMS - newTargetMS).length();
				self->turretTargetMS = newTargetMS + (self->turretTargetMS - newTargetMS).normal() * min(dist, 0.5f); // allow to lag behind
			}
			if (canShootNow)
			{
				if (consequtiveShots == 0 && timeToShoot < 0.0f && self->turretTargetingSocket.is_valid())
				{
					if (auto* a = imo->get_appearance())
					{
						Transform targettingSocketMS = a->calculate_socket_ms(self->turretTargetingSocket.get_index());
						float dotAtTarget = Vector3::dot(targettingSocketMS.get_axis(Axis::Forward), (self->turretTargetMS - targettingSocketMS.get_translation()).normal());
						if (dotAtTarget < 0.998f)
						{
							// not looking at the target
							canShootNow = false;
						}
					}
				}
				if (timeToShoot < 0.0f && canShootNow)
				{
					todo_note(TXT("check if shoot count is zero and is pointing at the target"));
					timeToShoot = max(projectileInterval, timeToShoot);
					if (auto* s = imo->get_sound())
					{
						if (s->play_sound(NAME(shoot)))
						{
							if (Framework::GameUtils::is_controlled_by_player(enemy.get()))
							{
								if (enemyPlacement.get_target())
								{
									MusicPlayer::request_combat_auto();
								}
								else
								{
									MusicPlayer::request_combat_auto_indicate_presence();
								}
							}
						}
					}
					if (auto* tos = imo->get_temporary_objects())
					{
						auto* projectile = tos->spawn(NAME(shoot));
						if (projectile)
						{
							// just in any case if we would be shooting from inside of a capsule
							if (auto* collision = projectile->get_collision())
							{
								collision->dont_collide_with_up_to_top_instigator(imo, 0.5f);
							}
							{
								float projectileSpeed = projectile->get_variables().get_value<float>(NAME(projectileSpeed), 10.0f);
								projectileSpeed = imo->get_variables().get_value<float>(NAME(projectileSpeed), projectileSpeed);
								Vector3 velocity = Vector3(0.0f, projectileSpeed, 0.0f);
								velocity = Utils::apply_shooting_accuracy(velocity, imo, enemyPlacement.get_target(), enemyPlacement.calculate_string_pulled_distance());
								projectile->on_activate_set_relative_velocity(velocity);
							}
							++ consequtiveShots;
						}
						tos->spawn_all(NAME(muzzleFlash));
					}
					if (consequtiveShots >= projectileCount)
					{
						consequtiveShots = 0;
						timeToShoot = projectileSeriesInterval;
					}
				}
			}
			else
			{
				if (timeToShootWhenDoesntSeeEnemy < 0.0f)
				{
					timeToShoot = max(projectileSeriesInterval, timeToShoot);
				}
				else
				{
					timeToShoot = max(timeToShootWhenDoesntSeeEnemy, timeToShoot);
				}
				consequtiveShots = 0;
			}
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Turret::execute_logic)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(Framework::InRoomPlacement, investigate);
	LATENT_COMMON_VAR(float, aggressiveness);
	LATENT_COMMON_VAR(bool, ignoreViolenceDisallowed);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, perceptionTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);

	LATENT_VAR(float, executionTime);

	aggressiveness = 99.0f;

	executionTime += LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	executionTime = 0.0f;

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::perception));
		perceptionTask.start_latent_task(mind, taskInfo);
	}

	if (auto* imo = mind->get_owner_as_modules_owner())
	{
		auto * self = fast_cast<Turret>(mind->get_logic());

		if (Name const * var = imo->get_variables().get_existing<Name>(NAME(turretIdleBehaviour)))
		{
			self->idleBehvaiour = *var;
		}
		if (Range const * var = imo->get_variables().get_existing<Range>(NAME(turretScanInterval)))
		{
			self->scanInterval = *var;
		}
		if (float const * var = imo->get_variables().get_existing<float>(NAME(turretScanInterval)))
		{
			self->scanInterval = Range(*var);
		}
		if (Range const * var = imo->get_variables().get_existing<Range>(NAME(turretScanStep)))
		{
			self->scanStep = *var;
		}
		if (float const * var = imo->get_variables().get_existing<float>(NAME(turretScanStep)))
		{
			self->scanStep = Range(*var);
		}
		if (Name const * var = imo->get_variables().get_existing<Name>(NAME(turretScanPOI)))
		{
			self->scanPOI = *var;
		}
		if (Name const * var = imo->get_variables().get_existing<Name>(NAME(turretTargetingSocket)))
		{
			self->turretTargetingSocket.set_name(*var);

			if (auto* a = imo->get_appearance())
			{
				self->turretTargetingSocket.look_up(a->get_mesh());
			}
		}
	}

	while (true)
	{
		{
			::Framework::AI::LatentTaskInfoWithParams nextTask;
			nextTask.propose(AI_LATENT_TASK_FUNCTION(remain_idle), 0);
			if (enemy.is_set() && enemyPlacement.is_active() && (enemyPlacement.get_target() || investigate.is_valid()) &&
				(! GameDirector::is_violence_disallowed() || ignoreViolenceDisallowed))
			{
				nextTask.propose(AI_LATENT_TASK_FUNCTION(attack_enemy), 5); // should stay in attack enemy
			}
			todo_hack(TXT("checking the execution time here is a hack to hide a turret that should appeared but should be hidden"));
			if (should_hide_disappear(mind, executionTime < 0.5f))
			{
				nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::hide_disappear), 10);
			}

			if (currentTask.can_start(nextTask))
			{
				an_assert(! currentTask.is_running(Tasks::hide_disappear));
				currentTask.start_latent_task(mind, nextTask);
			}
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//
	
	LATENT_ON_END();
	//
	perceptionTask.stop();
	currentTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}
