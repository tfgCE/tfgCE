#include "aiLogic_charger.h"

#include "components\aiLogicComponent_confussion.h"

#include "tasks\aiLogicTask_announcePresence.h"
#include "tasks\aiLogicTask_attackOrIdle.h"
#include "tasks\aiLogicTask_beingHeld.h"
#include "tasks\aiLogicTask_beingThrown.h"
#include "tasks\aiLogicTask_evadeAiming.h"
#include "tasks\aiLogicTask_grabEnergy.h"
#include "tasks\aiLogicTask_idle.h"
#include "tasks\aiLogicTask_wander.h"

#include "..\aiCommon.h"
#include "..\aiCommonVariables.h"
#include "..\aiRayCasts.h"
#include "..\perceptionRequests\aipr_FindInVisibleRooms.h"

#include "..\..\game\damage.h"
#include "..\..\game\game.h"
#include "..\..\game\gameSettings.h"
#include "..\..\library\library.h"
#include "..\..\modules\custom\mc_pickup.h"
#include "..\..\modules\custom\health\mc_health.h"
#include "..\..\modules\gameplay\moduleDuctEnergy.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiSocial.h"
#include "..\..\..\framework\appearance\controllers\ac_particlesUtils.h"
#include "..\..\..\framework\game\gameConfig.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\moduleMovement.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\module\custom\mc_lightningSpawner.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\nav\navLatentUtils.h"
#include "..\..\..\framework\nav\navPath.h"
#include "..\..\..\framework\nav\navTask.h"
#include "..\..\..\framework\nav\tasks\navTask_FindPath.h"
#include "..\..\..\framework\nav\tasks\navTask_GetRandomLocation.h"
#include "..\..\..\framework\object\scenery.h"
#include "..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\inRoomPlacement.h"
#include "..\..\..\framework\world\presenceLink.h"
#include "..\..\..\framework\world\room.h"

#include "..\..\..\core\performance\performanceUtils.h"

#include "..\..\modules\custom\mc_emissiveControl.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

//#define DEBUG_DRAW_THROWING

//

// ai message names
DEFINE_STATIC_NAME(dealtDamage);
DEFINE_STATIC_NAME(die);
DEFINE_STATIC_NAME(remoteControl);

// ai message params
DEFINE_STATIC_NAME(damageSource);
DEFINE_STATIC_NAME(location);
DEFINE_STATIC_NAME(throughDoor);
DEFINE_STATIC_NAME(room);
DEFINE_STATIC_NAME(who);

// socket
DEFINE_STATIC_NAME(projectile);

// lightnings
DEFINE_STATIC_NAME(idle);
DEFINE_STATIC_NAME(discharge);
DEFINE_STATIC_NAME_STR(dischargeMiss, TXT("discharge miss"));
DEFINE_STATIC_NAME_STR(dischargesDying, TXT("discharges dying"));

// temporary objects
DEFINE_STATIC_NAME_STR(lightningHit, TXT("lightning hit"));
DEFINE_STATIC_NAME_STR(ductEnergy, TXT("duct energy"));
DEFINE_STATIC_NAME(dying);
DEFINE_STATIC_NAME(explode);

// sounds / emissive
DEFINE_STATIC_NAME(alert);
DEFINE_STATIC_NAME(attack);

// variables
DEFINE_STATIC_NAME(dischargeDamage);
DEFINE_STATIC_NAME(hatchOpen);
DEFINE_STATIC_NAME(deathExplosionCount);

// gaits
DEFINE_STATIC_NAME(run);

/// devices
DEFINE_STATIC_NAME(hatchOpenSliderInteractiveDeviceId);

// cooldowns
DEFINE_STATIC_NAME(collision);
DEFINE_STATIC_NAME(playAttackSound);
DEFINE_STATIC_NAME(findCloseOne);

// global references
DEFINE_STATIC_NAME_STR(grLightningProjectileTypeTags, TXT("projectile; lightning; projectile type tags"));

// health extra effect
DEFINE_STATIC_NAME(overcharged);

//

REGISTER_FOR_FAST_CAST(Charger);

Charger::Charger(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

Charger::~Charger()
{
}

void Charger::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	deathExplosionCount = _parameters.get_value<int>(NAME(deathExplosionCount), deathExplosionCount);
}

void Charger::drop_target()
{
	newKeepAttackingFor = true;
	keepAttackingFor = 0.0f;
	possibleTarget.clear_target();
	hitEnemy = false;
}

void Charger::keep_attacking_for(float _keepAttackingFor, Framework::RelativeToPresencePlacement const & _r2pp, float _blockNewKeepAttackingFor, int _keepAttackingForPriority)
{
	if (blockNewKeepAttackingFor > 0.0f && keepAttackingForPriority >= _keepAttackingForPriority)
	{
		return;
	}
	newKeepAttackingFor = true;
	keepAttackingFor = _keepAttackingFor;
	if (auto * t = _r2pp.get_target())
	{
		possibleTarget.find_path(_r2pp.get_owner(), t->get_presence()->get_in_room(), t->get_presence()->get_placement().to_world(Transform(t->get_presence()->get_random_point_within_presence_os(0.5f), Quat::identity)));
	}
	else
	{
		possibleTarget = _r2pp;
	}
	hitEnemy = false;
	ai_log(this, TXT("new keep attacking for %.3f"), keepAttackingFor);

	blockNewKeepAttackingFor = _blockNewKeepAttackingFor;
}

void Charger::play_sound(Name const & _sound)
{
	if (auto* s = get_mind()->get_owner_as_modules_owner()->get_sound())
	{
		s->play_sound(_sound);
	}
}

void Charger::advance(float _deltaTime)
{
	base::advance(_deltaTime);
#ifdef AN_DEVELOPMENT
#ifndef AN_CLANG
	float prevKeepAttackingFor = keepAttackingFor;
#endif
#endif
	keepAttackingFor = max(0.0f, keepAttackingFor - _deltaTime);
	blockNewKeepAttackingFor = max(0.0f, blockNewKeepAttackingFor - _deltaTime);
}

LATENT_FUNCTION(Charger::perception_blind)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai charger] perception blind"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("perception blind"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::CooldownsSmall, cooldowns);
	LATENT_VAR(::Framework::IModulesOwner const *, lastTarget);
	LATENT_VAR(::Framework::IModulesOwner const *, newTarget);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	LATENT_VAR(Confussion, confussion);

	auto* imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Charger>(mind->get_logic());

	confussion.advance(LATENT_DELTA_TIME);

	cooldowns.advance(LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	confussion.setup(mind);

	messageHandler.use_with(mind);
	{
		auto * framePtr = &_frame;
		messageHandler.set(NAME(dealtDamage), [framePtr, mind, imo, self](Framework::AI::Message const & _message)
		{
			if (auto * source = _message.get_param(NAME(damageSource)))
			{
				if (auto* damageInstigator = source->get_as<SafePtr<Framework::IModulesOwner>>().get())
				{
					ai_log_colour(mind->get_logic(), Colour::cyan);
					ai_log(mind->get_logic(), TXT("[perception] damage source (%S)"), damageInstigator->ai_get_name().to_char());
					ai_log_no_colour(mind->get_logic());
					damageInstigator = damageInstigator->get_instigator_first_actor_or_valid_top_instigator();

					if (!self->keepAttackingFor)
					{
						ai_log_colour(mind->get_logic(), Colour::cyan);
						Framework::RelativeToPresencePlacement r2pp;
						r2pp.be_temporary_snapshot();
						if (r2pp.find_path(imo, damageInstigator))
						{
							self->keep_attacking_for(Random::get_float(20.0f, 25.0f), r2pp, 3.0f, 2);
							self->play_sound(NAME(alert));
						}
						ai_log_no_colour(mind->get_logic());
					}
				}
			}
			framePtr->end_waiting();
			AI_LATENT_STOP_LONG_RARE_ADVANCE();
		}
		);
	}

	while (true)
	{
		if (confussion.is_confused())
		{
			self->drop_target();
		}
		else if (cooldowns.is_available(NAME(collision)))
		{
			PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai charger] collision"));
			if (auto* mc = imo->get_collision())
			{
				if (auto* ai = imo->get_ai())
				{
					auto const & aiSocial = ai->get_mind()->get_social();
					if (!self->keepAttackingFor)
					{
						lastTarget = nullptr;
					}
					newTarget = lastTarget;
					for_every(oc, mc->get_collided_with())
					{
						if (Framework::IModulesOwner const * object = fast_cast<Framework::IModulesOwner>(oc->collidedWithObject.get()))
						{
							object = object->get_instigator_first_actor_or_valid_top_instigator();

							if (aiSocial.is_enemy(object))
							{
								cooldowns.set(NAME(collision), 1.0f);

								::Framework::RelativeToPresencePlacement r2pp;
								r2pp.be_temporary_snapshot(); 
								if (r2pp.find_path(imo, cast_to_nonconst(object)))
								{
									Vector3 relLocation = r2pp.get_target_centre_relative_to_owner();
									Vector3 flatDir = (relLocation * Vector3::xy).normal();
									ai_log(mind->get_logic(), TXT("flatDir.y %.3f"), flatDir.y);
									if (flatDir.y > -0.4f)
									{
										if (!lastTarget || Random::get_chance(0.2f))
										{
											ai_log(mind->get_logic(), TXT("collided, target!"));
											self->keep_attacking_for(Random::get_float(20.0f, 30.0f), r2pp, 3.0f, 3);
											self->play_sound(NAME(alert));
											lastTarget = object;
										}
									}
								}
							}
						}
					}
					lastTarget = newTarget;
				}
			}
		}

		LATENT_WAIT(Random::get_float(0.1f, 0.3f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	//

	LATENT_RETURN();
}

LATENT_FUNCTION(Charger::hatch)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai charger] hatch"));

	/**
	*/
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("hatch"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(SimpleVariableInfo, hatchOpenVar);
	LATENT_VAR(SafePtr<Framework::IModulesOwner>, ductEnergy);
	LATENT_VAR(bool, hatchWasOpen);
	LATENT_VAR(float, forceHatchOpen);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto* imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Charger>(mind->get_logic());

	forceHatchOpen = max(0.0f, forceHatchOpen - LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	self->switchHandler.initialise(imo, NAME(hatchOpenSliderInteractiveDeviceId));
	hatchOpenVar = imo->access_variables().find<float>(NAME(hatchOpen));

	if (!ductEnergy.get())
	{
		if (auto* to = imo->get_temporary_objects())
		{
			ductEnergy = to->spawn(NAME(ductEnergy));
		}
	}

	messageHandler.use_with(mind);
	{
		auto* framePtr = &_frame;
		messageHandler.set(NAME(remoteControl), [framePtr, mind, &forceHatchOpen](Framework::AI::Message const& _message)
			{
				// force switch off deflection
				forceHatchOpen = REMOTE_CONTROL_EFFECT_TIME;

				framePtr->end_waiting();
				AI_LATENT_STOP_LONG_RARE_ADVANCE();
			}
		);
	}

	LATENT_WAIT(0.1f);

	ModuleDuctEnergy::be_active(ductEnergy.get(), false);

	while (true)
	{
#ifndef BUILD_PUBLIC_RELEASE
		{
			static bool sentOnce = false;
			if (!sentOnce)
			{
				int switchCount = 0;
				int missingSwitchCount = 0;
				for_every(imoPtr, self->switchHandler.get_switches())
				{
					if (imoPtr->get())
					{
						++switchCount;
					}
					else
					{
						++missingSwitchCount;
					}
				}
				if (!switchCount || missingSwitchCount)
				{
					output(TXT("charger is missing it handle o%p"), imo);
					error_dev_investigate(TXT("charger is missing it handle o%p"), imo);
					sentOnce = true;
					String shortText = String::printf(TXT("Charger is missing its handle"));
					String moreInfo;
					{
						moreInfo += TXT("Charger is missing its handle\n");
						moreInfo += String::printf(TXT("charger: o%p\n"), imo);
					}
					if (auto* g = Game::get())
					{
						g->send_full_log_info_in_background(shortText, moreInfo);
					}
				}
			}
		}
#endif
		self->switchHandler.advance(LATENT_DELTA_TIME);
		{
			bool isOpen = self->switchHandler.is_on() || forceHatchOpen > 0.0f;
			/*
			{
				static bool ope = false;
				static float tim = 0.0f;
				tim -= LATENT_DELTA_TIME;
				if (tim < 0.0f)
				{
					ope = !ope;
					tim = 6.0f;
				}
				isOpen = ope;
			}
			*/
			/*
			if (isOpen && !hatchWasOpen)
			{
				Framework::RelativeToPresencePlacement r2pp;
				if (r2pp.find_path(imo, imo->get_presence()->get_in_room(), imo->get_presence()->get_placement().to_world(Transform(Vector3::yAxis * 5.0f, Quat::identity))))
				{
					ai_log(mind->get_logic(), TXT("look behind!")); 
					self->keep_attacking_for(5.0f, r2pp);
				}
			}
			*/
			hatchOpenVar.access<float>() = isOpen ? 1.0f : 0.0f;
			if (isOpen ^ hatchWasOpen)
			{
				ModuleDuctEnergy::be_active(ductEnergy.get(), isOpen);
			}
			hatchWasOpen = isOpen;
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	ModuleDuctEnergy::be_active(ductEnergy.get(), false);
	if (auto* de = ductEnergy.get())
	{
		Framework::ParticlesUtils::desire_to_deactivate(de);
		ductEnergy.clear();
	}

	LATENT_END_CODE();
	//

	LATENT_RETURN();
}

LATENT_FUNCTION(Charger::discharges)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai charger] discharges"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("discharges"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::CooldownsSmall, cooldowns);
	LATENT_VAR(::Framework::AI::PerceptionRequestPtr, perceptionRequest); // one perception should be enough
	LATENT_VAR(::Framework::PresencePath, aimAt);
	LATENT_VAR(Framework::CustomModules::LightningSpawner*, lightningSpawner);
	LATENT_VAR(float, dischargeMoreOften);
	LATENT_VAR(bool, dischargedAndHurt);
	LATENT_VAR(float, dischargePower);
	LATENT_VAR(bool, overDischarged);
	LATENT_VAR(Random::Generator, rg);

	LATENT_VAR(Tags, lightningProjectileTypeTags);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<Charger>(mind->get_logic());
	auto* npcBase = fast_cast<NPCBase>(mind->get_logic());

	dischargePower = max(0.0f, dischargePower - LATENT_DELTA_TIME);
	if (dischargePower >= magic_number 10.0f)
	{
		overDischarged = true;
	}
	else if (dischargePower <= 0.0f)
	{
		overDischarged = false;
	}

	cooldowns.advance(LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	lightningProjectileTypeTags = Library::get_current()->get_global_references().get_tags(NAME(grLightningProjectileTypeTags));

	lightningSpawner = imo->get_custom<Framework::CustomModules::LightningSpawner>();

	while (true)
	{
		LATENT_CLEAR_LOG();
		if (!perceptionRequest.is_set() &&
			cooldowns.is_available(NAME(findCloseOne)))
		{
			auto* ai = imo->get_ai();
			perceptionRequest = mind->access_perception().add_request(new AI::PerceptionRequests::FindInVisibleRooms(imo, [ai](Framework::IModulesOwner const* _object)
			{
				return ai->get_mind()->get_social().is_enemy(_object);
			}));
		}
		if (perceptionRequest.is_set() &&
			perceptionRequest->is_processed())
		{
			LATENT_LOG(TXT("request procesed"));
			aimAt.clear_target();
			if (auto * request = fast_cast<AI::PerceptionRequests::FindInVisibleRooms>(perceptionRequest.get()))
			{
				if (request->has_found_anything())
				{
					aimAt = request->get_path_to_best();
				}
			}
			perceptionRequest = nullptr;
			cooldowns.set(NAME(findCloseOne), 5.0f);
		}

		dischargedAndHurt = false;
		if (! overDischarged)
		{
			PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai charger] discharge"));
			//debug_context(imo->get_presence()->get_in_room());
			Framework::CheckSegmentResult result;
			Framework::RelativeToPresencePlacement relativePlacement;
			relativePlacement.set_owner(imo);
			Transform perceptionPlacement = imo->get_presence()->get_placement().to_world(imo->get_appearance()->calculate_socket_os(npcBase->get_perception_socket_index()));
			Transform strikePlacement = offset_transform_by_forward_angle(perceptionPlacement, REF_ rg, Range(0.0f, 45.0f));
			Vector3 startLoc = strikePlacement.get_translation();
			Vector3 dir = strikePlacement.get_axis(Axis::Forward);
			Transform targetPlacement = strikePlacement;
			float maxDist = 2.5f;
			Framework::IModulesOwner * attackTarget = nullptr;
			Framework::Room * attackInRoom = nullptr;
			ai_log(mind->get_logic(), TXT("keepAttackingFor %.3f"), self->keepAttackingFor);
			ai_log(mind->get_logic(), TXT("z %.3f"), self->possibleTarget.is_active()? (self->possibleTarget.get_target_centre_relative_to_owner() * Vector3::xy).normal().y : 0.0f);
			if (self->keepAttackingFor && rg.get_chance(0.9f) && self->possibleTarget.is_active() && (self->possibleTarget.get_target_centre_relative_to_owner() * Vector3::xy).normal().y > 0.4f)
			{
				ai_log(mind->get_logic(), TXT("at target"));
				attackTarget = self->possibleTarget.get_target();
				attackInRoom = self->possibleTarget.get_in_final_room();
				targetPlacement = self->possibleTarget.get_placement_in_owner_room();
			}
			else if (aimAt.is_active() && rg.get_chance(0.9f) && (aimAt.get_target_centre_relative_to_owner() * Vector3::xy).normal().y > 0.4f)
			{
				ai_log(mind->get_logic(), TXT("at target from perception"));
				attackTarget = aimAt.get_target();
				attackInRoom = aimAt.get_in_final_room();
				targetPlacement = aimAt.get_placement_in_owner_room();
			}

			if (attackTarget)
			{
				dir = (targetPlacement.get_translation() + rg.get_float(0.0f, 0.3f) * Vector3(rg.get_float(-1.0f, 1.0f), rg.get_float(-2.0f, 2.0f), rg.get_float(-1.0f, 1.0f)).normal() - startLoc).normal();
			}
			else
			{
				ai_log(mind->get_logic(), TXT("anywhere"));
			}
			targetPlacement.set_translation(startLoc + (dir * maxDist));
			/*
			if (self->possibleTarget.is_active())
			{
				debug_draw_time_based(1.0f, debug_draw_arrow(true, Colour::green, startLoc, self->possibleTarget.get_placement_in_owner_room().get_translation()));
			}
			debug_draw_time_based(1.0f, debug_draw_arrow(true, Colour::red, startLoc, targetPlacement.get_translation()));
			*/
			bool hitSomething = false;
			{
				PERFORMANCE_GUARD_LIMIT(0.001f, TXT("[ai charger] ray cast"));
				hitSomething = do_ray_cast(CastInfo().at_target().with_collision_flags(Framework::GameConfig::projectile_collides_with_flags()),
					startLoc, imo,
					attackTarget,
					attackInRoom,
					targetPlacement, result, &relativePlacement);
			}
			if (hitSomething && relativePlacement.get_straight_length() < maxDist)
			{
				PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai charger] discharge HIT"));
				auto & usePlacement = relativePlacement;
				auto* enemy = fast_cast<::Framework::IModulesOwner>(result.object);
				LATENT_LOG(TXT("discharge"));
				lightningSpawner->single(NAME(discharge), Framework::CustomModules::LightningSpawner::LightningParams().with_target(usePlacement, hitSomething));
				bool didHitSomethingWithHealth = false;
				if (enemy)
				{
					ai_log(self, TXT("hit %S"), enemy->ai_get_name().to_char());
					auto* check = enemy;
					while (check)
					{
						if (auto* health = check->get_custom<CustomModules::Health>())
						{
							didHitSomethingWithHealth = true;
							if (GameSettings::get().difficulty.npcs > GameSettings::NPCS::NonAggressive)
							{
								Damage dealDamage;
								dealDamage.damage = imo->get_variables().get_value(NAME(dischargeDamage), Energy(magic_number 2.0f));
								dealDamage.damageType = DamageType::Lightning;

								auto* ep = check->get_presence();
								auto* presence = imo->get_presence();

								DamageInfo damageInfo;
								damageInfo.damager = imo;
								damageInfo.source = imo;
								damageInfo.instigator = imo;

								ContinuousDamage dealContinuousDamage;
								dealContinuousDamage.setup_using_weapon_core_modifier_companion_for(dealDamage);

								health->adjust_damage_on_hit_with_extra_effects(REF_ dealDamage);
								if (PhysicalMaterial::ForProjectile const * useForProjectile = PhysicalMaterial::get_for_projectile(result.material, dealDamage, lightningProjectileTypeTags))
								{
									dealDamage.apply(useForProjectile, nullptr);
								}
								ai_log(self, TXT("do damage %.0f to %S"), dealDamage.damage.as_float(), check->ai_get_name().to_char());
								{
									if (ep->get_in_room() == presence->get_in_room())
									{
										damageInfo.hitRelativeLocation = ep->get_placement().location_to_local(presence->get_centre_of_presence_WS()).normal();
									}
									else
									{
										Framework::RelativeToPresencePlacement rpp;
										rpp.be_temporary_snapshot();
										if (rpp.find_path(cast_to_nonconst(check), presence->get_in_room(), presence->get_centre_of_presence_transform_WS()))
										{
											damageInfo.hitRelativeLocation = ep->get_placement().location_to_local(rpp.get_placement_in_owner_room().get_translation()).normal();
										}
									}
								}
								health->deal_damage(dealDamage, dealDamage, damageInfo);
								if (dealContinuousDamage.is_valid())
								{
									health->add_continuous_damage(dealContinuousDamage, damageInfo);
								}
							}
							break;
						}
						check = check->get_instigator(true);
					}
					self->hitEnemy = true;
					dischargedAndHurt = true;
					dischargePower += magic_number 0.5f;
					relativePlacement.find_path(imo, cast_to_nonconst(enemy));
					if (GameSettings::get().difficulty.npcs > GameSettings::NPCS::NonAggressive)
					{
						if (self->keepAttackingFor)
						{
							self->play_sound(NAME(alert));
						}
						else
						{
							self->play_sound(NAME(attack));
						}
						self->keep_attacking_for(rg.get_float(10.0f, 15.0f), relativePlacement, 1.0f, 1);
					}
				}
				// hit
				if (didHitSomethingWithHealth)
				{
					if (auto* tos = imo->get_temporary_objects())
					{
						if (result.hitNormal.is_normalised())
						{
							auto* lightningHit = tos->spawn(NAME(lightningHit));
							if (lightningHit)
							{
								Transform placement = matrix_from_up(result.hitLocation, result.hitNormal).to_transform();
								if (auto * at = result.presenceLink ? result.presenceLink->get_modules_owner() : nullptr)
								{
									Transform placementRelativeTo = result.presenceLink->get_placement_in_room();
									Name bone = result.shape ? result.shape->get_collidable_shape_bone() : Name::invalid();
									int boneIdx = NONE;
									auto* skeleton = at->get_appearance()->get_skeleton();
									if (bone.is_valid() && skeleton)
									{
										boneIdx = skeleton->get_skeleton()->find_bone_index(bone);
									}
									auto* poseMS = at->get_appearance()->get_final_pose_MS();
									if (boneIdx != NONE && poseMS)
									{
										Transform boneMS = poseMS->get_bone(boneIdx);
										Transform boneOS = at->get_appearance()->get_ms_to_os_transform().to_world(boneMS);
										Transform boneWS = placementRelativeTo.to_world(boneOS);
										placementRelativeTo = boneWS;
										Transform relativePlacement = placementRelativeTo.to_local(placement);
										lightningHit->on_activate_attach_to_bone(at, bone, false, relativePlacement);
									}
									else
									{
										Transform relativePlacement = placementRelativeTo.to_local(placement);
										lightningHit->on_activate_attach_to(at, false, relativePlacement);
									}
								}
								else
								{
									lightningHit->on_activate_place_in(cast_to_nonconst(result.inRoom), placement);
								}
							}
						}
					}
				}
				//
			}
			else
			{
				LATENT_LOG(TXT("miss"));
				relativePlacement.find_path(imo, imo->get_presence()->get_in_room(), targetPlacement);
				lightningSpawner->single(NAME(dischargeMiss), Framework::CustomModules::LightningSpawner::LightningParams().with_target(relativePlacement, hitSomething));
			}
			//debug_no_context();
		}
		dischargeMoreOften = (dischargedAndHurt ? 1.0f : 0.0f) * 0.7f + 0.3f * dischargeMoreOften;

		LATENT_WAIT(rg.get_float(0.05f + (1.0f- dischargeMoreOften) * 0.4f, 0.15f + (1.0f - dischargeMoreOften) * 1.0f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	//

	LATENT_RETURN();
}

LATENT_FUNCTION(Charger::to_target)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai charger] to target"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("to target"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);

	auto* imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Charger>(mind->get_logic());

	LATENT_BEGIN_CODE();

	if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
	{
		em->emissive_activate(NAME(attack));
	}

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("to target"));
	ai_log_no_colour(mind->get_logic());

	FIND_PATH:
	self->newKeepAttackingFor = false;
	{
		LATENT_LOG(TXT("find path"));
		Framework::Nav::PathRequestInfo pathRequestInfo(imo);
		pathRequestInfo.with_dev_info(TXT("charger to target"));
		Vector3 inDir = (self->possibleTarget.get_placement_in_owner_room().get_translation() - imo->get_presence()->get_placement().get_translation()).normal_2d();
		//debug_context(imo->get_presence()->get_in_room());
		//debug_draw_time_based(4.0f, debug_draw_arrow(true, Colour::green, imo->get_presence()->get_centre_of_presence_WS(), imo->get_presence()->get_centre_of_presence_WS() + inDir));
		//debug_no_context();
		pathTask = mind->access_navigation().get_random_path(Random::get_float(2.0f, 10.0f), inDir, pathRequestInfo);
	}

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
	}
	else
	{
		LATENT_LOG(TXT("no path task"));
		path.clear();
	}

	{
		auto& locomotion = mind->access_locomotion();
		if (path.is_set())
		{
			LATENT_LOG(TXT("follow path"));
			locomotion.follow_path_2d(path.get(), NP, Framework::MovementParameters().full_speed());
			locomotion.turn_follow_path_2d();
		}
		else
		{
			LATENT_LOG(TXT("turn towards possible target"));
			locomotion.stop();
			locomotion.turn_towards_2d(self->possibleTarget, 10.0f);
		}
	}

	// just wait
	while (true)
	{
		if (self->newKeepAttackingFor)
		{
			Vector3 inDir = (self->possibleTarget.get_placement_in_owner_room().get_translation() - imo->get_presence()->get_placement().get_translation()).normal();
			if (Vector3::dot(inDir, imo->get_presence()->get_placement().get_axis(Axis::Forward)) < 0.9f)
			{
				LATENT_LOG(TXT("new keep attacking for and in different direction"));
				goto FIND_PATH;
			}
		}
		LATENT_WAIT(Random::get_float(0.1f, 0.2f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	// don't stop locomotion, just allow to get going
	if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
	{
		em->emissive_deactivate(NAME(attack));
	}

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Charger::attack_or_wander)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai charger] attack or wander"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("attack or wander"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, dischargesTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, grabEnergyTask);

	auto* imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Charger>(mind->get_logic());

	LATENT_BEGIN_CODE();

	if (auto * ls = imo->get_custom<::Framework::CustomModules::LightningSpawner>())
	{
		ls->start(NAME(idle));
	}

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(discharges));
		dischargesTask.start_latent_task(mind, taskInfo);
	}

	if (false)
	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::grab_energy));
		grabEnergyTask.start_latent_task(mind, taskInfo);
	}

	while (true)
	{
		if (currentTask.can_start_new())
		{
			::Framework::AI::LatentTaskInfoWithParams taskInfo;
			if (self->keepAttackingFor && GameSettings::get().difficulty.npcs > GameSettings::NPCS::NonAggressive)
			{
				taskInfo.propose(AI_LATENT_TASK_FUNCTION(to_target));
			}
			else
			{
				taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::wander));
			}
			if (currentTask.can_start(taskInfo))
			{
				currentTask.start_latent_task(mind, taskInfo);
			}
		}
		LATENT_YIELD();
	}
	
	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	if (auto * ls = imo->get_custom<::Framework::CustomModules::LightningSpawner>())
	{
		ls->stop(NAME(idle));
	}
	dischargesTask.stop();
	grabEnergyTask.stop();

	LATENT_END_CODE();
	//

	// update allow to interrupt state
	thisTaskHandle->allow_to_interrupt(currentTask.can_start_new());

	LATENT_RETURN();
}

LATENT_FUNCTION(Charger::death_sequence)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai charger] death sequence"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("death sequence"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::IModulesOwner*, imo);
	LATENT_VAR(SafePtr<Framework::IModulesOwner>, dying);
	LATENT_VAR(float, waitTime);
	LATENT_VAR(int, explosionIdx);
	
	auto* self = fast_cast<Charger>(mind->get_logic());

	LATENT_BEGIN_CODE();

	ai_log(mind->get_logic(), TXT("death sequence"));

	imo = mind->get_owner_as_modules_owner();

	if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
	{
		em->emissive_deactivate_all();
	}

	{
		auto & locomotion = mind->access_locomotion();
		locomotion.stop();
	}

	if (auto* tos = imo->get_temporary_objects())
	{
		dying = tos->spawn_attached(NAME(dying));
	}

	if (auto* ls = imo->get_custom<Framework::CustomModules::LightningSpawner>())
	{
		ls->start(NAME(dischargesDying));
	}

	waitTime = 4.0f;
	if (auto* h = imo->get_custom<CustomModules::Health>())
	{
		if (h->has_extra_effect(NAME(overcharged)))
		{
			waitTime = 0.2f;
		}
	}

	LATENT_WAIT(waitTime);

	explosionIdx = 0;
	while (explosionIdx < self->deathExplosionCount)
	{
		LATENT_WAIT(Random::get_float(0.1f, 0.3f));
		if (auto* tos = imo->get_temporary_objects())
		{
			Vector3 explosionLocOS = imo->get_presence()->get_random_point_within_presence_os(0.8f);
			Transform explosionWS = imo->get_presence()->get_placement().to_world(Transform(explosionLocOS, Quat::identity));
			Framework::Room* inRoom;
			imo->get_presence()->move_through_doors(explosionWS, OUT_ inRoom);
			tos->spawn_in_room(NAME(explode), inRoom, explosionWS);
		}
		++explosionIdx;
	}
	if (dying.is_set())
	{
		if (auto* to = fast_cast<Framework::TemporaryObject>(dying.get()))
		{
			to->desire_to_deactivate();
		}
		dying.clear();
	}

	if (auto* ls = imo->get_custom<Framework::CustomModules::LightningSpawner>())
	{
		ls->stop(NAME(dischargesDying));
	}

	ai_log(mind->get_logic(), TXT("perform death"));
	if (auto* h = imo->get_custom<CustomModules::Health>())
	{
		// it's okay to not setup death here as we're using ai message originating in health module's death
		h->perform_death();
	}

	LATENT_ON_BREAK();
	//
	if (mind)
	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.stop();
	}

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Charger::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai charger] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Name, scriptedBehaviour);
	LATENT_COMMON_VAR(float, aggressiveness);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, announcePresenceTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, perceptionTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, hatchTask);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);

	LATENT_VAR(bool, emissiveFriendly);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	LATENT_VAR(bool, dieNow);

	aggressiveness = 99.0f;

	LATENT_BEGIN_CODE();

	if (ShouldTask::announce_presence(mind->get_owner_as_modules_owner()))
	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::announce_presence));
		announcePresenceTask.start_latent_task(mind, taskInfo);
	}

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(perception_blind));
		perceptionTask.start_latent_task(mind, taskInfo);
	}

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(hatch));
		hatchTask.start_latent_task(mind, taskInfo);
	}

	emissiveFriendly = false;

	messageHandler.use_with(mind);
	{
		auto * framePtr = &_frame;
		messageHandler.set(NAME(die), [framePtr, mind, &dieNow](Framework::AI::Message const & _message)
		{
			dieNow = true;
			framePtr->end_waiting();
			AI_LATENT_STOP_LONG_RARE_ADVANCE();
		}
		);
	}

	//LATENT_WAIT(1.0f);
	//dieNow = true;

	while (true)
	{
		if (dieNow)
		{
			{
				::Framework::AI::LatentTaskInfoWithParams nextTask;
				nextTask.propose(AI_LATENT_TASK_FUNCTION(death_sequence), NP, NP, false, true);
				currentTask.start_latent_task(mind, nextTask);
			}
			while (true)
			{
				// just wait here until we die
				LATENT_YIELD();
			}
		}
		{
			::Framework::AI::LatentTaskInfoWithParams nextTask;

			if (Common::handle_scripted_behaviour(mind, scriptedBehaviour, currentTask))
			{
				// doing scripted behaviour
			}
			else
			{
				// choose best action for now
				nextTask.propose(AI_LATENT_TASK_FUNCTION(attack_or_wander));

				// check if we can start new one, if so, start it
				if (currentTask.can_start(nextTask))
				{
					currentTask.start_latent_task(mind, nextTask);
				}
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
	hatchTask.stop();
	currentTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}
