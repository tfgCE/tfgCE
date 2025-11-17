#include "aiLogic_centipede.h"

#include "tasks\aiLogicTask_perception.h"

#include "..\aiCommon.h"
#include "..\aiCommonVariables.h"

#include "..\..\game\game.h"
#include "..\..\modules\moduleMovementCentipede.h"
#include "..\..\modules\moduleSound.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\health\mc_health.h"

#include "..\..\..\framework\framework.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\appearance\controllers\ac_particlesUtils.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\moduleController.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\world\room.h"

#include "..\..\..\core\debug\debugRenderer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define DEBUG_DRAW_CENTIPEDE_LOGIC

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// parameters
DEFINE_STATIC_NAME(createCentipedeTails);
DEFINE_STATIC_NAME(centipedeHeadIMO);
DEFINE_STATIC_NAME(centipedeStartStationary);
DEFINE_STATIC_NAME(centipedeMaxHeadingOff);
DEFINE_STATIC_NAME(centipedeMaxHeadingBackOffAngle);
DEFINE_STATIC_NAME(centipedeCallInterval);
DEFINE_STATIC_NAME(centipedeShootingInterval);
DEFINE_STATIC_NAME(centipedeShootingFirstShotAferTime);
DEFINE_STATIC_NAME(centipedeShootingFrontAngle);
DEFINE_STATIC_NAME(centipedeShootingBackShotChance);
DEFINE_STATIC_NAME(centipedeShootingBackShotMeanChance);
DEFINE_STATIC_NAME(centipedeShootingDelay);
DEFINE_STATIC_NAME(centipedeTimeToDestroyFirst);
DEFINE_STATIC_NAME(centipedeTimeToDestroyNext);
DEFINE_STATIC_NAME(centipedeTimeToDestroyLast);
DEFINE_STATIC_NAME(projectileSpeed);

// emissive layers
DEFINE_STATIC_NAME(head);

// variable
DEFINE_STATIC_NAME(isHead);
DEFINE_STATIC_NAME(backPlateDestroyed);
DEFINE_STATIC_NAME(backPlateExists);
DEFINE_STATIC_NAME(antennaeActive);

// pois
DEFINE_STATIC_NAME_STR(centipedeBoundary, TXT("centipede boundary"));

// poi params
DEFINE_STATIC_NAME(perpendicularRadius);

// ai messages
DEFINE_STATIC_NAME_STR(aimCentipedeStart, TXT("centipede; start"));

// temporary objects
DEFINE_STATIC_NAME(shoot);
DEFINE_STATIC_NAME(preShoot);
DEFINE_STATIC_NAME(muzzleFlash);

// sounds
DEFINE_STATIC_NAME(attack);
DEFINE_STATIC_NAME(call);
DEFINE_STATIC_NAME(hit);
DEFINE_STATIC_NAME_STR(smallHit, TXT("small hit"));


//

Concurrency::Counter g_centipedeId;
//

REGISTER_FOR_FAST_CAST(Centipede);

Centipede::Centipede(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	mark_requires_ready_to_activate();

	centipedeId = g_centipedeId.increase();

	shield.set_owner(_mind->get_owner_as_modules_owner());
}

Centipede::~Centipede()
{
	if (auto* psimo = preShootTO.get())
	{
		Framework::ParticlesUtils::desire_to_deactivate(psimo);

		psimo->cease_to_exist(false);

		preShootTO.clear();
	}

	if (auto* m = selfManager.get())
	{
		m->remove(this);
	}
}

void Centipede::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	setup.createCentipedeTails = _parameters.get_value<int>(NAME(createCentipedeTails), setup.createCentipedeTails);

	headIMO = _parameters.get_value<SafePtr<Framework::IModulesOwner>>(NAME(centipedeHeadIMO), SafePtr<Framework::IModulesOwner>());

	// as all segments may have it set, all should be informed about it via AI message
	bool beStationary = _parameters.get_value<bool>(NAME(centipedeStartStationary), action == Action::StayStationary);
	if (beStationary)
	{
		action = Action::StayStationary;
		update_movement_module();
	}

	maxHeadingOff = _parameters.get_value<float>(NAME(centipedeMaxHeadingOff), maxHeadingOff);
	maxHeadingBackOffAngle = _parameters.get_value<float>(NAME(centipedeMaxHeadingBackOffAngle), maxHeadingBackOffAngle);
	
	callInterval = _parameters.get_value<Range>(NAME(centipedeCallInterval), callInterval);

	shootingDelay = _parameters.get_value<float>(NAME(centipedeShootingDelay), shootingDelay);

	shield.learn_from(_parameters);
}

void Centipede::ready_to_activate()
{
	base::ready_to_activate();

	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		isHeadVar.set_name(NAME(isHead));
		isHeadVar.look_up<bool>(imo->access_variables());
	}
}

void Centipede::set_eyes(bool _on)
{
	if (eyesOn.is_set() &&
		eyesOn.get() == _on)
	{
		return;
	}
	eyesOn = _on;
	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
		{
			if (eyesOn.get())
			{
				em->emissive_activate(NAME(head));
			}
			else
			{
				em->emissive_deactivate(NAME(head));
			}
		}
		imo->access_variables().access<float>(NAME(antennaeActive)) = eyesOn.get() ? 1.0f : 0.0f;
	}
}

void Centipede::set_back_plate(bool _on)
{
	if (backPlate.is_set() &&
		backPlate.get() == _on)
	{
		return;
	}
	backPlate = _on;
	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		imo->access_variables().access<float>(NAME(backPlateExists)) = backPlate.get() ? 1.0f : 0.0f;
	}
}

LATENT_FUNCTION(Centipede::head_behaviour)
{
	scoped_call_stack_info(TXT("Centipede::head_behaviour"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("head_behaviour"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR_END();

	LATENT_VAR(Random::Generator, rg);
	LATENT_VAR(float, distanceToTurnAway);
	
	LATENT_VAR(float, forceTurnAway);
	LATENT_VAR(float, forceTurnAwayMoveForward);
	LATENT_VAR(float, forceTurnAwayTimeLeft);
	
	LATENT_VAR(Optional<float>, delayToShoot);

	LATENT_VAR(float, timeToCall);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Centipede>(mind->get_logic());

	LATENT_BEGIN_CODE();

	timeToCall = self->enteredBoundary? rg.get(self->callInterval) : 0.0f;

	// only head shoots, that's why self manager handling is done here
	if (auto* m = self->selfManager.get())
	{
		m->add(self);
	}

	if (auto* h = imo->get_custom<CustomModules::Health>())
	{
		h->be_invincible(0.5f); // be invincible for a very brief moment, this is to avoid killing with auto gun
	}

	while (true)
	{
		{
			timeToCall -= LATENT_DELTA_TIME;
			if (timeToCall < 0.0f)
			{
				timeToCall = rg.get(self->callInterval);

				if (auto* s = imo->get_sound())
				{
					if (!s->is_playing(NAME(attack)))
					{
						if (auto* s = fast_cast<ModuleSound>(imo->get_sound()))
						{
							if (!s->is_playing_talk_sounds())
							{
								s->play_sound(NAME(call));
							}
						}
					}
				}
			}
		}

		{
			if (CentipedeSelfManager::Orders* orders = self->selfManager->advance_for(self, LATENT_DELTA_TIME))
			{
				if (orders->pendingShot.get() > 0)
				{
					orders->pendingShot.decrease();

					if (!self->preShootTO.get())
					{
						delayToShoot = self->shootingDelay;

						if (auto* tos = imo->get_temporary_objects())
						{
							self->preShootTO = tos->spawn(NAME(preShoot));
						}
						if (auto* s = imo->get_sound())
						{
							if (auto* s = fast_cast<ModuleSound>(imo->get_sound()))
							{
								s->stop_talk_sounds(0.2f);
							}
							s->play_sound(NAME(attack));
							s->play_sound(NAME(preShoot));
						}
					}
				}
			}

			if (delayToShoot.is_set())
			{
				delayToShoot = delayToShoot.get() - LATENT_DELTA_TIME;
				if (delayToShoot.get() <= 0)
				{
					delayToShoot.clear();

					if (auto* psimo = self->preShootTO.get())
					{
						Framework::ParticlesUtils::desire_to_deactivate(psimo);

						psimo->cease_to_exist(false);

						self->preShootTO.clear();
					}

					todo_multiplayer_issue(TXT("we just get player here"));
					Transform eyesWS = Transform::identity;
					if (auto* g = Game::get_as<Game>())
					{
						if (auto* pa = g->access_player().get_actor())
						{
							if (auto* presence = pa->get_presence())
							{
								eyesWS = presence->get_placement().to_world(presence->get_eyes_relative_look());
							}
						}
					}

					if (auto* tos = imo->get_temporary_objects())
					{
						if (auto* projectile = tos->spawn(NAME(shoot)))
						{
							// just in any case if we would be shooting from inside of a capsule
							if (auto* collision = projectile->get_collision())
							{
								collision->dont_collide_with_up_to_top_instigator(imo, 0.5f);
							}
							float useProjectileSpeed = projectile->get_variables().get_value<float>(NAME(projectileSpeed), 5.0f);
							useProjectileSpeed = imo->get_variables().get_value<float>(NAME(projectileSpeed), useProjectileSpeed);

							projectile->on_activate_face_towards(eyesWS.get_translation());

							Vector3 velocity = Vector3(0.0f, useProjectileSpeed, 0.0f);
							projectile->on_activate_set_relative_velocity(velocity);

							tos->spawn_all(NAME(muzzleFlash));

							if (auto* s = imo->get_sound())
							{
								s->play_sound(NAME(shoot));
							}
						}
					}
				}
			}
		}

		if (self->boundaries.get_size() == 2)
		{
			Vector3 bStart = self->boundaries[0].loc;
			Vector3 bEnd = self->boundaries[1].loc;
			float bLength = max(0.1f, (bEnd - bStart).length());
			Vector3 bDir = (bEnd - bStart) / bLength;

			Transform placement = imo->get_presence()->get_placement();
			Vector3 loc = placement.get_translation();
			float alongLoc = Vector3::dot(loc - bStart, bDir);
			float alongPt = alongLoc / bLength;
			
			float alongVelocity = Vector3::dot(imo->get_presence()->get_velocity_linear(), bDir);

			float moveForward = 0.0f;
			Optional<float> turn;

			if (!self->enteredBoundary)
			{
				moveForward = 1.0f;
				if (alongPt > 0.0f && alongPt < 1.0f)
				{
					self->enteredBoundary = true;
					if (auto* m = self->selfManager.get())
					{
						m->mark_entered_boundary();
					}
				}
			}
			else
			{

				float linearSpeed = 4.0f;
				float turnSpeed = 120.0f;
				if (auto* m = imo->get_movement())
				{
					linearSpeed = m->find_speed_of_current_gait();
					turnSpeed = m->find_orientation_speed_of_current_gait().yaw;
				}

				if (self->action == Action::MoveAround ||
					self->action == Action::TurnAway)
				{
					{
						float timeToToTurn = ((360.0f / 4.0f) / turnSpeed);
						// x' + x' =r'    x =r/(sqr(2))
						distanceToTurnAway = (linearSpeed / sqrt(2.0f)) * timeToToTurn;
					}

					{	// check heading back
						todo_note(TXT("use rotation speed from movement"));
						if (alongVelocity > 0.0f)
						{
							if (alongLoc > bLength - distanceToTurnAway)
							{
								self->action = Action::HeadBack;
								self->headTowards = bStart;
								self->headTowardsUntilAngle = self->maxHeadingBackOffAngle;
								self->headTowardsTimeLeft.clear();
							}
						}
						else
						{
							if (alongLoc < distanceToTurnAway)
							{
								self->action = Action::HeadBack;
								self->headTowards = bEnd;
								self->headTowardsUntilAngle = self->maxHeadingBackOffAngle;
								self->headTowardsTimeLeft.clear();
							}
						}
					}
				}

#ifdef DEBUG_DRAW_CENTIPEDE_LOGIC
				debug_context(imo->get_presence()->get_in_room());
#endif
				{
					bool done = false;
					if (self->headTowards.is_set())
					{
						an_assert_immediate(self->headTowards.get().is_ok());
						Vector3 headTowardsLocal = placement.location_to_local(self->headTowards.get());
						an_assert_immediate(headTowardsLocal.is_ok());
						Rotator3 towardsHeadTo = headTowardsLocal.to_rotator().normal_axes();
						an_assert_immediate(towardsHeadTo.is_ok());

						if (self->headTowardsUntilAngle.is_set())
						{
							if (towardsHeadTo.yaw > self->headTowardsUntilAngle.get() || abs(towardsHeadTo.yaw) > 170.0f)
							{
								self->preferredTurn = 1.0f;
							}
							else if (towardsHeadTo.yaw < -self->headTowardsUntilAngle.get())
							{
								self->preferredTurn = -1.0f;
							}
							else
							{
								if (self->preferredTurn.get(0.0f) == 0.0f)
								{
									self->preferredTurn = sign(towardsHeadTo.yaw);
								}
								done = true;
							}
						}
						else
						{
							float useMaxHeadingOff = self->segmentsBehind == 0 ? 0.0f : self->maxHeadingOff;
							if (towardsHeadTo.yaw > useMaxHeadingOff || abs(towardsHeadTo.yaw) > 170.0f)
							{
								self->preferredTurn = 1.0f;
							}
							else if (towardsHeadTo.yaw < -useMaxHeadingOff)
							{
								self->preferredTurn = -1.0f;
							}
							else
							{
								if (self->preferredTurn.get(0.0f) == 0.0f)
								{
									self->preferredTurn = sign(towardsHeadTo.yaw);
								}
								done = true;
							}
						}
						if ((self->headTowards.get() - placement.get_translation()).length() > 1.0f)
						{
							if (self->headTowardsTimeLeft.is_set())
							{
								self->headTowardsTimeLeft = self->headTowardsTimeLeft.get() - LATENT_DELTA_TIME;
								if (self->headTowardsTimeLeft.get() > 0.0f)
								{
									done = false;
								}
							}
						}
					}
					else
					{
						if (self->action == Action::TurnAway)
						{
							done = forceTurnAwayTimeLeft <= 0.0f;
						}
						else
						{
							done = true;
						}
					}

					moveForward = 1.0;
					turn = self->preferredTurn.get(0.0f);
					an_assert_immediate(isfinite(turn.get()));

					if (done)
					{
						if (self->action == Action::HeadBack ||
							self->action == Action::TurnAway)
						{
							self->action = Action::MoveAround;
							self->headTowards.clear();
							self->headTowardsUntilAngle.clear();
							self->headTowardsTimeLeft.clear();
						}
						if (self->action == Action::MoveAround)
						{
							float atPt = mod(alongPt + rg.get_float(0.25f, 0.75f), 1.0f);
							atPt = clamp(atPt, 0.2f, 0.8f);
							Vector3 addOffset = Vector3::zero;
							{
								float radius = lerp(atPt, self->boundaries[0].perpendicularRadius, self->boundaries[1].perpendicularRadius);
								Vector3 bPerpendicularDir1 = abs(bDir.x) < 0.9f ? Vector3::cross(Vector3::xAxis, bDir).normal() : Vector3::cross(Vector3::zAxis, bDir).normal();
								Vector3 bPerpendicularDir2 = Vector3::cross(bDir, bPerpendicularDir1);
								float angle = rg.get_float(-180.0f, 180.0f);
								addOffset = (bPerpendicularDir1 * sin_deg(angle) + bPerpendicularDir2 * cos_deg(angle)) * radius;
							}
							
							self->headTowards = lerp(atPt, bStart, bEnd) + addOffset;

							self->headTowardsUntilAngle.clear();
							self->headTowardsTimeLeft = rg.get_float(1.0f, 4.0f);
						}
					}
				}

				{
					float bestMatch = 0.0f;
					float bestDist = 0.0f;
					float bestAngle = 0.0f;
					int bestIDDiff = 0;
					Framework::IModulesOwner* bestIMO = nullptr;

					//FOR_EVERY_OBJECT_IN_ROOM(o, imo->get_presence()->get_in_room())
					for_every_ptr(o, imo->get_presence()->get_in_room()->get_objects())
					{
						if (o != imo)
						{
							Vector3 toEnemyOS = placement.location_to_local(o->get_presence()->get_placement().get_translation());
							Vector3 enemyMovementOS = placement.vector_to_local(o->get_presence()->get_velocity_linear());
							toEnemyOS = toEnemyOS + enemyMovementOS * 0.2f;
							Vector3 enemyMovementDirOS = enemyMovementOS.normal();
							if (enemyMovementDirOS.y < 0.8f) // not running away from us
							{
								if (toEnemyOS.y > 0.0f && abs(toEnemyOS.z) < 2.0f)
								{
									float dist = toEnemyOS.length();
									if (dist < linearSpeed * 0.8f)
									{
										if (auto* ai = o->get_ai())
										{
											if (auto* m = ai->get_mind())
											{
												if (auto* c = fast_cast<Centipede>(m->get_logic()))
												{
													float angle = Rotator3::get_yaw(toEnemyOS);

													if (abs(angle) < 60.0f)
													{
														float match = -dist - abs(angle) / 70.0f;

														if (!bestIMO || bestMatch < match)
														{
															bestIMO = o;
															bestMatch = match;
															bestDist = dist;
															bestAngle = angle;
															bestIDDiff = c->centipedeId - self->centipedeId;
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}

					if (bestMatch)
					{
						forceTurnAway = -sign(bestAngle);
						forceTurnAwayMoveForward = moveForward;
						forceTurnAwayTimeLeft = 60.0f / turnSpeed;

						an_assert_immediate(isfinite(moveForward));
						if (bestIDDiff < 0)
						{
							// slow down or even stop
							an_assert_immediate(isfinite(forceTurnAwayMoveForward));
							forceTurnAwayMoveForward = remap_value(bestDist, linearSpeed * 0.3f, linearSpeed * 0.6f, 0.2f, 1.0f, true);
							an_assert_immediate(isfinite(forceTurnAwayMoveForward));
						}
						turn = -sign(bestAngle);
						an_assert_immediate(isfinite(turn.get()));
					}

					if (forceTurnAwayTimeLeft > 0.0f)
					{
						forceTurnAwayTimeLeft -= LATENT_DELTA_TIME;

						// always slowdown
						moveForward = forceTurnAwayMoveForward;
						an_assert_immediate(isfinite(moveForward));
						if (self->action != Action::HeadBack)
						{
							turn = forceTurnAway;
							an_assert_immediate(isfinite(turn.get()));

							self->action = Action::TurnAway;
							self->headTowards.clear();
							self->headTowardsUntilAngle.clear();
							self->headTowardsTimeLeft.clear();
						}
					}
				}


#ifdef DEBUG_DRAW_CENTIPEDE_LOGIC
				if (self->headTowards.is_set())
				{
					debug_draw_arrow(true, self->action == Action::HeadBack ? Colour::orange : Colour::yellow, placement.get_translation(), self->headTowards.get());
				}
				debug_draw_transform_size(true, placement, 0.5f);
				debug_draw_text(true, self->action == Action::TurnAway? Colour::red : (self->action == Action::HeadBack? Colour::orange : Colour::greyLight), placement.get_translation(), Vector2(0.5f, 0.0f), true, 1.0f, true,
					self->action == Action::TurnAway ? TXT("TA") : (self->action == Action::HeadBack? TXT("HB") : TXT("--")), turn.get(0.0f));
				debug_draw_text(true, forceTurnAwayTimeLeft > 0.0f? Colour::red :  Colour::greyLight, placement.get_translation(), Vector2(0.5f, 1.0f), true, 1.0f, true, TXT("t:%.0f"), turn.get(0.0f));
				debug_no_context();
#endif
			}

			if (auto* c = imo->get_controller())
			{
				if (moveForward > 0.0f)
				{
					an_assert_immediate(isfinite(moveForward));
					if (self->segmentsBehind == 0)
					{
						moveForward *= lerp(GameSettings::get().difficulty.aiMean, 0.6f, 0.8f);
						an_assert_immediate(isfinite(moveForward));
					}
					else if (self->segmentsBehind == 1)
					{
						moveForward *= lerp(GameSettings::get().difficulty.aiMean, 0.75f, 0.9f);
						an_assert_immediate(isfinite(moveForward));
					}
					else if (self->segmentsBehind < 4)
					{
						moveForward *= lerp(GameSettings::get().difficulty.aiMean, 0.9f, 1.0f);
						an_assert_immediate(isfinite(moveForward));
					}
					an_assert_immediate(isfinite(turn.get(0.0f)));
					//c->set_relative_requested_movement_direction(Vector3::yAxis);
					c->clear_requested_movement_direction();
					c->set_requested_movement_parameters(Framework::MovementParameters().relative_speed(moveForward));
					c->set_requested_relative_velocity_orientation(Rotator3(0.0f, turn.get(0.0f) * moveForward, 0.0f)); // adjust turn to forward speed
				}
				else
				{
					c->clear_requested_movement_direction();
					c->clear_requested_relative_velocity_orientation();
				}
			}
		}
		else
		{
			if (!Framework::is_preview_game())
			{
				todo_implement(TXT("not supported, implement"));
			}
		}
		LATENT_YIELD();
		//LATENT_WAIT(rg.get_float(0.02f, 0.05f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	if (auto* m = self->selfManager.get())
	{
		m->remove(self);
	}
	if (auto* psimo = self->preShootTO.get())
	{
		Framework::ParticlesUtils::desire_to_deactivate(psimo);

		psimo->cease_to_exist(false);

		self->preShootTO.clear();
	}

	Framework::ParticlesUtils::desire_to_deactivate(self->preShootTO.get());
	self->preShootTO.clear();

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Centipede::execute_logic)
{
	scoped_call_stack_info(TXT("Centipede::execute_logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, perceptionTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, headBehaviourTask);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	LATENT_VAR(Random::Generator, rg);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Centipede>(logic);

	self->shield.advance(LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::perception));
		perceptionTask.start_latent_task(mind, taskInfo);
	}

	messageHandler.use_with(mind);
	{
		SafePtr<Framework::IModulesOwner> keeper(imo);
		messageHandler.set(NAME(aimCentipedeStart), [keeper, self](Framework::AI::Message const& _message)
			{
				if (keeper.get())
				{
					if (self->action == Action::StayStationary)
					{
						self->action = Action::MoveAround;
						self->update_movement_module();
					}
				}
			}
		);
	}	

	LATENT_WAIT(0.01f);

	self->selfManager = CentipedeSelfManager::get_or_create();

	if (auto* inRoom = imo->get_presence()->get_in_room())
	{
		inRoom->for_every_point_of_interest(NAME(centipedeBoundary), [self](Framework::PointOfInterestInstance* _poi)
			{
				Vector3 loc = _poi->calculate_placement().get_translation();
				BoundaryPoint bp;
				bp.loc = loc;
				bp.perpendicularRadius = _poi->get_parameters().get_value<float>(NAME(perpendicularRadius), bp.perpendicularRadius);
				self->boundaries.push_back(bp);
			});
	}
	{	// process setup
		if (!self->headIMO.get())
		{
			// we're the first one! less enemies on a modifier
			self->setup.createCentipedeTails = TypeConversions::Normal::f_i_closest(GameSettings::get().difficulty.spawnCountModifier * (float)self->setup.createCentipedeTails);
		}
		if (self->setup.createCentipedeTails > 0 && imo->get_as_object())
		{
			auto* inRoom = imo->get_presence()->get_in_room();
			Framework::DelayedObjectCreation* doc = new Framework::DelayedObjectCreation();
			doc->activateImmediatelyEvenIfRoomVisible = true;
			doc->wmpOwnerObject = imo;
			doc->inRoom = inRoom;
			doc->name = String::printf(TXT("centipede segment spawned by %i"), self->centipedeId);
			doc->objectType = cast_to_nonconst(imo->get_as_object()->get_object_type());
			doc->placement = imo->get_presence()->get_placement().to_world(Transform(Vector3::yAxis * -1.0f, Quat::identity));
			doc->randomGenerator = imo->get_individual_random_generator();
			doc->randomGenerator = doc->randomGenerator.spawn();
			doc->priority = 1000;
			doc->checkSpaceBlockers = false;

			doc->variables.set_from(imo->get_variables()); // copy all variables

			self->tailDOC = doc;

			SafePtr<Framework::IModulesOwner> keeper(imo);
			doc->pre_initialise_modules_function = [keeper, self](Framework::Object* spawnedObject)
			{
				if (auto* imo = keeper.get())
				{
					spawnedObject->access_variables().access<int>(NAME(createCentipedeTails)) = self->setup.createCentipedeTails - 1;
					spawnedObject->access_variables().access<SafePtr<Framework::IModulesOwner>>(NAME(centipedeHeadIMO)) = imo;
				}
			};

			Game::get()->queue_delayed_object_creation(doc);
		}
	}

	if (auto* cm = fast_cast<ModuleMovementCentipede>(imo->get_movement()))
	{
		cm->set_head(self->headIMO.get());
	}
	if (self->isHeadVar.is_valid())
	{
		self->isHeadVar.access<bool>() = !self->headIMO.get();
	}

	LATENT_WAIT(Random::get_float(0.1f, 0.6f));

	while (true)
	{
		if (auto* doc = self->tailDOC.get())
		{
			if (doc->is_done())
			{
				self->tailIMO = doc->createdObject.get();
				self->tailDOC.clear();
				if (auto* cm = fast_cast<ModuleMovementCentipede>(imo->get_movement()))
				{
					cm->set_tail(self->tailIMO.get());
				}
			}
		}
		{
			if (imo->get_variables().get_value<bool>(NAME(backPlateDestroyed), false))
			{
				// disconnect
				self->tailIMO.clear();
				if (auto* cm = fast_cast<ModuleMovementCentipede>(imo->get_movement()))
				{
					cm->set_tail(nullptr);
				}
			}
		}
		if (auto* himo = self->headIMO.get())
		{
			if (auto* ai = himo->get_ai())
			{
				if (auto* m = ai->get_mind())
				{
					if (auto* c = fast_cast<Centipede>(m->get_logic()))
					{
						if (c->backPlate.is_set() &&
							!c->backPlate.get())
						{
							self->headIMO.clear();
							if (auto* cm = fast_cast<ModuleMovementCentipede>(imo->get_movement()))
							{
								cm->set_head(nullptr);
							}
							if (self->isHeadVar.is_valid())
							{
								self->isHeadVar.access<bool>() = true;
							}
						}
					}
					else
					{
						self->headIMO.clear();
						if (auto* cm = fast_cast<ModuleMovementCentipede>(imo->get_movement()))
						{
							cm->set_head(nullptr);
						}
						if (self->isHeadVar.is_valid())
						{
							self->isHeadVar.access<bool>() = true;
						}
					}
				}
			}
		}
		{
			int prevSegmentsBehind = self->segmentsBehind;
			if (auto* timo = self->tailIMO.get())
			{
				if (auto* ai = timo->get_ai())
				{
					if (auto* m = ai->get_mind())
					{
						if (auto* c = fast_cast<Centipede>(m->get_logic()))
						{
							self->segmentsBehind = c->segmentsBehind + 1;
						}
					}
				}
			}
			else
			{
				self->segmentsBehind = 0;
			}
			if (! self->headIMO.get() &&
				self->segmentsBehind < prevSegmentsBehind)
			{
				if (auto* s = imo->get_sound())
				{
					if (!s->is_playing(NAME(attack)))
					{
						if (auto* s = fast_cast<ModuleSound>(imo->get_sound()))
						{
							s->stop_talk_sounds(0.2f);
						}
						s->play_sound(rg.get_chance(0.8f) ? NAME(hit) : NAME(smallHit));
					}
				}
			}
		}
		self->shield.set_shield_requested(!self->headIMO.get() && self->segmentsBehind > 1); // if really small, no shield
		self->set_eyes(!self->headIMO.get());
		self->set_back_plate(self->tailIMO.get() || self->tailDOC.is_set());

		{
			::Framework::AI::LatentTaskInfoWithParams headBehaviourTaskInfo;
			if (!self->headIMO.get() && self->action != Action::StayStationary)
			{
				if (headBehaviourTaskInfo.propose(AI_LATENT_TASK_FUNCTION(head_behaviour)))
				{
					// no params?
				}
			}
			if (!headBehaviourTask.is_running(headBehaviourTaskInfo.taskFunctionInfo) &&
				headBehaviourTask.can_be_interrupted())
			{	
				headBehaviourTask.start_latent_task(mind, headBehaviourTaskInfo);
			}
		}
		LATENT_YIELD(); 
		//LATENT_WAIT(rg.get_float(0.02f, 0.05f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	perceptionTask.stop();
	//
	if (auto* m = CentipedeSelfManager::get_existing())
	{
		m->mark_segment_destroyed();
	}

	LATENT_END_CODE();
	LATENT_RETURN();
}

void Centipede::update_movement_module()
{
	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		if (auto* cm = fast_cast<ModuleMovementCentipede>(imo->get_movement()))
		{
			cm->set_stationary(action == Action::StayStationary);
		}
	}
}

//

CentipedeSelfManager* CentipedeSelfManager::s_manager = nullptr;
Concurrency::SpinLock CentipedeSelfManager::s_managerLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.AI.Logics.CentipedeSelfManager.s_managerLock"));;

CentipedeSelfManager* CentipedeSelfManager::get_or_create()
{
	Concurrency::ScopedSpinLock lock(s_managerLock, true);
	if (!s_manager)
	{
		return new CentipedeSelfManager();
	}
	return s_manager;
}

CentipedeSelfManager* CentipedeSelfManager::get_existing()
{
	Concurrency::ScopedSpinLock lock(s_managerLock, true);
	return s_manager;
}

CentipedeSelfManager::CentipedeSelfManager()
{
	an_assert(! s_manager);
	Concurrency::ScopedSpinLock lock(s_managerLock, true);
	s_manager = this;

	centipedes.set_size(MAX_CENTIPEDES);
}

CentipedeSelfManager::~CentipedeSelfManager()
{
	an_assert(s_manager);
	Concurrency::ScopedSpinLock lock(s_managerLock, true);
	s_manager = nullptr;
}

void CentipedeSelfManager::add(Centipede* _centipede)
{
	Concurrency::ScopedSpinLock lock(s_managerLock, true);
	for_every(c, centipedes)
	{
		if (!c->centipede)
		{
			timeSinceLastDestroyed = 0.0f; // reset timer, if we add, it means that something got destroyed and we separated into a few
			c->centipede = _centipede;
			c->imo = _centipede->get_mind()->get_owner_as_modules_owner();
			c->orders = Orders();
			if (!mainAdvancer)
			{
				mainAdvancer = _centipede;

				// we're first!

				auto* imo = c->imo.get();
				an_assert(imo);
				{
					shootingInterval = imo->get_variables().get_value<Range>(NAME(centipedeShootingInterval), shootingInterval);
					
					timeLeftToShoot = rg.get(imo->get_variables().get_value<Range>(NAME(centipedeShootingFirstShotAferTime), shootingInterval));
					
					timeToDestroy = timeToDestroyFirst;
					timeToDestroyFirst = imo->get_variables().get_value<float>(NAME(centipedeTimeToDestroyFirst), timeToDestroyFirst);
					timeToDestroyNext = imo->get_variables().get_value<float>(NAME(centipedeTimeToDestroyNext), timeToDestroyNext);
					timeToDestroyLast = imo->get_variables().get_value<float>(NAME(centipedeTimeToDestroyLast), timeToDestroyLast);

					frontAngle = imo->get_variables().get_value<float>(NAME(centipedeShootingFrontAngle), frontAngle);
					backShotChance = imo->get_variables().get_value<float>(NAME(centipedeShootingBackShotChance), backShotChance);
					backShotMeanChance = imo->get_variables().get_value<float>(NAME(centipedeShootingBackShotMeanChance), backShotMeanChance);
				}
			}
			break;
		}
	}
}

void CentipedeSelfManager::remove(Centipede* _centipede)
{
	Concurrency::ScopedSpinLock lock(s_managerLock, true);
	for_every(c, centipedes)
	{
		if (c->centipede == _centipede)
		{
			timeSinceLastDestroyed = 0.0f; // reset timer
			c->centipede = nullptr;
			if (mainAdvancer == _centipede)
			{
				for_every(mac, centipedes)
				{
					if (mac->centipede)
					{
						mainAdvancer = mac->centipede;
					}
				}
			}
			break;
		}
	}
}

void CentipedeSelfManager::mark_segment_destroyed()
{
	Concurrency::ScopedSpinLock lock(s_managerLock, true);
	timeSinceLastDestroyed = 0.0f;
}

void CentipedeSelfManager::mark_entered_boundary()
{
	if (!enteredBoundary)
	{
		Concurrency::ScopedSpinLock lock(s_managerLock, true);
		if (!enteredBoundary)
		{
			enteredBoundary = true;
			timeToDestroy = timeToDestroyFirst;
		}
	}
}

CentipedeSelfManager::Orders* CentipedeSelfManager::advance_for(Centipede* _centipede, float _deltaTime)
{
	if (mainAdvancer == _centipede)
	{
		if (enteredBoundary)
		{
			timeSinceLastDestroyed += _deltaTime;
			if (timeToDestroy > 0.0f &&
				timeSinceLastDestroyed > timeToDestroy &&
				!centipedes.is_empty()) // never should be empty
			{
				// destroy the last segment - we should kill all segments till there's nothing left
				Concurrency::ScopedSpinLock lock(s_managerLock, true);
				Centipede* centipede = nullptr;
				for_every(c, centipedes)
				{
					if (c->centipede)
					{
						centipede = c->centipede;
					}
				}
				if (centipede)
				{
					int segmentCount = 1;
					while (true)
					{
						if (auto* imo = centipede->tailIMO.get())
						{
							if (auto* h = imo->get_custom<CustomModules::Health>())
							{
								if (!h->is_alive())
								{
									break;
								}
							}

							if (auto* ai = imo->get_ai())
							{
								if (auto* m = ai->get_mind())
								{
									if (auto* c = fast_cast<Centipede>(m->get_logic()))
									{
										++segmentCount;
										centipede = c;
										continue;
									}
								}
							}
							break;
						}
						else
						{
							break;
						}
					}
					if (auto* cimo = centipede->get_imo())
					{
						if (auto* h = cimo->get_custom<CustomModules::Health>())
						{
							bool isLast = false;
							if (segmentCount <= 2) // quite likely it might be the last segment but we can't be sure, calculate all segments in all centipedes
							{
								isLast = true;
								segmentCount = 0;
								for_every(c, centipedes)
								{
									if (auto* centipede = c->centipede)
									{
										++segmentCount;
										while (isLast)
										{
											if (auto* imo = centipede->tailIMO.get())
											{
												if (auto* h = imo->get_custom<CustomModules::Health>())
												{
													if (!h->is_alive())
													{
														break;
													}
												}
												if (auto* ai = imo->get_ai())
												{
													if (auto* m = ai->get_mind())
													{
														if (auto* c = fast_cast<Centipede>(m->get_logic()))
														{
															++segmentCount;
															if (segmentCount > 2)
															{
																isLast = false;
															}
															centipede = c;
															continue;
														}
													}
												}
												break;
											}
											else
											{
												break;
											}
										}
										if (!isLast)
										{
											break;
										}
									}
								}
							}
							if (isLast)
							{
								// 2 = 1 (last left) + 1 (current one)
								timeToDestroy = timeToDestroyLast;
								timeSinceLastDestroyed = 0.0f;
							}
							else
							{
								timeToDestroy = timeToDestroyNext;
								timeSinceLastDestroyed = 0.0f;
							}
							h->perform_death_without_reward();
						}
					}
				}
			}
		}
		timeLeftToShoot -= _deltaTime;
		if (timeLeftToShoot < 0.0f)
		{
			timeLeftToShoot = rg.get(shootingInterval) * (1.0f - 0.2f * GameSettings::get().difficulty.aiMean);

			todo_multiplayer_issue(TXT("we just get player here"));
			Transform eyesWS = Transform::identity;
			if (auto* g = Game::get_as<Game>())
			{
				if (auto* pa = g->access_player().get_actor())
				{
					if (auto* presence = pa->get_presence())
					{
						eyesWS = presence->get_placement().to_world(presence->get_eyes_relative_look());
					}
				}
			}

			CentipedeSegment* shooter = nullptr;
			{
				float useFrontAngle = frontAngle * (1.0f + 1.0f * GameSettings::get().difficulty.aiMean);
				float frontAngleCos = cos_deg(useFrontAngle);
				float bestAngleCos = 0;
				CentipedeSegment* bestCS = nullptr;
				ArrayStatic<CentipedeSegment*, MAX_CENTIPEDES> csInRadius;
				ArrayStatic<CentipedeSegment*, MAX_CENTIPEDES> allCS;
				for_every(c, centipedes)
				{
					if (c->centipede)
					{
						if (auto* imo = c->imo.get())
						{
							allCS.push_back(c);
							Vector3 locES = eyesWS.location_to_local(imo->get_presence()->get_centre_of_presence_WS());
							float atAngleCos = locES.normal().y;
							if (atAngleCos >= frontAngleCos)
							{
								csInRadius.push_back(c);
							}
							if (!bestCS || bestAngleCos < atAngleCos)
							{
								bestCS = c;
								bestAngleCos = atAngleCos;
							}
						}
					}
				}
				if (rg.get_chance(lerp(GameSettings::get().difficulty.aiMean, backShotChance, backShotMeanChance)))
				{
					// shoot from behind
					if (!allCS.is_empty())
					{
						shooter = allCS[rg.get_int(allCS.get_size())];
					}
				}
				else
				{
					if (!csInRadius.is_empty())
					{
						shooter = csInRadius[rg.get_int(csInRadius.get_size())];
					}
					else
					{
						shooter = bestCS;
					}
				}
			}
			if (shooter)
			{
				shooter->orders.pendingShot.increase();
			}
		}
	}

	// always look for the right centipede
	{
		// should only read memory
		for_every(c, centipedes)
		{
			if (c->centipede == _centipede)
			{
				return &c->orders;
			}
		}
	}
	return nullptr;
}
