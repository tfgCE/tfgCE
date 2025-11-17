#include "aiLogic_plant.h"

#include "tasks\aiLogicTask_grabEnergy.h"

#include "..\..\game\damage.h"

#include "..\..\utils.h"

#include "..\..\modules\gameplay\moduleEnergyQuantum.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_pickup.h"
#include "..\..\modules\custom\health\mc_health.h"
#include "..\..\modules\gameplay\equipment\me_armable.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\collision\checkCollisionContext.h"
#include "..\..\..\framework\collision\checkSegmentResult.h"
#include "..\..\..\framework\game\game.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\meshGen\meshGenValueDefImpl.inl"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\moduleController.h"
#include "..\..\..\framework\module\moduleMovement.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT
//#define INSPECT__PLANTING_IN_ROOM
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// temporary objects
DEFINE_STATIC_NAME_STR(onAttach, TXT("on attach"));
DEFINE_STATIC_NAME_STR(onAttachFastDetach, TXT("on attach, fast detach"));
DEFINE_STATIC_NAME_STR(onDetach, TXT("on detach"));
DEFINE_STATIC_NAME_STR(healthInhaled, TXT("health inhaled"));
DEFINE_STATIC_NAME_STR(spitDischarge, TXT("spit discharge"));

// sounds
DEFINE_STATIC_NAME(spit);

// movements
DEFINE_STATIC_NAME(immovable);
DEFINE_STATIC_NAME(attached);

// vars
DEFINE_STATIC_NAME(flying);
DEFINE_STATIC_NAME(hooksOpenWhenHeld);
DEFINE_STATIC_NAME(plantAttached);
DEFINE_STATIC_NAME(lootObject);

// collision flags
DEFINE_STATIC_NAME(beingHeld);
DEFINE_STATIC_NAME(inPocket);

//

REGISTER_FOR_FAST_CAST(Plant);

Plant::Plant(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	data = fast_cast<PlantData>(_logicData);
}

Plant::~Plant()
{
}

void Plant::advance(float _deltaTime)
{
	bool armed = false;
	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		if (auto* armable = imo->get_gameplay_as<ModuleEquipments::Armable>())
		{
			if (armable->is_armed())
			{
				armed = true;
			}
		}
	}

	if (armed)
	{
		set_auto_rare_advance_if_not_visible(NP);
	}
	else
	{
		set_auto_rare_advance_if_not_visible(Range(0.1f, 0.5f));
	}

	base::advance(_deltaTime);
}

LATENT_FUNCTION(Plant::being_held)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("being held"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(float, additionalWait);
	LATENT_VAR(bool, doAdditionalWait);

	LATENT_VAR(float, proximity);

	LATENT_VAR(float, blockSpitting);
	LATENT_VAR(float, timeToAutoSpitDischarge);

	LATENT_VAR(bool, hooksOpenWhenHeld);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Plant>(mind->get_logic());

	if (doAdditionalWait)
	{
		if (proximity > 0.5f)
		{
			additionalWait = -1.0f;
		}
		else
		{
			additionalWait -= LATENT_DELTA_TIME * (1.0f + 15.0f * proximity);
		}
	}

	blockSpitting = max(0.0f, blockSpitting - LATENT_DELTA_TIME);
	timeToAutoSpitDischarge = max(0.0f, timeToAutoSpitDischarge - LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	proximity = 0.0f;
	additionalWait = 0.0f;
	doAdditionalWait = false;
	blockSpitting = 0.0f;
	timeToAutoSpitDischarge = 0.0f;
	hooksOpenWhenHeld = imo->get_variables().get_value<bool>(NAME(hooksOpenWhenHeld), false);

	imo->activate_movement(NAME(immovable));

	self->hooksInGround.blend_to(0.0f, 0.5f);
	self->hooksOpen.blend_to(0.0f, 0.1f);
	self->draggingLegActive.blend_to(0.0f, 0.3f);

	self->attachedToRoom = false;
	self->attachedToCollidableObject.clear();

	if (auto* c = imo->get_collision())
	{
		bool update = false;
		if (self->data->beingHeldCollidesWithFlags.is_set())
		{
			c->push_collides_with_flags(NAME(beingHeld), self->data->beingHeldCollidesWithFlags.get());
			update = true;
		}
		if (self->data->beingHeldCollisionFlags.is_set())
		{
			c->push_collision_flags(NAME(beingHeld), self->data->beingHeldCollisionFlags.get());
			update = true;
		}
		if (update)
		{
			c->update_collidable_object();
		}
	}

	while (true)
	{
		if (hooksOpenWhenHeld)
		{
			self->hooksOpen.blend_to(0.5f, 0.3f);
		}
		else
		{
			if (self->hooksOpen.state == 0.0f)
			{
				if (additionalWait <= 0.0f)
				{
					self->hooksOpen.blend_to(1.0f, max(0.25f, min(2.0f - 3.0f * proximity, Random::get_float(0.2f, 0.5f))));
					additionalWait = max(0.0f, Random::get_float(-1.0f, 1.0f));
					doAdditionalWait = false;
				}
				else
				{
					doAdditionalWait = true;
				}
			}
			else if (self->hooksOpen.state == 1.0f)
			{
				if (additionalWait <= 0.0f)
				{
					self->hooksOpen.blend_to(0.0f, max(0.15f, min(2.0f - 3.0f * proximity, self->hooksOpen.blendTime * Random::get_float(0.8f, 1.2f))));
					additionalWait = max(0.0f, Random::get_float(-1.0f, 4.0f));
					doAdditionalWait = false;
				}
				else
				{
					doAdditionalWait = true;
				}
			}
		}
		if (self->spitEnergySocket.is_valid())
		{
			if (auto* attached = imo->get_presence()->get_attached_to())
			{
				if (auto* top = attached->get_top_instigator())
				{
					if (auto* pilgrim = top->get_gameplay_as<ModulePilgrim>())
					{
						Transform energyInhaleWS = top->get_presence()->get_placement().to_world(top->get_appearance()->calculate_socket_os(pilgrim->get_energy_inhale_socket().get_index()));
						Transform spitEnergyWS = imo->get_presence()->get_placement().to_world(imo->get_appearance()->calculate_socket_os(self->spitEnergySocket.get_index()));

						bool canCheck = true;
						Transform top2imo = Transform::identity;
						if (imo->get_presence()->get_in_room() != top->get_presence()->get_in_room())
						{
							canCheck = false;
							Framework::PresencePath pp;
							pp.be_temporary_snapshot();
							if (pp.find_path(imo, top, false, 3)) // there shouldn't be more than 3 doors on the way
							{
								canCheck = true;
								energyInhaleWS = pp.from_target_to_owner(energyInhaleWS);
							}
						}
						if (canCheck)
						{
							float dist = (energyInhaleWS.get_translation() - spitEnergyWS.get_translation()).length();
							float dot = Vector3::dot(energyInhaleWS.get_axis(Axis::Y), spitEnergyWS.get_axis(Axis::Y));
							float invProximity;
							invProximity = clamp((dist - 0.1f) / 0.2f, 0.0f, 1.0f);
							invProximity += 0.4f * (0.2f + dot);
							proximity = 1.0f - clamp(invProximity, 0.0f, 1.0f);
							if (dist < 0.05f &&
								dot < -0.8f)
							{
								if (blockSpitting <= 0.0f)
								{
									blockSpitting = Random::get(self->spitEnergyInterval);

									SafePtr<Framework::IModulesOwner> imoSafe;
									imoSafe = imo;
									SafePtr<Framework::IModulesOwner> pilgrimIMOSafe;
									pilgrimIMOSafe = pilgrim->get_owner();

									auto spitEnergyAmount = self->spitEnergyAmount;
									auto spitEnergyAmountMul = self->spitEnergyAmountMul;
									Framework::Game::get()->add_immediate_sync_world_job(TXT("spit energy"),
										[imoSafe, pilgrimIMOSafe, spitEnergyAmount, spitEnergyAmountMul]()
										{
											if (!imoSafe.get() ||
												!pilgrimIMOSafe.get())
											{
												return;
											}
											auto* imo = imoSafe.get();
											auto* pilgrimIMO = pilgrimIMOSafe.get();
											if (auto* pilgrimHealth = pilgrimIMO->get_custom<CustomModules::Health>())
											{
												Energy maxToTake = pilgrimHealth->get_max_total_health() - pilgrimHealth->get_total_health();

												if (maxToTake.is_positive())
												{
													Energy energyToGive = Energy(0);
													Energy energyToUse = Energy(0);

													// calculate how much energy are we going to give and use
													{
														EnergyCoef energyMul = EnergyCoef(1);
														if (auto* h = imo->get_custom<CustomModules::Health>())
														{
															energyToUse = min(spitEnergyAmount.get_random(), h->get_health());
															energyMul = spitEnergyAmountMul;
														}
														if (energyMul.is_positive())
														{
															energyToGive = energyToUse.adjusted(energyMul);
															if (energyToGive > maxToTake)
															{
																energyToGive = maxToTake;
																energyToUse = energyToGive.adjusted_inv(energyMul);
															}
														}
														else
														{
															energyToUse = Energy(0);
														}
													}

													if (energyToGive.is_positive())
													{
														// give to pilgrim
														if (pilgrimHealth)
														{
															EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
															etr.energyRequested = energyToGive;
															pilgrimHealth->handle_health_energy_transfer_request(etr);

															if (auto* to = pilgrimIMO->get_temporary_objects())
															{
																to->spawn_all(NAME(healthInhaled));
															}
														}

														// damage us
														{
															if (auto* s = imo->get_sound())
															{
																s->play_sound(NAME(spit));
															}
															if (auto* h = imo->get_custom<CustomModules::Health>())
															{
																Damage damage;
																damage.damage = energyToUse;
																DamageInfo damageInfo;
																damageInfo.damager = imo;
																damageInfo.source = imo;
																damageInfo.instigator = imo;
																// don't adjust_damage_on_hit_with_extra_effects
																h->deal_damage(damage, damage, damageInfo);
															}
														}

													}
												}
											}
										});
								}
							}
							else
							{
								if (timeToAutoSpitDischarge <= 0.0f /* health check not required, we disintegrate instantly */)
								{
									timeToAutoSpitDischarge = Random::get(self->spitEnergyDischargeInterval);
									if (auto* to = imo->get_temporary_objects())
									{
										to->spawn_all(NAME(spitDischarge));
									}
								}
							}
						}
					}
					else
					{
						timeToAutoSpitDischarge = 0.0f;
					}
				}
			}
			else
			{
				timeToAutoSpitDischarge = 0.0f;
			}
		}
		LATENT_WAIT(max(0.0f, Random::get_float(0.0f, 0.1f)));
	}

	LATENT_ON_BREAK();
	//
	LATENT_ON_END();
	//
	if (auto* c = imo->get_collision())
	{
		bool update = false;
		if (self->data->beingHeldCollidesWithFlags.is_set())
		{
			c->pop_collides_with_flags(NAME(beingHeld));
			update = true;
		}
		if (self->data->beingHeldCollisionFlags.is_set())
		{
			c->pop_collision_flags(NAME(beingHeld));
			update = true;
		}
		if (update)
		{
			c->update_collidable_object();
		}
	}

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Plant::remain_in_pocket)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("remain in pocket"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(SafePtr<Framework::Room>, inRoom);
	LATENT_VAR(Transform, placement);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Plant>(mind->get_logic());

	LATENT_BEGIN_CODE();

	imo->activate_movement(NAME(immovable));

	self->hooksInGround.blend_to(0.0f, 0.1f);
	self->hooksOpen.blend_to(0.0f, 0.1f);
	self->draggingLegActive.blend_to(0.7f, 1.0f);

	if (auto* c = imo->get_collision())
	{
		bool update = false;
		if (self->data->inPocketCollidesWithFlags.is_set())
		{
			c->push_collides_with_flags(NAME(inPocket), self->data->inPocketCollidesWithFlags.get());
			update = true;
		}
		if (self->data->inPocketCollisionFlags.is_set())
		{
			c->push_collision_flags(NAME(inPocket), self->data->inPocketCollisionFlags.get());
			update = true;
		}
		if (update)
		{
			c->update_collidable_object();
		}
	}

	while (true)
	{
		if (auto* p = imo->get_presence())
		{
			inRoom = p->get_in_room();
			placement = p->get_placement();
		}
		LATENT_WAIT(0.5f);
	}

	LATENT_ON_BREAK();
	//
	LATENT_ON_END();
	//
	if (auto* c = imo->get_collision())
	{
		bool update = false;
		if (self->data->inPocketCollidesWithFlags.is_set())
		{
			c->pop_collides_with_flags(NAME(inPocket));
			update = true;
		}
		if (self->data->inPocketCollisionFlags.is_set())
		{
			c->pop_collision_flags(NAME(inPocket));
			update = true;
		}
		if (update)
		{
			c->update_collidable_object();
		}
	}

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Plant::remain_attached)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("remain attached"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::RegisteredLatentTaskHandles, extraTasks);
	LATENT_VAR(SafePtr<Framework::IModulesOwner>, attachedParticles);

	LATENT_VAR(SafePtr<Framework::Room>, inRoom);
	LATENT_VAR(Transform, placement);
	
	LATENT_VAR(float, timeToCheckIfAttached);
	
	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Plant>(mind->get_logic());

	timeToCheckIfAttached -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	timeToCheckIfAttached = 0.5f;

	extraTasks.add(mind, self->data->whileAttached);

	imo->activate_movement(NAME(attached));
	imo->access_variables().access<bool>(NAME(plantAttached)) = true;

	self->hooksInGround.blend_to(1.0f, 0.1f);
	self->hooksOpen.blend_to(0.0f, 0.1f);
	self->draggingLegActive.blend_to(0.0f, 1.5f);

	if (auto* tos = imo->get_temporary_objects())
	{
		//we don't actually attach to doors, that's why we need different particles - ones that quickly clean up after themselves
		attachedParticles = tos->spawn_in_room_attached_to(self->attachedToDoorInRoom.is_set()? NAME(onAttachFastDetach) : NAME(onAttach), imo->get_presence()->get_attached_to());
	}
	if (auto* s = imo->get_sound())
	{
		s->play_sound(NAME(onAttach));
	}

	while (true)
	{
		if (auto* p = imo->get_presence())
		{
			inRoom = p->get_in_room();
			placement = p->get_placement();
		}
		if (timeToCheckIfAttached < 0.0f)
		{
			Framework::CheckCollisionContext checkCollisionContext;
			checkCollisionContext.collision_info_needed();
			checkCollisionContext.use_collision_flags(self->data->attachToCollisionFlags);
			checkCollisionContext.avoid(imo, true);

			checkCollisionContext.start_with_nothing_to_check();
			checkCollisionContext.check_scenery();
			checkCollisionContext.check_room();
			checkCollisionContext.check_room_scenery();

			Transform placementCentre = imo->get_presence()->get_centre_of_presence_transform_WS();
			Transform placementActual = imo->get_presence()->get_placement(); // placement is where we attach, so it is the most reliable way to check

			Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::WS);

			{
				Vector3 traceInDir = - placement.get_axis(Axis::Up);
				float thresholdImmediateTry = 0.05f;
				Vector3 startLoc = placementCentre.get_translation(); // start from the centre, should be fair enough and will start in the actual room
				Vector3 endLoc = placementActual.get_translation() + traceInDir * thresholdImmediateTry;
				collisionTrace.add_location(startLoc);
				collisionTrace.add_location(endLoc);

#ifdef INSPECT__PLANTING_IN_ROOM
				debug_context(imo->get_presence()->get_in_room());
				debug_subject(imo);
				debug_draw_time_based(3.0f, debug_draw_arrow(true, Colour::mint, startLoc, endLoc));
				debug_no_subject();
				debug_no_context();
#endif
			}

			// we only need to know if we hit something
			int collisionTraceFlags = Framework::CollisionTraceFlag::ResultNotRequired;

			// if we didn't hit anything, stop
			Framework::CheckSegmentResult result;
			if (! imo->get_presence()->trace_collision(AgainstCollision::Movement, collisionTrace, result, collisionTraceFlags, checkCollisionContext, nullptr))
			{
#ifdef INSPECT__PLANTING_IN_ROOM
				debug_context(imo->get_presence()->get_in_room());
				debug_subject(imo);
				debug_draw_time_based(3.0f, debug_draw_sphere(true, true, Colour::yellow, 0.5f, Sphere(imo->get_presence()->get_placement().get_translation(), 0.05f)));
				debug_no_subject();
				debug_no_context();
#endif
				self->attachedToRoom = false;
				if (auto* p = imo->get_presence())
				{
					p->detach();
				}
			}

			timeToCheckIfAttached = Random::get_float(0.5f, 3.0f);
		}
		if (self->attachedToDoorInRoom.is_set())
		{
			{
				if (auto* dir = self->attachedToDoorInRoom.get())
				{
					float currentOpenFactor = dir->get_door() ? dir->get_door()->get_open_factor() : 1.0f;
					if (abs(self->attachedToDoorInRoomOpenFactor - currentOpenFactor) > 0.01f)
					{
						ai_log(self, TXT("detached on door movement"));
						self->attachedToRoom = false;
						if (auto* p = imo->get_presence())
						{
							p->detach();
						}
					}
				}
			}
			LATENT_WAIT(0.1f);
		}
		else
		{
			LATENT_WAIT(0.5f);
		}
	}

	LATENT_ON_BREAK();
	//
	LATENT_ON_END();
	//
	if (attachedParticles.is_set())
	{
		if (auto* to = fast_cast<Framework::TemporaryObject>(attachedParticles.get()))
		{
			to->desire_to_deactivate();
		}
		attachedParticles.clear();
	}
	if (auto* tos = imo->get_temporary_objects())
	{
		if (inRoom.is_set())
		{
			attachedParticles = tos->spawn_in_room(NAME(onDetach), inRoom.get(), placement);
		}
		else
		{
			attachedParticles = tos->spawn_in_room(NAME(onDetach));
		}
	}
	if (auto* s = imo->get_sound())
	{
		s->play_sound(NAME(onDetach));
	}
	imo->access_variables().access<bool>(NAME(plantAttached)) = false;

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Plant::open_and_close_hooks)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("open and close hooks"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_PARAM(float*, distanceToGoal);
	LATENT_END_PARAMS();

	auto * self = fast_cast<Plant>(mind->get_logic());

	LATENT_BEGIN_CODE();

	while (true)
	{
		if (self->hooksOpen.state == 0.0f)
		{
			self->hooksOpen.blend_to(1.0f, clamp((*distanceToGoal - 0.3f) / 1.0f, 0.25f, 0.7f) * Random::get_float(0.75f, 1.25f));
		}
		if (self->hooksOpen.state == 1.0f)
		{
			self->hooksOpen.blend_to(0.0f, clamp((*distanceToGoal - 0.3f) / 1.0f, 0.25f, 0.7f) * Random::get_float(0.75f, 1.25f));
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

LATENT_FUNCTION(Plant::try_to_attach)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("try to attach"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, hooksTask);
	LATENT_VAR(::Framework::RelativeToPresencePlacement, goTo);
	LATENT_VAR(Name, goToAttachToBone);
	LATENT_VAR(SafePtr<::Collision::ICollidableObject>, goToCollidableObject);
	LATENT_VAR(::Framework::RelativeToPresencePlacement, goToHelper);
	LATENT_VAR(float, relativeSpeed);
	LATENT_VAR(float, distanceToGoal);
	LATENT_VAR(bool, foundGoal);

	LATENT_VAR(float, keepGoToTime);
	LATENT_VAR(float, timeTryingToAttach);
	LATENT_VAR(bool, tryToAttachImmediately);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Plant>(mind->get_logic());

	keepGoToTime -= LATENT_DELTA_TIME;
	timeTryingToAttach += LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	imo->activate_movement(NAME(flying));

	self->hooksInGround.blend_to(0.0f, 0.5f);

	self->flyTowardsActive.blend_to(1.0f, 0.5f);
	self->draggingLegActive.blend_to(1.0f, 0.5f);
	self->flyTowardsRelativeToPresenceVar.access<Framework::RelativeToPresencePlacement*>() = &goTo;

	timeTryingToAttach = 0.0f;
	tryToAttachImmediately = true;
	distanceToGoal = 1000.0f;

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		if (taskInfo.propose(AI_LATENT_TASK_FUNCTION(open_and_close_hooks)))
		{
			ADD_LATENT_TASK_INFO_PARAM(taskInfo, float*, &distanceToGoal);
		}

		hooksTask.start_latent_task(mind, taskInfo);
	}

	relativeSpeed = Random::get_float(0.8f, 1.2f);

	goTo.set_owner(imo);
	goToHelper.set_owner(imo);

	while (true)
	{
#ifdef INSPECT__PLANTING_IN_ROOM
		if (goTo.is_active())
		{
			debug_context(imo->get_presence()->get_in_room());
			debug_subject(imo);
			debug_draw_arrow(true, Colour::orange, imo->get_presence()->get_centre_of_presence_WS(), goTo.get_placement_in_owner_room().get_translation());
			debug_no_subject();
			debug_no_context();
		}
#endif
		if (imo->get_presence()->get_in_room())
		{
			if (keepGoToTime <= 0.0f)
			{
				goTo.clear_target();
				goToCollidableObject.clear();
				foundGoal = false;
			}
			bool immediateAttachNow = false;
			float thresholdImmediateTry = 0.1f;
			float thrownVelocity = 1.0f;
			// check if is not going anywhere but also is not flying at higher velocity
			bool lookForSomethingToAttachTo = false;
			bool thrownAccordingToVelocity = (imo->get_presence()->get_velocity_linear().length_squared() > sqr(thrownVelocity));
			if (!goTo.is_active())
			{
				lookForSomethingToAttachTo = !thrownAccordingToVelocity;
			}
			// check if collides and has a higher velocity, if so, attach immediately
			Vector3 collisionGradient = Vector3::zero;
			if (auto* mc = imo->get_collision())
			{
				collisionGradient = mc->get_gradient();
				if (!collisionGradient.is_zero())
				{
					collisionGradient = collisionGradient.normal();
					Transform placement = imo->get_presence()->get_centre_of_presence_transform_WS();
					// if moving slowly but more or less collision gradient is aligned with up dir, keep it. otherwise clear gradient
					if (!thrownAccordingToVelocity && Vector3::dot(collisionGradient, placement.get_axis(Axis::Up)) < 0.8f)
					{
						collisionGradient = Vector3::zero;;
					}
				}
			}
			if (lookForSomethingToAttachTo || !collisionGradient.is_zero())
			{
				Vector3 gravityDir = imo->get_presence()->get_gravity_dir();

				Framework::CheckCollisionContext checkCollisionContext;
				checkCollisionContext.collision_info_needed();
				checkCollisionContext.use_collision_flags(self->data->attachToCollisionFlags);
				checkCollisionContext.avoid(imo, true);

				checkCollisionContext.start_with_nothing_to_check();
				checkCollisionContext.check_scenery();
				checkCollisionContext.check_room();
				checkCollisionContext.check_room_scenery();

				Transform placementCentre = imo->get_presence()->get_centre_of_presence_transform_WS();
				Transform placementActual = imo->get_presence()->get_placement(); // placement is where we attach, so it is the most reliable way to check

				Transform placementUse = placementCentre;

				Transform ownerRelativeOffset = Transform::identity;
				Vector3 traceInDir;

				Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::WS);

				bool attachNowIfPossible = tryToAttachImmediately;
				if (!collisionGradient.is_zero())
				{
					traceInDir = -collisionGradient;
					thresholdImmediateTry = 0.2f;
					Vector3 startLoc = placementCentre.get_translation(); // start from the centre, should be fair enough and will start in the actual room
					Vector3 endLoc = placementActual.get_translation() + traceInDir * thresholdImmediateTry;
					collisionTrace.add_location(startLoc);
					collisionTrace.add_location(endLoc);
					attachNowIfPossible = true;
				}
				else if (tryToAttachImmediately)
				{
					ownerRelativeOffset = imo->get_appearance()->calculate_socket_os(self->attachPointSocket.get_index());
					if (self->attachPointSocket.is_valid())
					{
						placementUse = imo->get_presence()->get_placement();
						placementUse = placementUse.to_world(ownerRelativeOffset);
					}

					traceInDir = -placementUse.get_axis(Axis::Z);
					collisionTrace.add_location(placementUse.get_translation() - traceInDir * 0.1f);
					collisionTrace.add_location(placementUse.get_translation() + traceInDir * thresholdImmediateTry); // we won't need to check distance!
				}
				else
				{
					collisionTrace.add_location(placementCentre.get_translation());

					traceInDir = gravityDir.normal() + imo->get_presence()->get_placement().get_axis(Axis::Z) * -0.3f;
					traceInDir += Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f)).normal() * 0.4f;
					traceInDir.normalise();

					collisionTrace.add_location(placementActual.get_translation() + traceInDir);
				}
				
				ai_log(self, TXT("look for place to attach to"));
				ai_log_increase_indent(self);
				Framework::CheckSegmentResult result;
				if (imo->get_presence()->trace_collision(AgainstCollision::Movement, collisionTrace, result, Framework::CollisionTraceFlag::ResultInFinalRoom, checkCollisionContext, &goTo) && !result.hitNormal.is_zero())
				{
					ai_log(self, TXT("hit something"));
					if (result.has_collision_flags(self->data->dontAttachToCollisionFlags, false))
					{
						ai_log(self, TXT("but shouldn't attach to it"));
					}
					else
					{
						Vector3 up = result.hitNormal;
						Vector3 right = placementUse.get_axis(Axis::X);
						if (abs(Vector3::dot(up, right)) >= 0.9f)
						{
							right = placementUse.get_axis(Axis::Y); // will turn
						}
						if (abs(Vector3::dot(up, right)) < 0.9f)
						{
							ai_log(self, TXT("good one!"));
							goTo.set_placement_in_final_room(matrix_from_up_right(result.hitLocation, up, right).to_transform(), cast_to_nonconst(fast_cast<Framework::IModulesOwner>(result.object)));
#ifdef INSPECT__PLANTING_IN_ROOM
							debug_context(result.inRoom);
							debug_subject(imo);
							debug_draw_time_based(120.0f, debug_draw_arrow(true, Colour::mint, result.hitLocation, result.hitLocation + up * 0.4f));
							debug_draw_time_based(120.0f, debug_draw_arrow(true, Colour::mint, result.hitLocation, result.hitLocation + right * 0.2f));
							debug_no_subject();
							debug_no_context();
#endif
							goToAttachToBone = result.shape ? result.shape->get_collidable_shape_bone() : Name::invalid();
							goToCollidableObject = result.object;
							keepGoToTime = Random::get_float(8.0f, 12.0f);
							foundGoal = true;
						}
						else
						{
							ai_log(self, TXT("skip it"));
							keepGoToTime = Random::get_float(0.1f, 0.2f);
							foundGoal = false;
						}
						if (attachNowIfPossible)
						{
#ifdef AN_DEVELOPMENT
#ifndef AN_CLANG
							float dist = goTo.calculate_string_pulled_distance(ownerRelativeOffset);
#endif
#endif

							if (foundGoal)
							{
								ai_log(self, TXT("force immediate attach!"));
								immediateAttachNow = true;

								Vector3 prevHitNormalOWS = result.intoInRoomTransform.vector_to_local(result.hitNormal);

								Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::WS);
								collisionTrace.add_location(placementCentre.get_translation());
								// use mix of trace in dir and normal to get in that direction but closer to wall
								collisionTrace.add_location(placementCentre.get_translation() + (traceInDir * 0.15f - prevHitNormalOWS * 0.85f).normal() * thresholdImmediateTry);
								goToHelper.clear_target();
								if (imo->get_presence()->trace_collision(AgainstCollision::Movement, collisionTrace, result, Framework::CollisionTraceFlag::ResultInFinalRoom, checkCollisionContext, &goToHelper) && !result.hitNormal.is_zero())
								{
									Vector3 newHitNormalOWS = result.intoInRoomTransform.vector_to_local(result.hitNormal);
									if (Vector3::dot(newHitNormalOWS, prevHitNormalOWS) > 0.9f)
									{
										Vector3 up = result.hitNormal;
										Vector3 right = placementUse.get_axis(Axis::X);
										if (abs(Vector3::dot(up, right)) >= 0.9f)
										{
											right = placementUse.get_axis(Axis::Y); // will turn
										}
										if (abs(Vector3::dot(up, right)) < 0.9f)
										{
											ai_log(self, TXT("force immediate attach at closer location!"));
											goTo = goToHelper;
											goToHelper.clear_target();
											// set as new go to location
											goTo.set_placement_in_final_room(matrix_from_up_right(result.hitLocation, up, right).to_transform(), cast_to_nonconst(fast_cast<Framework::IModulesOwner>(result.object)));
											goToAttachToBone = result.shape ? result.shape->get_collidable_shape_bone() : Name::invalid(); 
											goToCollidableObject = result.object;
											keepGoToTime = Random::get_float(0.1f, 0.2f);
										}
									}
								}
							}
						}
					}
				}
				else if (!goTo.is_active())
				{
					ai_log(self, TXT("just go"));
					Transform targetPlacement = matrix_from_up_right(placementUse.get_translation() + traceInDir * 2.0f, -traceInDir, placementUse.get_axis(Axis::X)).to_transform();
					goTo.set_placement_in_final_room(targetPlacement);
					goToCollidableObject.clear();
					keepGoToTime = Random::get_float(0.3f, 0.8f);
					foundGoal = false;
				}
				ai_log_decrease_indent(self);

				if (lookForSomethingToAttachTo)
				{
					relativeSpeed = Random::get_float(0.8f, 1.2f);

					if (!foundGoal && tryToAttachImmediately)
					{
						// try again next frame
						tryToAttachImmediately = false;
						goTo.clear_target();
						goToCollidableObject.clear();
					}
				}
			}
			if (goTo.is_active())
			{
				Transform ownerRelativeOffset = Transform::identity;
				if (self->attachPointSocket.is_valid())
				{
					ownerRelativeOffset = imo->get_appearance()->calculate_socket_os(self->attachPointSocket.get_index());
				}
				distanceToGoal = goTo.calculate_string_pulled_distance(ownerRelativeOffset);
				float const threshold = 0.0025f;

				auto& locomotion = mind->access_locomotion();
				Vector3 targetLoc = goTo.get_placement_in_owner_room().location_to_world(-ownerRelativeOffset.get_translation());
				Vector3 correctedTargetLoc = targetLoc;
				if (self->flyDirSocket.is_valid())
				{
					Transform placement = imo->get_presence()->get_placement();
					Vector3 flyDir = placement.vector_to_world(imo->get_appearance()->calculate_socket_os(self->flyDirSocket.get_index()).get_axis(Axis::Y));
					correctedTargetLoc = placement.get_translation() + (targetLoc - placement.get_translation()).keep_within_angle(flyDir, 0.95f);
				}

				locomotion.move_to_3d(Vector3::lerp(clamp(distanceToGoal / 0.3f, 0.0f, 0.8f), targetLoc, correctedTargetLoc), threshold * 0.5f, Framework::MovementParameters().relative_speed(relativeSpeed * clamp(distanceToGoal / 0.1f, 0.2f, 1.0f)));
				locomotion.allow_any_turn();

				if (!foundGoal && distanceToGoal <= 0.3f)
				{
					goTo.clear_target();
					goToCollidableObject.clear();
				}
				else if (foundGoal && (distanceToGoal <= 0.3f || immediateAttachNow))
				{
					bool attach = false;
					if (immediateAttachNow) // this way we may plant it anywhere we want without waiting for it to attach
					{
						attach = true;
					}
					else if (distanceToGoal <= threshold)
					{
						// check if we are going to attach to the same target!
						Framework::CheckCollisionContext checkCollisionContext;
						checkCollisionContext.collision_info_needed();
						checkCollisionContext.use_collision_flags(self->data->attachToCollisionFlags);
						checkCollisionContext.avoid(imo, true);

						checkCollisionContext.start_with_nothing_to_check();
						checkCollisionContext.check_scenery();
						checkCollisionContext.check_room();
						checkCollisionContext.check_room_scenery();

						Transform centrePlacement = imo->get_presence()->get_centre_of_presence_transform_WS();

						Transform placement = imo->get_presence()->get_placement();
						Vector3 flyDir = placement.vector_to_world(imo->get_appearance()->calculate_socket_os(self->flyDirSocket.get_index()).get_axis(Axis::Y));

						Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::WS);
						collisionTrace.add_location(centrePlacement.get_translation());
						collisionTrace.add_location(placement.get_translation() + flyDir);

						Framework::CheckSegmentResult result;
						if (imo->get_presence()->trace_collision(AgainstCollision::Movement, collisionTrace, result, Framework::CollisionTraceFlag::ResultInFinalRoom, checkCollisionContext, &goTo) && !result.hitNormal.is_zero())
						{
							if (!result.has_collision_flags(self->data->dontAttachToCollisionFlags, false))
							{
								auto* hitImo = fast_cast<Framework::IModulesOwner>(result.object);
								if (goTo.get_target() == hitImo)
								{
									attach = true;
								}
							}
						}
					}

					if (attach)
					{
						ai_log(self, TXT("attach! to \"%S\""), fast_cast<Framework::IAIObject>(goToCollidableObject.get())? fast_cast<Framework::IAIObject>(goToCollidableObject.get())->ai_get_name().to_char() : TXT("--"));

						self->attachedToDoorInRoom.clear();
						if (auto* dir = fast_cast<Framework::DoorInRoom>(goToCollidableObject.get()))
						{
							if (auto* d = dir->get_door())
							{
								if (d->is_open_factor_still())
								{
									self->attachedToDoorInRoom = dir;
									self->attachedToDoorInRoomOpenFactor = dir->get_door() ? dir->get_door()->get_open_factor() : 1.0f;
								}
								else
								{
									ai_log(self, TXT("don't attach. we would attach to a door that is either opening or closing"));
									attach = false;
								}
							}
						}

						if (attach)
						{
							self->attachedToCollidableObject = goToCollidableObject;
							if (goTo.get_target())
							{
								if (immediateAttachNow)
								{
									ai_log(self, TXT("immediate attach to a target \"%S\""), goTo.get_target()->ai_get_name().to_char());
									imo->get_presence()->request_attach_to_bone_using_in_room_placement(goTo.get_target(), goToAttachToBone, false, goTo.get_placement_in_final_room());
								}
								else
								{
									ai_log(self, TXT("non-immediate attach to a target \"%S\""), goTo.get_target()->ai_get_name().to_char());
									imo->get_presence()->request_attach_to_bone_using_in_room_placement(goTo.get_target(), goToAttachToBone, false, goTo.from_owner_to_target(imo->get_presence()->get_placement()));
								}
							}
							else
							{
								if (immediateAttachNow)
								{
									ai_log(self, TXT("immediate attach to room \"%S\""), goTo.get_in_final_room()? goTo.get_in_final_room()->get_name().to_char() : TXT("--"));
									imo->get_presence()->request_teleport(goTo.get_in_final_room(), goTo.get_placement_in_final_room());
								}
								self->attachedToRoom = true;
							}
						}
						else
						{
							goTo.clear_target();
							goToCollidableObject.clear();
							foundGoal = false;
						}
					}
					else
					{
						self->flyTowardsActive.blend_to(0.0f, 2.0f);
						self->draggingLegActive.blend_to(0.5f, 2.0f);
						imo->get_controller()->set_requested_orientation(goTo.get_placement_in_owner_room().get_orientation());
					}
				}
				else
				{
					self->flyTowardsActive.blend_to(1.0f, 0.5f);
					self->draggingLegActive.blend_to(1.0f, 0.5f);
					Transform placement = imo->get_presence()->get_centre_of_presence_transform_WS();
					Vector3 to = goTo.get_placement_in_owner_room().get_translation();
					imo->get_controller()->set_requested_orientation(matrix_from_up_right(to, (placement.get_translation() - to).normal(), placement.get_axis(Axis::X)).to_quat());
				}
			}
		}
		tryToAttachImmediately = false;
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	self->flyTowardsRelativeToPresenceVar.access<Framework::RelativeToPresencePlacement*>() = nullptr;
	self->flyTowardsActive.blend_to(0.0f, 2.0f);
	hooksTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Plant::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);
	LATENT_VAR(::Framework::AI::RegisteredLatentTaskHandles, extraTasks);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Plant>(logic);

	LATENT_BEGIN_CODE();

	self->asEquipment = imo->get_gameplay_as<ModuleEquipment>();

	self->hooksInGround.set_variable<float>(imo, self->data->hooksInGroundVar);
	self->hooksOpen.set_variable<float>(imo, self->data->hooksOpenVar);
	self->attachPointSocket.set_name(self->data->attachPointSocket.get(imo->access_variables()));
	self->flyDirSocket.set_name(self->data->flyDirSocket.get(imo->access_variables()));
	self->flyTowardsActive.set_variable<float>(imo, self->data->flyTowardsActiveVar);
	self->flyTowardsRelativeToPresenceVar.set_name(self->data->flyTowardsRelativeToPresenceVar.get(imo->access_variables()));
	self->draggingLegActive.set_variable<float>(imo, self->data->draggingLegActiveVar);
	if (self->data->spitEnergySocket.is_set())
	{
		self->spitEnergySocket.set_name(self->data->spitEnergySocket.get(imo->access_variables()));
		self->spitEnergyInterval = self->data->spitEnergyInterval.get(imo->access_variables());
		self->spitEnergyAmount = self->data->spitEnergyAmount.get(imo->access_variables());
		self->spitEnergyAmountMul = self->data->spitEnergyAmountMul.get(imo->access_variables());
		self->spitEnergyDischargeInterval = self->data->spitEnergyDischargeInterval.get(imo->access_variables());
	}
	
	self->attachPointSocket.look_up(imo->get_appearance()->get_mesh());
	self->flyDirSocket.look_up(imo->get_appearance()->get_mesh());
	self->flyTowardsRelativeToPresenceVar.look_up<Framework::RelativeToPresencePlacement*>(imo->access_variables());
	self->spitEnergySocket.look_up(imo->get_appearance()->get_mesh());

	extraTasks.add(mind, self->data->duringMain);

	if (auto* imo = mind->get_owner_as_modules_owner())
	{
		if (!imo->get_variables().get_value<bool>(NAME(lootObject), false))
		{
			imo->get_presence()->request_on_spawn_random_dir_teleport(4.5f, 0.1f, 0.1f);
		}
	}

	while (true)
	{
		{
			self->hooksInGround.update(LATENT_DELTA_TIME, blend_to_using_speed_based_on_time<float>, BlendCurve::cubic);
			self->hooksOpen.update(LATENT_DELTA_TIME, blend_to_using_speed_based_on_time<float>, BlendCurve::cubic);
			self->flyTowardsActive.update(LATENT_DELTA_TIME, blend_to_using_speed_based_on_time<float>, BlendCurve::cubic);
			self->draggingLegActive.update(LATENT_DELTA_TIME, blend_to_using_speed_based_on_time<float>, BlendCurve::cubic);
		}

		{
			::Framework::AI::LatentTaskInfoWithParams nextTaskInfo;
			nextTaskInfo.clear();
			auto* pickup = imo->get_custom<CustomModules::Pickup>();
			if (self->asEquipment->get_user() ||
				(pickup && pickup->is_held()))
			{
				nextTaskInfo.propose(AI_LATENT_TASK_FUNCTION(being_held), NP, NP, NP, true);
			}
			else
			{
				if (pickup && (pickup->is_in_pocket() || pickup->is_in_holder()))
				{
					nextTaskInfo.propose(AI_LATENT_TASK_FUNCTION(remain_in_pocket));
				}
				else if (imo->get_presence()->is_attached() || self->attachedToRoom)
				{
					nextTaskInfo.propose(AI_LATENT_TASK_FUNCTION(remain_attached));
				}
				else
				{
					nextTaskInfo.propose(AI_LATENT_TASK_FUNCTION(try_to_attach));
				}
			}
			if (currentTask.can_start(nextTaskInfo))
			{
				currentTask.start_latent_task(mind, nextTaskInfo);
			}
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(PlantData);

bool PlantData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= hooksInGroundVar.load_from_xml(_node, TXT("hooksInGroundVarID"));
	result &= hooksOpenVar.load_from_xml(_node, TXT("hooksOpenVarID"));
	result &= attachPointSocket.load_from_xml(_node, TXT("attachPointSocket"));
	result &= flyDirSocket.load_from_xml(_node, TXT("flyDirSocket"));
	result &= flyTowardsActiveVar.load_from_xml(_node, TXT("flyTowardsActiveVarID"));
	result &= flyTowardsRelativeToPresenceVar.load_from_xml(_node, TXT("flyTowardsRelativeToPresenceVarID"));
	result &= draggingLegActiveVar.load_from_xml(_node, TXT("draggingLegActiveVarID"));
	result &= spitEnergySocket.load_from_xml(_node, TXT("spitEnergySocket"));
	result &= spitEnergyInterval.load_from_xml(_node, TXT("spitEnergyInterval"));
	result &= spitEnergyAmount.load_from_xml(_node, TXT("spitEnergyAmount"));
	result &= spitEnergyAmountMul.load_from_xml(_node, TXT("spitEnergyAmountMul"));	
	result &= spitEnergyDischargeInterval.load_from_xml(_node, TXT("spitEnergyDischargeInterval"));

	attachToCollisionFlags.load_from_xml(_node, TXT("attachToCollisionFlags"));
	dontAttachToCollisionFlags.load_from_xml(_node, TXT("dontAttachToCollisionFlags"));
	{
		String flags = _node->get_string_attribute_or_from_child(TXT("beingHeldCollidesWithFlags"), String::empty());
		if (!flags.is_empty())
		{
			beingHeldCollidesWithFlags = Collision::Flags::none();
			beingHeldCollidesWithFlags.access().apply(flags);
		}
	}	
	{
		String flags = _node->get_string_attribute_or_from_child(TXT("beingHeldCollisionFlags"), String::empty());
		if (!flags.is_empty())
		{
			beingHeldCollisionFlags = Collision::Flags::none();
			beingHeldCollisionFlags.access().apply(flags);
		}
	}
	{
		String flags = _node->get_string_attribute_or_from_child(TXT("inPocketCollidesWithFlags"), String::empty());
		if (!flags.is_empty())
		{
			inPocketCollidesWithFlags = Collision::Flags::none();
			inPocketCollidesWithFlags.access().apply(flags);
		}
	}	
	{
		String flags = _node->get_string_attribute_or_from_child(TXT("inPocketCollisionFlags"), String::empty());
		if (!flags.is_empty())
		{
			inPocketCollisionFlags = Collision::Flags::none();
			inPocketCollisionFlags.access().apply(flags);
		}
	}

	for_every(node, _node->children_named(TXT("tasks")))
	{
		result &= duringMain.load_from_xml(node, TXT("duringMain"));
		result &= whileAttached.load_from_xml(node, TXT("whileAttached"));
	}

	return result;
}