#include "aiLogic_cleanser.h"

#include "tasks\aiLogicTask_announcePresence.h"
#include "tasks\aiLogicTask_attackOrIdle.h"
#include "tasks\aiLogicTask_beingThrown.h"
#include "tasks\aiLogicTask_lookAt.h"
#include "tasks\aiLogicTask_perception.h"
#include "tasks\aiLogicTask_runAway.h"
#include "tasks\aiLogicTask_wander.h"

#include "..\aiCommon.h"
#include "..\aiCommonVariables.h"
#include "..\aiRayCasts.h"
#include "..\perceptionRequests\aipr_FindAnywhere.h"

#include "..\..\game\gameSettings.h"
#include "..\..\library\library.h"
#include "..\..\modules\custom\health\mc_health.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\music\musicPlayer.h"

#include "..\..\..\core\random\randomUtils.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiPerceptionRequest.h"
#include "..\..\..\framework\appearance\controllers\ac_particlesUtils.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\moduleController.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
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

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai messages
DEFINE_STATIC_NAME(die);

// ai message params
DEFINE_STATIC_NAME(location);
DEFINE_STATIC_NAME(throughDoor);
DEFINE_STATIC_NAME(room);
DEFINE_STATIC_NAME(who);

// sounds / temporary objects
DEFINE_STATIC_NAME_STR(postShoot, TXT("post shoot"));
DEFINE_STATIC_NAME(attack);
DEFINE_STATIC_NAME(needler);

// task feedback
DEFINE_STATIC_NAME(taskFeedback);
DEFINE_STATIC_NAME(enemyLost);

// tasks
DEFINE_STATIC_NAME(attackSequence);
DEFINE_STATIC_NAME(idleSequence);

// shader/material params
DEFINE_STATIC_NAME(needlerTime);

// emissive layers
DEFINE_STATIC_NAME(needlesAttack);

// variables
DEFINE_STATIC_NAME(useSpitPose);
DEFINE_STATIC_NAME(useSpitPoseArms);
DEFINE_STATIC_NAME(useSpitPoseHit);
DEFINE_STATIC_NAME(useStrikeRPose);
DEFINE_STATIC_NAME(useStrikeRPoseArm);
DEFINE_STATIC_NAME(useStrikeRPoseHit);
DEFINE_STATIC_NAME(arm_rf_animated);
DEFINE_STATIC_NAME(arm_lf_animated);
DEFINE_STATIC_NAME(arm_rf_off);
DEFINE_STATIC_NAME(arm_lf_off);
DEFINE_STATIC_NAME(arm_rb_off);
DEFINE_STATIC_NAME(arm_lb_off);
DEFINE_STATIC_NAME(upgradeCanisterId);
DEFINE_STATIC_NAME(canisterExtraUpgradeOptionsTagged);
DEFINE_STATIC_NAME(probCoef);
DEFINE_STATIC_NAME(spitDuration);

// temporary objects
DEFINE_STATIC_NAME(spit);
DEFINE_STATIC_NAME(dying);
DEFINE_STATIC_NAME(explode);

// movement gaits
DEFINE_STATIC_NAME(creep);

//

REGISTER_FOR_FAST_CAST(Cleanser);

Cleanser::Cleanser(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

Cleanser::~Cleanser()
{
}

void Cleanser::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	canister.content.exm.clear();
	canister.content.extra.clear();

	{
		TagCondition tc = _parameters.get_value<TagCondition>(NAME(canisterExtraUpgradeOptionsTagged), TagCondition());
		Array<ExtraUpgradeOption*> availableEUOs;
		availableEUOs = ExtraUpgradeOption::get_all(tc);
		if (!availableEUOs.is_empty())
		{
			Random::Generator rg;
			int idx = RandomUtils::ChooseFromContainer<Array<ExtraUpgradeOption*>, ExtraUpgradeOption*>::choose(
				rg, availableEUOs, [](ExtraUpgradeOption* _e) { return _e->get_tags().get_tag(NAME(probCoef), 1.0f); });
			canister.content.extra = availableEUOs[idx];
		}
		else
		{
			warn(TXT("cleanser has to extra upgrade available"));
		}
	}

	spitDuration = _parameters.get_value<float>(NAME(spitDuration), spitDuration);
}

void Cleanser::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	Framework::IModulesOwner* imo = nullptr;
	if (auto* mind = get_mind())
	{
		imo = mind->get_owner_as_modules_owner();
	}

	{
		float prevNeedlerActive = needlerActive;
		float currentTarget = max(needlerAttackActiveTarget, needlerActiveTarget);
		needlerActive = blend_to_using_time(needlerActive, currentTarget, currentTarget > needlerActive? 0.25f : 0.4f, _deltaTime);
		if (needlerActive < 0.1f && currentTarget == 0.0f)
		{
			needlerActive = 0.0f;
		}
		if (needlerActive > 0.0f && prevNeedlerActive == 0.0f)
		{
			if (imo)
			{
				if (auto* s = imo->get_sound())
				{
					needlerSound = s->play_sound(NAME(needler));
				}
			}
		}
		else if (needlerActive == 0.0f && prevNeedlerActive > 0.0f)
		{
			if (needlerSound.is_set())
			{
				needlerSound->stop();
				needlerSound.clear();
			}
		}

		if (needlerActive > 0.0f && needlerSound.is_set())
		{
			needlerSound->access_sample_instance().set_pitch(max(0.6f, 1.0f + (needlerActive - 1.0f) * 0.2f));
		}
	}

	needlerTime += needlerDir * needlerActive * _deltaTime;
	if (needlerTime > 60.0f)
	{
		needlerDir = -1.0f;
	}
	if (needlerTime < 0.5f)
	{
		needlerDir = 1.0f;
	}

	if (imo)
	{
		if (auto* a = imo->get_appearance())
		{
			auto& meshInst = a->access_mesh_instance();
			for_count(int, i, meshInst.get_material_instance_count())
			{
				if (auto* mat = meshInst.access_material_instance(i))
				{
					mat->set_uniform(NAME(needlerTime), needlerTime);
				}
			}
		}
		if (auto* emi = imo->get_custom<CustomModules::EmissiveControl>())
		{
			if (firstAdvance)
			{
				emi->emissive_deactivate_all();
			}
			bool currentEmissiveTarget = needlerAttackEmissiveTarget || needlerEmissiveTarget;
			if (needlerEmissive != currentEmissiveTarget)
			{
				needlerEmissive = currentEmissiveTarget;
				if (needlerEmissive)
				{
					emi->emissive_activate(NAME(needlesAttack));
				}
				else
				{
					emi->emissive_deactivate(NAME(needlesAttack));
				}
			}
		}
	}

	if (canisterGivenTime.is_set())
	{
		canisterGivenTime = canisterGivenTime.get() + _deltaTime;

		canister.set_active(false);
		canister.advance(_deltaTime);
		canister.force_emissive(UpgradeCanister::EmissiveUsed);
	}
	else
	{
		canister.set_active(true);
		canister.advance(_deltaTime);
		canister.force_emissive(NP);
	}

	if (canister.should_give_content() && !canisterGivenTime.is_set())
	{
		if (auto* pa = canister.get_held_by_owner())
		{
			canister.give_content(pa, false);
			canisterGivenTime = 0.0f;
			runAwayFrom = pa;
		}
	}

	firstAdvance = false;
}

LATENT_FUNCTION(Cleanser::do_attack)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("spit corrosion"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(bool, doSpitAttack);
	LATENT_VAR(SafePtr<::Framework::IModulesOwner>, spitParticle);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<Cleanser>(mind->get_logic());

	LATENT_BEGIN_CODE();

	{
		auto& locomotion = mind->access_locomotion();
		locomotion.stop();
		locomotion.turn_towards_2d(enemyPlacement.get_placement_in_owner_room().get_translation(), 5.0f);
	}

	doSpitAttack = true;// self->rg.get_chance(0.8f);

	if (doSpitAttack)
	{
		{
			self->useSpitPose.access<float>() = 1.0f;
			self->useSpitPoseArms.access<float>() = 0.0f;
			self->useSpitPoseHit.access<float>() = 0.0f;
			self->arm_lf_animated.access<bool>() = false;
			self->arm_rf_animated.access<bool>() = false;
		}
		self->needlerAttackActiveTarget = 3.0f;
		self->needlerAttackEmissiveTarget = true;
		LATENT_WAIT(0.2f);
		{
			self->useSpitPoseArms.access<float>() = 1.0f;
			self->arm_lf_animated.access<bool>() = true;
			self->arm_rf_animated.access<bool>() = true;
		}
		LATENT_WAIT(0.3f);
		{
			self->useSpitPose.access<float>() = 1.0f;
			self->useSpitPoseArms.access<float>() = 1.0f;
			self->useSpitPoseHit.access<float>() = 1.0f;
			self->arm_lf_animated.access<bool>() = true;
			self->arm_rf_animated.access<bool>() = true;
		}
		if (!self->canisterGivenTime.is_set())
		{
			if (auto* to = imo->get_temporary_objects())
			{
				spitParticle = to->spawn(NAME(spit));
			}
		}
		LATENT_WAIT(0.1f);
		{
			self->useSpitPose.access<float>() = 0.0f;
			self->useSpitPoseArms.access<float>() = 1.0f;
			self->useSpitPoseHit.access<float>() = 1.0f;
			self->arm_lf_animated.access<bool>() = true;
			self->arm_rf_animated.access<bool>() = true;
		}
		LATENT_WAIT(self->spitDuration - 0.1f);
		if (auto* sp = spitParticle.get())
		{
			::Framework::ParticlesUtils::desire_to_deactivate(sp);
			spitParticle.clear();
		}
		{
			self->useSpitPoseArms.access<float>() = 0.0f;
		}
		LATENT_WAIT(0.1f);
		{
			self->useSpitPose.access<float>() = 0.0f;
			self->useSpitPoseArms.access<float>() = 0.0f;
			self->useSpitPoseHit.access<float>() = 0.0f;
			self->arm_lf_animated.access<bool>() = false;
			self->arm_rf_animated.access<bool>() = false;
		}
		self->needlerAttackActiveTarget = 0.0f;
		self->needlerAttackEmissiveTarget = false;
		LATENT_WAIT(0.6f);
	}
	else
	{
		{
			self->useStrikeRPose.access<float>() = 0.0f;
			self->useStrikeRPoseArm.access<float>() = 1.0f;
			self->useStrikeRPoseHit.access<float>() = 0.0f;
			self->arm_rf_animated.access<bool>() = false;
		}
		LATENT_WAIT(0.2f);
		{
			self->useStrikeRPose.access<float>() = 1.0f;
			self->useStrikeRPoseArm.access<float>() = 1.0f;
			self->useStrikeRPoseHit.access<float>() = 0.0f;
			self->arm_rf_animated.access<bool>() = true;
		}
		LATENT_WAIT(0.3f);
		{
			self->useStrikeRPose.access<float>() = 1.0f;
			self->useStrikeRPoseArm.access<float>() = 1.0f;
			self->useStrikeRPoseHit.access<float>() = 1.0f;
			self->arm_rf_animated.access<bool>() = true;
		}
		LATENT_WAIT(0.1f);
		{
			todo_implement(TXT("do melee attack, find enemy, attack it"));
		}
		{
			self->useStrikeRPose.access<float>() = 0.0f;
			self->useStrikeRPoseArm.access<float>() = 1.0f;
			self->useStrikeRPoseHit.access<float>() = 0.0f;
			self->arm_rf_animated.access<bool>() = true;
		}
		LATENT_WAIT(0.2f);
		{
			self->useStrikeRPoseArm.access<float>() = 0.0f;
			self->arm_rf_animated.access<bool>() = true;
		}
		LATENT_WAIT(0.1f);
		{
			self->useStrikeRPose.access<float>() = 0.0f;
			self->useStrikeRPoseArm.access<float>() = 0.0f;
			self->useStrikeRPoseHit.access<float>() = 0.0f;
			self->arm_lf_animated.access<bool>() = false;
			self->arm_rf_animated.access<bool>() = false;
		}
		self->needlerAttackActiveTarget = 0.0f;
		self->needlerAttackEmissiveTarget = false;
		LATENT_WAIT(0.6f);
	}


	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	self->useSpitPose.access<float>() = 0.0f;
	self->useSpitPoseArms.access<float>() = 0.0f;
	self->useSpitPoseHit.access<float>() = 0.0f;
	self->useStrikeRPose.access<float>() = 0.0f;
	self->useStrikeRPoseHit.access<float>() = 0.0f;
	self->arm_rf_animated.access<bool>() = false;
	self->arm_lf_animated.access<bool>() = false;
	self->needlerAttackActiveTarget = 0.0f;
	self->needlerAttackEmissiveTarget = false;
	//
	if (auto* sp = spitParticle.get())
	{
		::Framework::ParticlesUtils::desire_to_deactivate(sp);
		spitParticle.clear();
	}

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Cleanser::attack_enemy)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("attack enemy"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(PerceptionLock, perceptionEnemyLock);
	LATENT_COMMON_VAR(Framework::InRoomPlacement, investigate);
	LATENT_COMMON_VAR(int, investigateIdx);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(int, isInvestigatingIdx);

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);

	LATENT_VAR(float, minDist);

	LATENT_VAR(bool, keepAttacking);
	LATENT_VAR(bool, justStarted);
	
	LATENT_VAR(::Framework::AI::LatentTaskHandle, attackActionTask);
	
	LATENT_VAR(::System::TimeStamp, timeInTask);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<Cleanser>(mind->get_logic());
	auto* npcBase = fast_cast<NPCBase>(mind->get_logic());
	an_assert(npcBase);

	LATENT_BEGIN_CODE();

	timeInTask.reset();

	thisTaskHandle->allow_to_interrupt(false); // can't be interrupted, we want to go where the enemy is and attack it

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("attack enemy (%S)"), enemy.get() ? enemy->ai_get_name().to_char() : TXT("??"));
	ai_log_no_colour(mind->get_logic());

	keepAttacking = false;
	isInvestigatingIdx = investigateIdx;

	if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
	{
		em->emissive_activate(NAME(attack));
	}

	minDist = 3.0f;

	if (auto* s = imo->get_sound())
	{
		if (s->play_sound(NAME(attack)))
		{
			if (Framework::GameUtils::is_controlled_by_player(enemy.get()))
			{
				if (!npcBase || npcBase->is_ok_to_play_combat_music(enemyPlacement))
				{
					MusicPlayer::request_combat_auto_indicate_presence();
				}
			}
		}
	}

	self->set_movement_gait();
	keepAttacking = true;
	justStarted = true;
	while (keepAttacking || timeInTask.get_time_since() < 2.0f) // switch no often than 2 seconds
	{
		LATENT_CLEAR_LOG();

		if (! pathTask.is_set() && enemyPlacement.is_active())
		{
			auto& locomotion = mind->access_locomotion();
			if (justStarted || locomotion.is_done_with_move() || locomotion.has_finished_move(1.0f))
			{
				if (enemyPlacement.get_target())
				{
					// sees enemy
					LATENT_LOG(TXT("I see enemy"));
				}
				else
				{
					// follow enemy
					LATENT_LOG(TXT("not seeing enemy"));
				}

				Framework::Nav::PlacementAtNode ourNavLoc = mind->access_navigation().find_nav_location();
				Framework::Nav::PlacementAtNode toEnemyNavLoc = Framework::Nav::PlacementAtNode::invalid();

				if (enemyPlacement.get_through_doors().is_empty())
				{
					// same room
					LATENT_LOG(TXT("same room - check actual location"));
					toEnemyNavLoc = mind->access_navigation().find_nav_location(enemyPlacement.get_in_final_room(), enemyPlacement.get_placement_in_final_room());
				}
				else
				{
					// get door through which enemy is available
					if (auto* dir = enemyPlacement.get_through_doors().get_first().get())
					{
						if (auto* nav = dir->get_nav_door_node().get())
						{
							LATENT_LOG(TXT("different room, consider first door from the chain/path"));
							toEnemyNavLoc.set(nav);
						}
						else
						{
							LATENT_LOG(TXT("different room, get close to first door although can't get through"));
							Transform dirPlacement = dir->get_placement();
							Transform inFront(Vector3::yAxis * -0.3f, Quat::identity);
							toEnemyNavLoc = mind->access_navigation().find_nav_location(dir->get_in_room(), dirPlacement.to_world(inFront));
						}
					}
				}

				if (ourNavLoc.is_valid() && toEnemyNavLoc.is_valid())
				{
					LATENT_LOG(TXT("we know where enemy is on navmesh"));
					Framework::Room* goToRoom = imo->get_presence()->get_in_room();
					Framework::DoorInRoom* goThroughDoor = nullptr;
					Vector3 goToLoc = toEnemyNavLoc.get_current_placement().get_translation();
					Vector3 startLoc = imo->get_presence()->get_placement().get_translation();

					bool tryToApproach = false;

					// check if door-through or enemy belong to the same nav group that we are and decide whether to follow to enemy or stay where we are
					if (ourNavLoc.node.get()->get_group() == toEnemyNavLoc.node.get()->get_group())
					{
						LATENT_LOG(TXT("same nav group"));
						// same group
						if (enemyPlacement.get_through_doors().is_empty())
						{
							// is within same room we see enemy or we do not see enemy's location
							LATENT_LOG(TXT("approach!"));
							tryToApproach = true;
							goToRoom = enemyPlacement.get_in_final_room();
							goToLoc = enemyPlacement.get_placement_in_final_room().get_translation();
						}
						else
						{
							if (enemyPlacement.get_target())
							{
								// we see enemy, approach him wherever he is
								LATENT_LOG(TXT("we see enemy, go to the enemy's room"));
								tryToApproach = true;
								goToRoom = enemyPlacement.get_in_final_room();
								goToLoc = enemyPlacement.get_placement_in_final_room().get_translation();
								startLoc = enemyPlacement.location_from_owner_to_target(startLoc);
							}
							else
							{
								LATENT_LOG(TXT("we do not see enemy, go to the door, on the other side"));
								// we don't see enemy and enemy is in different room, just go to the door and see what will happen
								goThroughDoor = enemyPlacement.get_through_doors().get_first().get();
							}
						}
					}
					else
					{
						LATENT_LOG(TXT("different nav group, try to approach if possible"));
						// remain here, within this group, try to approach enemy
						tryToApproach = true;
						// keep "goTo"s
						todo_note(TXT("check chances if maybe we should try to find a way to the enemy, break this function then and request following enemy"));
					}

					Framework::Nav::PathRequestInfo pathRequestInfo(imo);
					pathRequestInfo.with_dev_info(TXT("cleanser ?"));
					if (!Mobility::may_leave_room_when_attacking(imo, imo->get_presence()->get_in_room()))
					{
						pathRequestInfo.within_same_room_and_navigation_group();
					}

					if (goThroughDoor)
					{
						pathTask = mind->access_navigation().find_path_through(goThroughDoor, 0.5f, pathRequestInfo);
					}
					else
					{
						if (tryToApproach)
						{
							Vector3 enemyLoc = goToLoc;
							Vector3 startToEnemy = enemyLoc - startLoc;
							goToLoc = startLoc + startToEnemy * (startToEnemy.length() + 0.5f);
							Vector3 enemyToGoTo = goToLoc - enemyLoc;
							goToLoc = enemyLoc + enemyToGoTo.normal() * max(minDist, enemyToGoTo.length());
						}
						pathTask = mind->access_navigation().find_path_to(goToRoom, goToLoc, pathRequestInfo);
					}
				}
			}
		}

		justStarted = false;

		if (pathTask.is_set())
		{
			LATENT_NAV_TASK_WAIT_IF_SUCCEED(pathTask)
			{
				if (auto* task = fast_cast<Framework::Nav::Tasks::PathTask>(pathTask.get()))
				{
					path = task->get_path();
				}
				else
				{
					path.clear();
				}
			}
			else
			{
				path.clear();
			}
			pathTask = nullptr;
		}

	MOVE:
		{
			if (enemyPlacement.is_active())
			{
				if (auto* t = enemyPlacement.get_target())
				{
					// calc_distance_from_primitive_os ?
					Vector3 rel = enemyPlacement.get_target_centre_relative_to_owner();
					if (rel.z >= -0.2f && rel.z <= 1.5f &&
						abs(rel.x) < 0.3f &&
						rel.y <= 1.0f)
					{
						goto ATTACK;
					}
				}
				{
					auto placementInOwner = enemyPlacement.get_placement_in_owner_room();
					// calc_distance_from_primitive_os ?
					Vector3 rel = imo->get_presence()->get_placement().location_to_local(placementInOwner.get_translation());
					if (rel.z >= -0.4f && rel.z <= 2.4f &&
						abs(rel.x) < 0.3f &&
						rel.y <= 1.0f)
					{
						goto ATTACK;
					}
				}
			}
			{
				auto& locomotion = mind->access_locomotion();
				if (! locomotion.check_if_path_is_ok(path.get()))
				{
					locomotion.dont_move();
				}
				else if (path.get() && !path->is_close_to_the_end(imo->get_presence()->get_in_room(), imo->get_presence()->get_placement().get_translation(), 0.25f))
				{
					if (!locomotion.is_following_path(path.get()))
					{
						Framework::MovementParameters mp;
						mp.full_speed();
						if (auto* npcBase = fast_cast<NPCBase>(mind->get_logic()))
						{
							if (npcBase->get_movement_gait().is_valid())
							{
								mp.gait(npcBase->get_movement_gait());
							}
						}
						locomotion.follow_path_2d(path.get(), NP, mp);
						locomotion.turn_follow_path_2d();
					}
				}
				else
				{
					locomotion.dont_move();
				}
			}

			{
				auto& locomotion = mind->access_locomotion();
				if (locomotion.has_finished_move(0.5f))
				{
					LATENT_LOG(TXT("finished movement"));
					locomotion.dont_move();
					{
						keepAttacking = false;
						enemyPlacement.clear_target();
						investigate.clear();
						++investigateIdx;
					}
					goto MOVED;
				}
				if (investigateIdx != isInvestigatingIdx)
				{
					LATENT_LOG(TXT("changed investigation target"));
					locomotion.dont_move();
					{
						keepAttacking = false;
						// keep investigate
					}
					goto MOVED;
				}
			}

			LATENT_WAIT(self->rg.get_float(0.1f, 0.3f));
		}
		goto MOVE;

	ATTACK:
		perceptionEnemyLock.override_lock(NAME(attack), AI::PerceptionLock::Lock);
		{
			LATENT_LOG(TXT("attack spit, maybe melee?"));
			{
				::Framework::AI::LatentTaskInfoWithParams taskInfo;
				taskInfo.clear();
				taskInfo.propose(AI_LATENT_TASK_FUNCTION(do_attack));
				attackActionTask.start_latent_task(mind, taskInfo);
			}
			while (attackActionTask.is_running())
			{
				LATENT_YIELD();
			}
		}
		{
			keepAttacking = false;
			enemyPlacement.clear_target();
			auto nav = mind->access_navigation().find_nav_location(imo, Vector3(0.0f, 1.0f, 0.0f));
			if (nav.is_valid())
			{
				// investigate forward
				investigate = Framework::InRoomPlacement(nav.get_room(), nav.get_current_placement());
				++investigateIdx;
			}
			else
			{
				investigate.clear();
				++investigateIdx;
			}
		}
		perceptionEnemyLock.override_lock(NAME(attack));

	MOVED:
		{
			LATENT_YIELD();
		}
	}
	// ...

	LATENT_WAIT(self->rg.get_float(0.1f, 0.2f));

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	perceptionEnemyLock.override_lock(NAME(attack));
	// don't stop locomotion
	/*
	if (mind)
	{
		auto& locomotion = mind->access_locomotion();
		locomotion.stop();
	}
	*/
	attackActionTask.stop();
	if (imo)
	{
		if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
		{
			em->emissive_deactivate(NAME(attack));
		}
	}
	self->set_movement_gait(NAME(creep));

	thisTaskHandle->allow_to_interrupt(true);

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Cleanser::needler_idling)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("needler idling"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	//auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<Cleanser>(mind->get_logic());

	LATENT_BEGIN_CODE();

	while (true)
	{
		self->needlerEmissiveTarget = false;
		self->needlerActiveTarget = 0.0f;
		LATENT_WAIT(self->rg.get_float(3.0f, 10.0f));
		self->needlerActiveTarget = 3.0f;
		LATENT_WAIT(self->rg.get_float(0.2f, 0.6f));
	}

	LATENT_ON_BREAK();
	//
	
	LATENT_ON_END();
	//
	self->needlerActiveTarget = 0.0f;
	self->needlerEmissiveTarget = false;

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Cleanser::death_sequence)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai cleanser] death sequence"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("death sequence"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::IModulesOwner*, imo);
	LATENT_VAR(SafePtr<Framework::IModulesOwner>, dying);
	LATENT_VAR(::System::TimeStamp, timePassed);
	LATENT_VAR(SafePtr<Framework::IModulesOwner>, mostLikelyDeathInstigator);
	
	LATENT_VAR(float, timeLimit);
	LATENT_VAR(float, rotateDir);
	
	LATENT_VAR(Random::Generator, rg);

	auto* self = fast_cast<Cleanser>(mind->get_logic());

	LATENT_BEGIN_CODE();

	imo = mind->get_owner_as_modules_owner();

	ai_log(mind->get_logic(), TXT("death sequence"));

	if (auto* h = imo->get_custom<CustomModules::Health>())
	{
		mostLikelyDeathInstigator = h->get_last_damage_instigator();
	}
	
	self->needlerAttackActiveTarget = 6.0f;
	self->needlerAttackEmissiveTarget = false;

	if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
	{
		em->emissive_deactivate_all();
	}

	if (auto* tos = imo->get_temporary_objects())
	{
		dying = tos->spawn(NAME(spit));
	}

	if (self->canisterGivenTime.is_set())
	{
		timeLimit = 5.0f;
	}
	else
	{
		timeLimit = 0.5f;
	}

	timePassed.reset();
	rotateDir = (rg.get_chance(0.5f) ? 1.0f : -1.0f);
	while (timePassed.get_time_since() < timeLimit)
	{
		{
			auto& locomotion = mind->access_locomotion();
			locomotion.stop();
			locomotion.dont_control();
			if (auto* c = imo->get_controller())
			{
				c->set_requested_relative_velocity_orientation(Rotator3(0.0f, rg.get_float(0.6f, 1.6f) /* a bit faster than 100% */ * rotateDir, 0.0f));
			}
		}

		LATENT_WAIT(min(rg.get_float(0.2f, 0.6f), max(0.01f, timeLimit - timePassed.get_time_since())));

		rotateDir = -rotateDir;
	}

	timePassed.reset();

	if (auto* tos = imo->get_temporary_objects())
	{
		Vector3 explosionLocOS = imo->get_presence()->get_random_point_within_presence_os(0.8f);
		Transform explosionWS = imo->get_presence()->get_placement().to_world(Transform(explosionLocOS, Quat::identity));
		Framework::Room* inRoom;
		imo->get_presence()->move_through_doors(explosionWS, OUT_ inRoom);
		tos->spawn_in_room(NAME(explode), inRoom, explosionWS);
	}

	ai_log(mind->get_logic(), TXT("perform death"));
	if (auto* h = imo->get_custom<CustomModules::Health>())
	{
		h->override_death_effects(false, 0.05f);
		Framework::IModulesOwner* deathInstigator = mostLikelyDeathInstigator.get();
		if (auto* di = self->runAwayFrom.get())
		{
			// if we were running away from someone, most likely it was the player pulling canister
			deathInstigator = di;
		}
		h->setup_death_params(false, false, deathInstigator);
		h->perform_death(deathInstigator);
	}

	LATENT_ON_BREAK();
	//
	if (mind)
	{
		::Framework::AI::Locomotion& locomotion = mind->access_locomotion();
		locomotion.stop();
	}

	LATENT_ON_END();
	//
	if (dying.is_set())
	{
		if (auto* to = fast_cast<Framework::TemporaryObject>(dying.get()))
		{
			to->desire_to_deactivate();
		}
		dying.clear();
	}

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Cleanser::execute_logic)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Name, scriptedBehaviour);
	LATENT_COMMON_VAR(float, aggressiveness);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, needlerTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, announcePresenceTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, perceptionTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	LATENT_VAR(bool, dieNow);
	LATENT_VAR(bool, runAwayNow);
	
	LATENT_VAR(Random::Generator, rg);

	aggressiveness = 99.0f;

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<Cleanser>(mind->get_logic());

	LATENT_BEGIN_CODE();

	{
		self->useSpitPose = NAME(useSpitPose);
		self->useSpitPoseArms = NAME(useSpitPoseArms);
		self->useSpitPoseHit = NAME(useSpitPoseHit);
		self->useStrikeRPose = NAME(useStrikeRPose);
		self->useStrikeRPoseArm = NAME(useStrikeRPoseArm);
		self->useStrikeRPoseHit = NAME(useStrikeRPoseHit);
		self->arm_rf_animated = NAME(arm_rf_animated);
		self->arm_lf_animated = NAME(arm_lf_animated);

		auto& varStorage = mind->get_owner_as_modules_owner()->access_variables();
		self->useSpitPose.look_up<float>(varStorage);
		self->useSpitPoseArms.look_up<float>(varStorage);
		self->useSpitPoseHit.look_up<float>(varStorage);
		self->useStrikeRPose.look_up<float>(varStorage);
		self->useStrikeRPoseArm.look_up<float>(varStorage);
		self->useStrikeRPoseHit.look_up<float>(varStorage);
		self->arm_rf_animated.look_up<bool>(varStorage);
		self->arm_lf_animated.look_up<bool>(varStorage);
	}

	messageHandler.use_with(mind);
	{
		auto* framePtr = &_frame;
		messageHandler.set(NAME(die), [framePtr, mind, &dieNow](Framework::AI::Message const& _message)
			{
				dieNow = true;
				framePtr->end_waiting();
				AI_LATENT_STOP_LONG_RARE_ADVANCE();
			}
		);
	}

	if (ShouldTask::announce_presence(mind->get_owner_as_modules_owner()))
	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::announce_presence));
		announcePresenceTask.start_latent_task(mind, taskInfo);
	}

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::perception));
		perceptionTask.start_latent_task(mind, taskInfo);
	}

	if (auto* logicLatent = fast_cast<::Framework::AI::LogicWithLatentTask>(logic))
	{
		logicLatent->register_latent_task(NAME(attackSequence), attack_enemy);
		logicLatent->register_latent_task(NAME(idleSequence), Tasks::wander);
	}

	if (imo)
	{
		imo->get_presence()->request_on_spawn_random_dir_teleport(4.5f, 0.9f);
	}

	self->set_movement_gait(NAME(creep));

	{
		self->canister.initialise(imo, NAME(upgradeCanisterId));
		self->canister.update_emissive(UpgradeCanister::EmissiveOff, true);
	}

	while (true)
	{
		// check if we should die
		{
			{
				int legsOff = 0;
				legsOff += imo->get_variables().get_value<bool>(NAME(arm_rf_off), 0) ? 1 : 0;
				legsOff += imo->get_variables().get_value<bool>(NAME(arm_lf_off), 0) ? 1 : 0;
				legsOff += imo->get_variables().get_value<bool>(NAME(arm_rb_off), 0) ? 1 : 0;
				legsOff += imo->get_variables().get_value<bool>(NAME(arm_lb_off), 0) ? 1 : 0;
				if (legsOff >= 1)
				{
					runAwayNow = true;
				}
				if (legsOff >= 2)
				{
					dieNow = true;
				}
			}
			if (self->canisterGivenTime.is_set())
			{
				dieNow = true;
			}
		}
		if (!needlerTask.is_running())
		{
			::Framework::AI::LatentTaskInfoWithParams nextTask;
			nextTask.propose(AI_LATENT_TASK_FUNCTION(needler_idling));
			if (needlerTask.can_start(nextTask))
			{
				needlerTask.start_latent_task(mind, nextTask);
			}
		}
		{
			::Framework::AI::LatentTaskInfoWithParams nextTask;
			bool specialTask = false;
			{
				nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::attack_or_idle));
			}
			if (dieNow)
			{
				if (nextTask.propose(AI_LATENT_TASK_FUNCTION(death_sequence), 100, NP, false, true))
				{
					specialTask = true;
				}
			}
			if (ShouldTask::being_thrown(imo))
			{
				if (nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::being_thrown), 20, NP, NP, true))
				{
					specialTask = true;
				}
			}
			if (runAwayNow)
			{
				if (nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::run_away), 10, NP, NP, true))
				{
					if (!currentTask.is_running(AI_LATENT_TASK_FUNCTION(Tasks::run_away)))
					{
						self->set_movement_gait();
					}
					if (!self->runAwayFrom.is_set())
					{
						if (auto* h = imo->get_custom<CustomModules::Health>())
						{
							self->runAwayFrom = h->get_last_damage_instigator();
						}
					}
					ADD_LATENT_TASK_INFO_PARAM(nextTask, SafePtr<::Framework::IModulesOwner>, self->runAwayFrom);
					ADD_LATENT_TASK_INFO_PARAM(nextTask, bool, true); // keep running away
				}
			}
			if (!specialTask &&
				Common::handle_scripted_behaviour(mind, scriptedBehaviour, currentTask))
			{
				// doing scripted behaviour
			}
			else
			{
				// check if we can start new one, if so, start it
				if (currentTask.can_start(nextTask))
				{
					currentTask.start_latent_task(mind, nextTask);
				}
			}
		}
		LATENT_WAIT(rg.get_float(0.1f, 0.5f));
	}

	LATENT_ON_BREAK();
	//
	
	LATENT_ON_END();
	//
	needlerTask.stop();
	announcePresenceTask.stop();
	perceptionTask.stop();
	currentTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}
