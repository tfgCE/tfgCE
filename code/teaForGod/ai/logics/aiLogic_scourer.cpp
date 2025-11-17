#include "aiLogic_scourer.h"

#include "actions\aiAction_findPath.h"

#include "tasks\aiLogicTask_announcePresence.h"
#include "tasks\aiLogicTask_lookAt.h"
#include "tasks\aiLogicTask_perception.h"
#include "tasks\aiLogicTask_wander.h"

#include "..\..\game\gameDirector.h"
#include "..\..\game\gameSettings.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\health\mc_health.h"
#include "..\..\modules\gameplay\moduleProjectile.h"
#include "..\..\music\musicPlayer.h"

#include "..\..\teaForGod.h"

#include "..\..\..\framework\ai\aiMindInstance.h"

#include "..\aiCommon.h"
#include "..\aiCommonVariables.h"
#include "..\aiRayCasts.h"

#include "..\..\..\framework\framework.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\appearance\controllers\ac_particlesUtils.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\module\custom\mc_lightSources.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\nav\navPath.h"
#include "..\..\..\framework\nav\navTask.h"
#include "..\..\..\framework\nav\navLatentUtils.h"
#include "..\..\..\framework\nav\tasks\navTask_PathTask.h"
#include "..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\framework\world\doorInRoom.h"
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

// ai message
DEFINE_STATIC_NAME(remoteControl);

// params
DEFINE_STATIC_NAME(shootCount);
DEFINE_STATIC_NAME(maxDeflectDistance);
DEFINE_STATIC_NAME(deflectionAcceleration);
DEFINE_STATIC_NAME(deflectionSlowDownAcceleration);
DEFINE_STATIC_NAME(deflectedProjectileSpeed);

// emissive layers
DEFINE_STATIC_NAME(attack);
DEFINE_STATIC_NAME(deflectorOn);

// temporary objects
DEFINE_STATIC_NAME(deflect);
DEFINE_STATIC_NAME(shoot);
DEFINE_STATIC_NAME(muzzleFlash);

// task feedback
DEFINE_STATIC_NAME(taskFeedback);
DEFINE_STATIC_NAME(enemyLost);

// room tags
DEFINE_STATIC_NAME(transportRoom);

// appearance controller variables
DEFINE_STATIC_NAME(deflectorActive);
DEFINE_STATIC_NAME(relativeTargetLoc);

//

REGISTER_FOR_FAST_CAST(Scourer);

Scourer::Scourer(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

Scourer::~Scourer()
{
}

void Scourer::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	shootCount = _parameters.get_value<int>(NAME(shootCount), shootCount);

	maxDeflectDistance = _parameters.get_value<float>(NAME(maxDeflectDistance), maxDeflectDistance);
	deflectionAcceleration = _parameters.get_value<float>(NAME(deflectionAcceleration), deflectionAcceleration);
	deflectionSlowDownAcceleration = _parameters.get_value<float>(NAME(deflectionSlowDownAcceleration), deflectionSlowDownAcceleration);
	deflectedProjectileSpeed = _parameters.get_value<float>(NAME(deflectedProjectileSpeed), deflectedProjectileSpeed);

	muzzleSockets.clear();
	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		if (auto* a = imo->get_appearance())
		{
			for (int idx = 0;; ++idx)
			{
				Name muzzleSocket = Name(String::printf(TXT("muzzle_%i"), idx));
				if (a->has_socket(muzzleSocket))
				{
					muzzleSockets.push_back(muzzleSocket);
				}
				else
				{
					break;
				}
			}
		}
	}
}

LATENT_FUNCTION(Scourer::deflect)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("deflect"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(float, timeToSwitchDeflect);
	LATENT_VAR(bool, deflectActive);
	LATENT_VAR(bool, deflectEmissiveActive);
	LATENT_VAR(float, timeSinceShooting);
	LATENT_VAR(float, timeSinceEnemySeen);
	LATENT_VAR(SafePtr<Framework::IModulesOwner>, deflectParticles);
	LATENT_VAR(Energy, currHealth);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<Scourer>(mind->get_logic());

	timeToSwitchDeflect -= LATENT_DELTA_TIME;
	timeSinceShooting = self->shootingNow? 0.0f : timeSinceShooting + LATENT_DELTA_TIME;
	timeSinceEnemySeen += LATENT_DELTA_TIME;

	if (enemyPlacement.get_target())
	{
		timeSinceEnemySeen = 0.0f;
	}

	LATENT_BEGIN_CODE();

	timeToSwitchDeflect = 0.0f;
	deflectEmissiveActive = false;
	timeSinceShooting = 100.0f;
	timeSinceEnemySeen = 100.0f;

	messageHandler.use_with(mind);
	{
		auto* framePtr = &_frame;
		messageHandler.set(NAME(remoteControl), [framePtr, mind, &timeToSwitchDeflect, self](Framework::AI::Message const& _message)
			{
				// force switch off deflection
				timeToSwitchDeflect = max(timeToSwitchDeflect, REMOTE_CONTROL_EFFECT_TIME);
				self->deflectActiveGeneral = false;

				framePtr->end_waiting();
				AI_LATENT_STOP_LONG_RARE_ADVANCE();
			}
		);
	}

	while (true)
	{
		LATENT_CLEAR_LOG();

		if (auto* h = imo->get_custom<CustomModules::Health>())
		{
			Energy ch = h->get_health();
			if (ch != currHealth)
			{
				currHealth = ch;
				self->deflectActiveGeneral = true;
				timeToSwitchDeflect = max(1.5f, timeToSwitchDeflect);
			}
		}

		if (timeToSwitchDeflect <= 0.0f)
		{
			self->deflectActiveGeneral = !self->deflectActiveGeneral;
			if (self->deflectActiveGeneral)
			{
				timeToSwitchDeflect = Random::get_float(1.0f, 4.0f); // switch deflection randomly, this should make it harder
			}
			else
			{
				timeToSwitchDeflect = Random::get_float(1.0f, 3.0f);
			}
		}

		deflectActive = (self->deflectActiveGeneral && timeSinceEnemySeen <= 20.0f);// && (timeSinceShooting > 1.0f);
#ifdef AN_DEVELOPMENT
		if (Framework::is_preview_game())
		{
			deflectActive = self->deflectActiveGeneral;
		}
#endif

		imo->get_appearance()->access_controllers_variable_storage().access<float>(NAME(deflectorActive)) = deflectActive ? 1.0f : 0.0f;

		if (deflectEmissiveActive != deflectActive)
		{
			deflectEmissiveActive = deflectActive;
			if (auto* e = imo->get_custom<CustomModules::EmissiveControl>())
			{
				if (deflectActive)
				{
					e->emissive_activate(NAME(deflectorOn));
				}
				else
				{
					e->emissive_deactivate(NAME(deflectorOn));
				}
			}
		}

		if (deflectActive)
		{
			if (! deflectParticles.get())
			{
				if (auto* to = imo->get_temporary_objects())
				{
					deflectParticles = to->spawn(NAME(deflect));
				}
			}
			{
				Vector3 imoCentreWS = imo->get_presence()->get_centre_of_presence_WS();
				ARRAY_STACK(Framework::FoundRoom, rooms, 16);
				Framework::FoundRoom::find(rooms, imo->get_presence()->get_in_room(), imoCentreWS,
					Framework::FindRoomContext().only_visible().with_depth_limit(6).with_max_distance(self->maxDeflectDistance));
				for_every(foundRoom, rooms)
				{
					Transform foundRoomTransform = Transform::identity;
					auto* goUp = foundRoom;
					while (goUp && goUp->enteredThroughDoor)
					{
						foundRoomTransform = goUp->enteredThroughDoor->get_door_on_other_side()->get_other_room_transform().to_world(foundRoomTransform);
						goUp = goUp->enteredFrom;
					}
					float sqrMaxDistance = sqr(self->maxDeflectDistance);
					//FOR_EVERY_TEMPORARY_OBJECT_IN_ROOM(to, foundRoom->room)
					for_every_ptr(to, foundRoom->room->get_temporary_objects())
					{
						if (auto* mp = to->get_gameplay_as<ModuleProjectile>())
						{
							auto* top = to->get_presence();
							if (top && mp->is_deflectable() && to->get_top_instigator() != imo) // doesn't deflect own shots
							{
								Vector3 locWS = foundRoomTransform.location_to_world(top->get_placement().get_translation());
								Vector3 toLocWS = locWS - imoCentreWS;
								if (Vector3::dot(toLocWS, top->get_velocity_linear()) < 0.0f) // only incoming
								{
									//debug_push_filter(alwaysDraw);
									//debug_context(pilgrimOwner->get_presence()->get_in_room());
									//debug_draw_line(true, Colour::green, pilgrimCentreWS, locWS);
									//debug_no_context();
									//debug_pop_filter();
									float distanceSquared = toLocWS.length_squared();
									if (distanceSquared < sqrMaxDistance)
									{
										Vector3 velocityImpulse = Vector3::zero;
										float distance = sqrt(distanceSquared);
										float strength = (1.0f - distance / self->maxDeflectDistance) * 5.0f * (1.0f - mp->get_anti_deflection());
										strength = clamp(strength, 0.0f, 1.0f);
										{
											//Vector3 velWS = foundRoomTransform.vector_to_world(top->get_velocity_linear());
											Vector3 towardsProjectileOSXY = imo->get_presence()->get_placement().vector_to_local(toLocWS).normal_2d();
											Vector3 deflectDirOS = towardsProjectileOSXY.rotated_right();
											Vector3 deflectionWS = imo->get_presence()->get_placement().vector_to_world(deflectDirOS);
											Vector3 deflectionOWS = foundRoomTransform.vector_to_local(deflectionWS); // back to original one
											// if armour is ineffective, deflection does work a tiny bit
											if (GameSettings::get().difficulty.armourIneffective)
											{
												strength *= 0.1f;
											}
											float acceleration = self->deflectionAcceleration * clamp(strength, 0.0f, 1.0f);
											velocityImpulse += deflectionOWS * acceleration * LATENT_DELTA_TIME;
										}
										{
											float speed = top->get_velocity_linear().length();
											if (speed > self->deflectedProjectileSpeed)
											{
												// slow down and turn
												float acceleration = self->deflectionSlowDownAcceleration * clamp(strength, 0.0f, 1.0f);
												velocityImpulse += - top->get_velocity_linear().normal() * acceleration * LATENT_DELTA_TIME;
											}
										}
										top->add_velocity_impulse(velocityImpulse);
									}
								}
							}
						}
					}
				}
			}
			LATENT_YIELD();
		}
		else
		{
			if (auto* dp = deflectParticles.get())
			{
				Framework::ParticlesUtils::desire_to_deactivate(dp);
				deflectParticles.clear();
			}

			LATENT_WAIT(Random::get_float(0.05f, 0.2f));
		}
	}

	LATENT_ON_BREAK();
	//
	
	LATENT_ON_END();
	//
	Framework::ParticlesUtils::desire_to_deactivate(deflectParticles.get());

	LATENT_END_CODE();

	LATENT_RETURN();
}

LATENT_FUNCTION(Scourer::scour)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("scour"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(bool, ignoreViolenceDisallowed);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(bool, resetShoots);
	LATENT_VAR(float, timeSinceEnemySeen);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<Scourer>(mind->get_logic());
	auto* npcBase = self;

	timeSinceEnemySeen += LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	resetShoots = true;
	timeSinceEnemySeen = 0.0f;

	if (auto* e = imo->get_custom<CustomModules::EmissiveControl>())
	{
		e->emissive_activate(NAME(attack));
	}

	while (true)
	{
		LATENT_CLEAR_LOG();

		if (resetShoots)
		{
			self->shootingNow = false;
			resetShoots = false;
			if (self->shootFromMuzzles.get_size() < self->muzzleSockets.get_size())
			{
				self->shootFromMuzzles.clear();
				for_every(m, self->muzzleSockets)
				{
					self->shootFromMuzzles.push_back(*m);
				}
				self->shootFromMuzzlesLeft = min(self->shootFromMuzzles.get_size(), self->shootCount);
			}
			if (auto* s = imo->get_sound())
			{
				if (s->play_sound(NAME(attack)))
				{
					if (Framework::GameUtils::is_controlled_by_player(enemyPlacement.get_target()))
					{
						if (!npcBase || npcBase->is_ok_to_play_combat_music(enemyPlacement))
						{
							MusicPlayer::request_combat_auto_indicate_presence();
						}
					}
				}
			}
		}
		if (enemyPlacement.get_target())
		{
			timeSinceEnemySeen = 0.0f;
		}
		if (!self->shootFromMuzzles.is_empty() &&
			self->shootFromMuzzlesLeft > 0)
		{
			if (enemyPlacement.is_active())
			{
				bool shootAllowed = false;
				if (!GameDirector::is_violence_disallowed() || ignoreViolenceDisallowed)
				{
					if (self->lookAtBlocked)
					{
						shootAllowed = true;
					}
					else
					{
						Vector3 relativeLoc = imo->get_appearance()->access_controllers_variable_storage().access<Vector3>(NAME(relativeTargetLoc));
						if (abs(relativeLoc.x) < 0.2f && relativeLoc.y >= 0.0f && abs(relativeLoc.to_rotator().yaw) < 10.0f)
						{
							shootAllowed = true;
						}
					}
				}

				bool shootNow = false;
				if (!shootNow && shootAllowed)
				{
					shootNow |= enemyPlacement.calculate_string_pulled_distance() < 6.0f || timeSinceEnemySeen < 6.0f;
				}
				if (!shootNow && shootAllowed)
				{
					if (imo->get_presence()->get_in_room()->get_tags().get_tag(NAME(transportRoom)) != 0.0f)
					{
						shootNow = true;
					}
					else
					{
						auto& throughDoors = enemyPlacement.get_through_doors();
						if (!throughDoors.is_empty())
						{
							if (auto* r = throughDoors.get_first()->get_world_active_room_on_other_side())
							{
								if (r->get_tags().get_tag(NAME(transportRoom)) != 0.0f)
								{
									shootNow = true;
								}
							}
						}
					}
				}
				if (shootNow && shootAllowed)
				{
					self->shootingNow = true;
					int shotsNow = max(2, self->shootFromMuzzlesLeft / 2);
					bool shotAny = false;
					while (!self->shootFromMuzzles.is_empty() && self->shootFromMuzzlesLeft > 0 && shotsNow > 0)
					{
						if (auto* tos = imo->get_temporary_objects())
						{
							int idx = Random::get_int(self->shootFromMuzzles.get_size());
							auto* projectile = tos->spawn_in_room(NAME(shoot), Framework::ModuleTemporaryObjects::SpawnParams().at_socket(self->shootFromMuzzles[idx]));
							if (projectile)
							{
								// just in any case if we would be shooting from inside of a capsule
								if (auto* collision = projectile->get_collision())
								{
									collision->dont_collide_with_up_to_top_instigator(imo, 0.5f);
								}
								todo_note(TXT("hardcoded"));
								{
									Vector3 velocity = Vector3(0.0f, 10.0f, 0.0f);
									float dispersionAngleDeg = 10.0f;
									Quat dispersionAngle = Rotator3(0.0f, 0.0f, Random::get_float(-180.0f, 180.0f)).to_quat().to_world(Rotator3(Random::get_float(0.0f, dispersionAngleDeg), 0.0f, 0.0f).to_quat());
									Quat velocityDir = velocity.to_rotator().to_quat();
									velocity = velocityDir.to_world(dispersionAngle).get_y_axis() * velocity.length();
									projectile->on_activate_set_relative_velocity(velocity);
								}
								shotAny = true;

								tos->spawn_attached(NAME(muzzleFlash), Framework::ModuleTemporaryObjects::SpawnParams().at_socket(self->shootFromMuzzles[idx]));

								if (auto* ls = imo->get_custom<Framework::CustomModules::LightSources>())
								{
									ls->add(NAME(muzzleFlash));
								}
							}
							self->shootFromMuzzles.remove_fast_at(idx);
							--self->shootFromMuzzlesLeft;
						}
						--shotsNow;
					}
					if (shotAny)
					{
						if (auto* s = imo->get_sound())
						{
							s->play_sound(NAME(shoot));
						}
					}
				}
				else
				{
					resetShoots = true;
				}
			}
			LATENT_WAIT(0.1f);
		}
		else
		{
			LATENT_WAIT(2.0f);
			
			resetShoots = true;
			self->shootingNow = false;
		}
	}

	LATENT_ON_BREAK();
	//
	
	LATENT_ON_END();
	//
	if (auto* e = imo->get_custom<CustomModules::EmissiveControl>())
	{
		e->emissive_deactivate(NAME(attack));
	}

	LATENT_END_CODE();

	LATENT_RETURN();
}

LATENT_FUNCTION(Scourer::follow_to_scour)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("follow to scour"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(bool, perceptionSightImpaired);
	LATENT_COMMON_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, scourTask);

	LATENT_VAR(bool, placeEnemyAtTheEndOfPath);

	LATENT_VAR(float, timeSinceEnemySeen);
	LATENT_VAR(float, timeSinceEnemySeenStop);
	LATENT_VAR(float, timeSinceEnemySeenHunt);
	LATENT_VAR(bool, runAwayFromTheEnemy);
	LATENT_VAR(bool, isRunningAway);
	LATENT_VAR(bool, pathTaskForRunningAway);
	LATENT_VAR(bool, wasRunningAwayThroughTransportRoom);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<Scourer>(mind->get_logic());

	if (! scourTask.is_running())
	{	// setup and start look at task
		::Framework::AI::LatentTaskInfoWithParams scourTaskInfo;
		scourTaskInfo.clear();
		scourTaskInfo.propose(AI_LATENT_TASK_FUNCTION(scour));
		scourTask.start_latent_task(mind, scourTaskInfo);
	}

	timeSinceEnemySeenStop = 3.0f;
	timeSinceEnemySeenHunt = 10.0f;

	timeSinceEnemySeen += LATENT_DELTA_TIME;

	{
		if (enemyPlacement.get_target())
		{
			if (timeSinceEnemySeen > timeSinceEnemySeenStop)
			{
				// run further
				path.clear();
			}
			// run away from the enemy
			timeSinceEnemySeen = 0.0f;
			runAwayFromTheEnemy = true;
		}
		else
		{
			// we haven't seen the enemy for a while now
			if (timeSinceEnemySeen > timeSinceEnemySeenHunt)
			{
				runAwayFromTheEnemy = false;
			}
		}
		bool runningAwayNowOrAboutTo = isRunningAway || pathTaskForRunningAway;
		if (runAwayFromTheEnemy != runningAwayNowOrAboutTo)
		{
			if (!isRunningAway)
			{
				wasRunningAwayThroughTransportRoom = false;
			}
			path.clear();
		}
	}

	LATENT_BEGIN_CODE();

	self->lookAtBlocked = 0; // allow!

	timeSinceEnemySeen = 0.0f;
	runAwayFromTheEnemy = false;
	isRunningAway = false;
	pathTaskForRunningAway = false;

	placeEnemyAtTheEndOfPath = false;

	ai_log(self, TXT("follow to scour"));

	while (enemyPlacement.is_active())
	{
		LATENT_CLEAR_LOG();

		if (!pathTask.is_set() && enemyPlacement.is_active())
		{
			placeEnemyAtTheEndOfPath = false;

			LATENT_LOG(TXT("get path"));
			if (runAwayFromTheEnemy)
			{
				LATENT_LOG(TXT("run away"));
				if ((timeSinceEnemySeen <= timeSinceEnemySeenStop && imo->get_presence()->get_in_room()->get_tags().get_tag(NAME(transportRoom)) != 0.0f)
					|| !wasRunningAwayThroughTransportRoom // if hasn't been running through a transport room, run in any direction
					|| enemyPlacement.get_target()) // we see enemy, run anywhere
				{
					Framework::DoorInRoom* through = nullptr;
					if (enemyPlacement.get_through_doors().is_empty())
					{
						LATENT_LOG(TXT("we're in the same room, run away through the closest door to us"));
						Vector3 imoLoc = imo->get_presence()->get_placement().get_translation();
						Vector3 enmLoc = enemyPlacement.get_placement_in_owner_room().get_translation();
						through = Framework::RoomUtils::find_closest_door(imo->get_presence()->get_in_room(), imoLoc);
						auto* closestToEnemy = Framework::RoomUtils::find_closest_door(imo->get_presence()->get_in_room(), enmLoc);
						if (through &&
							through == closestToEnemy)
						{
							// if the same door and it is closer to the enemy, check the other one
							Vector3 thrLoc = through->get_placement().get_translation();
							if ((enmLoc - thrLoc).length_squared() < (imoLoc - thrLoc).length_squared())
							{
								through = Framework::RoomUtils::get_random_door(imo->get_presence()->get_in_room(), closestToEnemy);
							}
						}
					}
					else
					{
						LATENT_LOG(TXT("we're in a different room, run away even further"));
						Framework::DoorInRoom* notThrough = enemyPlacement.get_through_doors().get_first().get();
						//through = Framework::RoomUtils::get_random_door(imo->get_presence()->get_in_room(), notThrough, false);
						through = Framework::RoomUtils::find_furthest_door(imo->get_presence()->get_in_room(), notThrough->get_placement().get_translation());
					}
					if (through)
					{
						Framework::Nav::PathRequestInfo pathRequestInfo(imo);
						pathTask = mind->access_navigation().find_path_through(through, 0.5f, pathRequestInfo);
					}
					pathTaskForRunningAway = true;
				}
				else
				{
					LATENT_LOG(TXT("just stand hereaway"));
					isRunningAway = true;
					path.clear();
				}
			}
			else
			{
				LATENT_LOG(TXT("fight"));
				bool continueTask = Actions::find_path(_frame, mind, pathTask, enemy, enemyPlacement, perceptionSightImpaired);

				if (!continueTask)
				{
					LATENT_LOG(TXT("no enemy here?")); // find_path handles enemy
					if (imo->get_ai() && // this should always be the case?
						mind->access_navigation().is_at_transport_route())
					{
						LATENT_LOG(TXT("transport route, follow"));
						Framework::Nav::PathRequestInfo pathRequestInfo(imo);
						if (!Mobility::may_leave_room_to_investigate(imo, imo->get_presence()->get_in_room()))
						{
							pathRequestInfo.within_same_room_and_navigation_group();
						}
						Framework::DoorInRoom* through = nullptr;
						if (!enemyPlacement.get_through_doors().is_empty())
						{
							through = enemyPlacement.get_through_doors().get_first().get();
						}
						if (!through)
						{
							through = Framework::RoomUtils::get_random_door(imo->get_presence()->get_in_room(), imo->get_ai()->get_entered_through_door());
							if (through && through->get_door() && !Framework::DoorOperation::is_likely_to_be_open(through->get_door()->get_operation()))
							{
								ai_log(self, TXT("can't get to enemy, drop it (dead end, one of the doors permanently closed"));
								LATENT_LOG(TXT("door closed"));
								thisTaskHandle->access_result().access(NAME(taskFeedback)).access_as<Name>() = NAME(enemyLost);
								enemyPlacement.clear_target(); // without this we may get stuck here
								isRunningAway = false;
								goto MOVED;
							}
						}
						pathTask = mind->access_navigation().find_path_through(through, 0.5f, pathRequestInfo);
						placeEnemyAtTheEndOfPath = true;
					}
					else
					{
						ai_log(self, TXT("can't get to enemy, drop it (dead end or multiple (>2) exits)"));
						LATENT_LOG(TXT("really close and no enemy in sight and not at a transport route, try doing something else"));
						thisTaskHandle->access_result().access(NAME(taskFeedback)).access_as<Name>() = NAME(enemyLost);
						enemyPlacement.clear_target(); // without this we may get stuck here
						goto MOVED;
					}
				}

				if (pathTask.is_set())
				{
					pathTaskForRunningAway = false;
				}
				else
				{
					isRunningAway = false;
				}
			}
		}

		if (pathTask.is_set())
		{
			LATENT_NAV_TASK_WAIT_IF_SUCCEED(pathTask)
			{
				if (auto* task = fast_cast<Framework::Nav::Tasks::PathTask>(pathTask.get()))
				{
					LATENT_LOG(TXT("new path available"));
					ai_log(self, TXT("new path given"));

					isRunningAway = pathTaskForRunningAway;
					path = task->get_path();
					if (placeEnemyAtTheEndOfPath)
					{
						ai_log(self, TXT("enemy placement to the end of path"));
						enemyPlacement.find_path(imo, path->get_final_room(), Transform(path->get_final_node_location(), Quat::identity));
						placeEnemyAtTheEndOfPath = false;
					}
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
			if (isRunningAway && timeSinceEnemySeen > timeSinceEnemySeenStop)
			{
				auto& locomotion = mind->access_locomotion();
				LATENT_LOG(TXT("running away, we should be safe now, we haven't seen the enemy for a moment"));
				locomotion.dont_move();
				goto MOVED;
			}
			if (path.is_set())
			{
				// no target visible
				self->lookAtBlocked = runAwayFromTheEnemy? true : (path->is_transport_path() ? 1 : 0);
				if (imo->get_presence()->get_in_room()->get_tags().get_tag(NAME(transportRoom)) != 0.0f)
				{
					// look forward in a transport room
					self->lookAtBlocked = 1;
					if (isRunningAway)
					{
						wasRunningAwayThroughTransportRoom = true;
					}
				}
			}
			{
				auto& locomotion = mind->access_locomotion();
				if (path.get() && ! locomotion.check_if_path_is_ok(path.get()))
				{
					LATENT_LOG(TXT("path got outdated"));
					locomotion.dont_move();
					goto MOVED;
				}
				float pathDistance = 0.0f;
				if (path.get() && !path->is_close_to_the_end(imo->get_presence()->get_in_room(), imo->get_presence()->get_placement().get_translation(), 0.25f, NP, &pathDistance))
				{
					LATENT_LOG(TXT("not close (%.3f)"), pathDistance);
					if (!locomotion.is_following_path(path.get()))
					{
						LATENT_LOG(TXT("follow path"));
						locomotion.follow_path_2d(path.get(), NP, Framework::MovementParameters().full_speed());
						locomotion.turn_follow_path_2d();
						if (isRunningAway)
						{
							locomotion.turn_yaw_offset(180.0f);
						}
					}
				}
				else
				{
					LATENT_LOG(TXT("close, don't move"));
					locomotion.dont_move();
					goto MOVED;
				}
			}

			{
				auto& locomotion = mind->access_locomotion();
				if (locomotion.has_finished_move(0.5f))
				{
					ai_log(self, TXT("reached the destination"));
					goto MOVED;
				}
			}
			LATENT_WAIT(Random::get_float(0.1f, 0.3f));
		}
		goto MOVE;

		MOVED:
		{
			LATENT_YIELD();
		}
	}

	if (!enemyPlacement.is_active())
	{
		ai_log_colour(self, Colour::red);
		ai_log(self, TXT("no more enemy to follow"));
		ai_log_no_colour(self);
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	if (mind)
	{
		auto& locomotion = mind->access_locomotion();
		locomotion.stop();
	}
	scourTask.stop();

	self->lookAtBlocked = 1; // block again

	LATENT_END_CODE();

	LATENT_RETURN();
}

LATENT_FUNCTION(Scourer::wander_or_scour)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("wander or scour"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);
	LATENT_VAR(::Framework::AI::LatentTaskInfoWithParams, nextTask);

	LATENT_BEGIN_CODE();

	while (true)
	{
		LATENT_CLEAR_LOG();

		nextTask.clear();
		nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::wander));
		if (enemy.is_set() &&
			enemyPlacement.is_active())
		{
			nextTask.propose(AI_LATENT_TASK_FUNCTION(follow_to_scour), 10);
		}

		if (currentTask.can_start(nextTask))
		{
			currentTask.start_latent_task(mind, nextTask);
		}

		LATENT_WAIT(Random::get_float(0.1f, 0.3f)); // reaction time
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();

	// update allow to interrupt state
	thisTaskHandle->allow_to_interrupt(currentTask.can_start_new());

	LATENT_RETURN();
}

LATENT_FUNCTION(Scourer::execute_logic)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Name, scriptedBehaviour);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(Vector3, enemyTargetingOffsetOS);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, announcePresenceTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, perceptionTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, lookAtTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, deflectTask);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);

	auto* self = fast_cast<Scourer>(mind->get_logic());

	LATENT_BEGIN_CODE();

	self->lookAtBlocked = 1; // only scouring allows to look at

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

	{	// setup and start look at task
		::Framework::AI::LatentTaskInfoWithParams lookAtTaskInfo;
		lookAtTaskInfo.clear();
		lookAtTaskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::look_at));
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, int*, &self->lookAtBlocked); // no blocking
		ADD_LATENT_TASK_INFO_PARAM_OPTIONAL_NOT_PROVIDED(lookAtTaskInfo);
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, ::Framework::RelativeToPresencePlacement*, &enemyPlacement);
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, Vector3*, &enemyTargetingOffsetOS);
		lookAtTask.start_latent_task(mind, lookAtTaskInfo);
	}

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(deflect));
		deflectTask.start_latent_task(mind, taskInfo);
	}

	while (true)
	{
		if (Common::handle_scripted_behaviour(mind, scriptedBehaviour, currentTask))
		{
			// doing scripted behaviour
		}
		else
		{
			if (!currentTask.is_running())
			{
				::Framework::AI::LatentTaskInfoWithParams nextTask;
				nextTask.propose(AI_LATENT_TASK_FUNCTION(wander_or_scour));
				currentTask.start_latent_task(mind, nextTask);
			}
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//
	
	LATENT_ON_END();
	//
	announcePresenceTask.stop();
	perceptionTask.stop();
	currentTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}
