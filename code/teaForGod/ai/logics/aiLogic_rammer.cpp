#include "aiLogic_rammer.h"

#include "tasks\aiLogicTask_flyTo.h"
#include "tasks\aiLogicTask_idle.h"
#include "tasks\aiLogicTask_perception.h"
#include "tasks\aiLogicTask_projectileHit.h"

#include "..\aiCommon.h"
#include "..\aiCommonVariables.h"
#include "..\perceptionRequests\aipr_FindInRoom.h"

#include "..\..\game\damage.h"
#include "..\..\game\gameSettings.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_operativeDevice.h"
#include "..\..\modules\custom\health\mc_health.h"
#include "..\..\modules\shield.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiSocial.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\moduleController.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\object\scenery.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\world.h"

#include "..\..\..\core\debug\debugRenderer.h"


using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai message name
DEFINE_STATIC_NAME(projectileHit);
DEFINE_STATIC_NAME(die);
DEFINE_STATIC_NAME(death);
DEFINE_STATIC_NAME(entersRoom);
DEFINE_STATIC_NAME(dealtDamage);
DEFINE_STATIC_NAME(rotateOnDamage);
DEFINE_STATIC_NAME(ram_IWantToRam);
DEFINE_STATIC_NAME(ram_IAmAlreadyRamming);

// ai message params
DEFINE_STATIC_NAME(instigator);
DEFINE_STATIC_NAME(hit);
DEFINE_STATIC_NAME(inRoom);
DEFINE_STATIC_NAME(inRoomLocation);
DEFINE_STATIC_NAME(inRoomNormal);
DEFINE_STATIC_NAME(who);
DEFINE_STATIC_NAME(damage);
DEFINE_STATIC_NAME(damageUnadjusted);

// sound
DEFINE_STATIC_NAME(engine);
DEFINE_STATIC_NAME(chatter);
DEFINE_STATIC_NAME_STR(ramTelegraph, TXT("ram telegraph"));
DEFINE_STATIC_NAME_STR(ramHit, TXT("ram hit"));

// temporary objects
DEFINE_STATIC_NAME_STR(chargeToRam, TXT("charge to ram"));

// ai variables
DEFINE_STATIC_NAME(beaconRoom);
DEFINE_STATIC_NAME(beaconPlacement);
DEFINE_STATIC_NAME(beaconRadius);

// gaits
DEFINE_STATIC_NAME(fast);
DEFINE_STATIC_NAME(normal);
DEFINE_STATIC_NAME(ram);
DEFINE_STATIC_NAME(keepClose);

// emissive
DEFINE_STATIC_NAME(berserker);

//

static float aggressiveness_coef(float _perOne = 0.3f)
{
	return max(1.0f, 1.0f + _perOne * (GameSettings::get().ai_agrressiveness_as_float() - 1.0f));
}

//

REGISTER_FOR_FAST_CAST(Rammer);

Rammer::Rammer(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	rammerData = fast_cast<RammerData>(_logicData);
}

Rammer::~Rammer()
{
}

void Rammer::advance(float _deltaTime)
{
	set_auto_rare_advance_if_not_visible(Range(0.1f, 0.5f));

	base::advance(_deltaTime);
}

void Rammer::learn_from(SimpleVariableStorage & _parameters)
{
	if (!should_stay_in_room())
	{
		warn(TXT("rammers may now only stay in room"));
		stay_in_room(true);
	}
	base::learn_from(_parameters);
}

static LATENT_FUNCTION(update_beacon)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("update beacon"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::Logic*, logic);
	LATENT_VAR(::Framework::IModulesOwner*, imo);
	LATENT_VAR(SimpleVariableInfo, beaconRoomVar);
	LATENT_VAR(SimpleVariableInfo, beaconPlacementVar);
	LATENT_VAR(SimpleVariableInfo, beaconRadiusVar);

	LATENT_BEGIN_CODE();

	logic = mind->get_logic();
	imo = mind->get_owner_as_modules_owner();

	beaconRoomVar = imo->access_variables().find<SafePtr<::Framework::Room>>(NAME(beaconRoom));
	beaconPlacementVar = imo->access_variables().find<Transform>(NAME(beaconPlacement));
	beaconRadiusVar = imo->access_variables().find<float>(NAME(beaconRadius));

	while (true)
	{
		if (auto* presence = imo->get_presence())
		{
			if (auto * room = presence->get_in_room())
			{
				if (room != beaconRoomVar.get<SafePtr<::Framework::Room>>().get())
				{
					float doorCount = 0.0f;
					Vector3 loc = Vector3::zero;
					Vector3 up = presence->get_environment_up_dir();

					for_every_ptr(door, room->get_doors())
					{
						// include invisible door
						loc += door->get_placement().get_translation();
						doorCount += 1.0f;
					}

					if (doorCount == 0.0f)
					{
						loc = presence->get_placement().get_translation();
					}
					else
					{
						loc = loc / doorCount;
					}

					Transform beaconPlacement = Transform(loc, up_matrix33(up.normal()).to_quat());
					beaconPlacementVar.access<Transform>() = beaconPlacement;
					beaconRoomVar.access<SafePtr<::Framework::Room>>() = room;

					float beaconRadius = 6.0f;
					if (doorCount > 0.0f)
					{
						float externalDoorCount = 0.0f;
						beaconRadius = 0.0f;
						for_every_ptr(door, room->get_doors())
						{
							// include invisible door
							Vector3 doorLoc = beaconPlacement.location_to_local(door->get_placement().get_translation()) * Vector3::xy;
							Vector3 doorInboundDir = beaconPlacement.vector_to_local(door->get_inbound_matrix().get_y_axis()) * Vector3::xy;
							if (Vector3::dot(doorLoc.normal(), doorInboundDir.normal()) > 0.3f)
							{
								externalDoorCount += 1.0f;
							}
							beaconRadius += doorLoc.length_2d();
						}
						beaconRadius /= doorCount;
						if (externalDoorCount > doorCount * 0.5f)
						{
							beaconRadius = max(beaconRadius * 3.1f, beaconRadius + Random::get_float(4.0f, 6.5f));
						}
						else
						{
							beaconRadius = max(beaconRadius * 0.7f, beaconRadius - Random::get_float(1.0f, 2.5f));
						}
						if (beaconRadius == 0.0f)
						{
							beaconRadius = 6.0f;
						}
					}
					beaconRadiusVar.access<float>() = beaconRadius;
				}
			}
		}
		LATENT_WAIT(Random::get_float(0.3f, 0.6f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

static LATENT_FUNCTION(death_sequence)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("death sequence"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::IModulesOwner*, imo);

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

	if (auto* c = imo->get_controller())
	{
		Vector3 gravity = imo->get_presence()->get_gravity() * Random::get_float(0.0f, 0.1f);
		
		c->set_requested_velocity_linear(gravity);

		float yaw = imo->get_presence()->get_velocity_rotation().yaw;
		if (abs(yaw) > 50.0f)
		{
			yaw = sign(yaw);
		}
		else
		{
			yaw = Random::get_bool()? 1.0f : -1.0f;
		}

		c->set_requested_velocity_orientation(Rotator3(0.0f, yaw * Random::get_float(360.0f, 480.0f), 0.0f));
	}

	LATENT_WAIT(0.4f);

	ai_log(mind->get_logic(), TXT("perform death"));
	if (auto* h = imo->get_custom<CustomModules::Health>())
	{
		// we got here through "die" ai message from health
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

static LATENT_FUNCTION(fly_around_beacon)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("fly around beacon"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(bool, berserkerMode);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::Logic*, logic);
	LATENT_VAR(::Framework::IModulesOwner*, imo);
	LATENT_VAR(SimpleVariableInfo, beaconRoomVar);
	LATENT_VAR(SimpleVariableInfo, beaconPlacementVar);
	LATENT_VAR(SimpleVariableInfo, beaconRadiusVar);
	
	LATENT_VAR(bool, goRight);
	LATENT_VAR(Vector3, currentLocation);
	LATENT_VAR(float, updateTime);
	LATENT_VAR(bool, keepFlying);
	LATENT_VAR(float, relSpeed);
	LATENT_VAR(float, currentRadius);
	LATENT_VAR(float, currentAlt);

	LATENT_BEGIN_CODE();

	logic = mind->get_logic();
	imo = mind->get_owner_as_modules_owner();

	ai_log(logic, TXT("fly around beacon"));

	beaconRoomVar = imo->access_variables().find<SafePtr<::Framework::Room>>(NAME(beaconRoom));
	beaconPlacementVar = imo->access_variables().find<Transform>(NAME(beaconPlacement));
	beaconRadiusVar = imo->access_variables().find<float>(NAME(beaconRadius));
	
	goRight = true;

	currentLocation = Vector3::zero;
	updateTime = 0.5f;

	relSpeed = Random::get_float(0.5f, 1.2f);

	currentRadius = 0.0f;
	currentAlt = 0.0f;

	keepFlying = true;
	while (keepFlying)
	{
		updateTime = 0.2f;
		if (Random::get_chance(0.05f))
		{
			relSpeed = Random::get_float(0.5f, 1.2f);
		}

		{
			auto * presence = mind->get_owner_as_modules_owner()->get_presence();
			currentLocation = presence->get_centre_of_presence_WS();
			updateTime = 0.25f;
			::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
			if (mind && beaconRoomVar.get<SafePtr<::Framework::Room>>() == presence->get_in_room())
			{
				float beaconRadius = beaconRadiusVar.get<float>();
				if (currentRadius == 0.0f)
				{
					currentRadius = beaconRadius * Random::get_float(0.8f, 1.1f);
				}
				else
				{
					currentRadius = clamp(currentRadius * Random::get_float(0.995f, 1.005f), beaconRadius * 0.7f, beaconRadius * 1.2f);
				}
					
				Transform beaconPlacement = beaconPlacementVar.get<Transform>();
				Vector3 beaconLocation = beaconPlacement.location_to_world(Vector3::zAxis * 2.0f);
#ifdef AN_DEVELOPMENT
#ifndef AN_CLANG
				float radius = (currentLocation - beaconLocation).length();
#endif
#endif
				Vector3 towardsTarget = beaconLocation - currentLocation;
				Vector3 upDir = beaconPlacement.get_axis(Axis::Z);
				Vector3 towardsTargetFlat = towardsTarget.drop_using(upDir);
				if (towardsTargetFlat.is_almost_zero())
				{
					towardsTargetFlat = beaconPlacement.get_axis(Axis::X);
				}

				float altNow = Vector3::dot(currentLocation - beaconLocation, upDir);
				if (currentAlt == 0.0f)
				{
					currentAlt = altNow + Random::get_float(-1.0f, 1.0f);
				}
				else
				{
					currentAlt = clamp(currentAlt + Random::get_float(-0.1f, 0.1f), -1.5f, 1.5f);
				}

				Vector3 rightDir = Vector3::cross(towardsTargetFlat, upDir);
				Vector3 navPoint = currentLocation + rightDir * 2.0f * (goRight ? 1.0f : -1.0f);
				navPoint = beaconLocation + (navPoint - beaconLocation).drop_using(upDir).normal() * currentRadius;
				navPoint += upDir * (altNow + (currentAlt - altNow) * 6.0f);
				debug_filter(ai_draw);
				debug_draw_time_based(updateTime, debug_draw_arrow(true, Colour::green, currentLocation, beaconLocation));
				debug_draw_time_based(updateTime, debug_draw_arrow(true, Colour::red, currentLocation, navPoint));
				debug_draw_time_based(updateTime, debug_draw_sphere(true, true, Colour::red, 0.3f, Sphere(currentLocation, 0.05f)));
				debug_no_filter();

				locomotion.move_to_3d(navPoint, 0.4f, Framework::MovementParameters().relative_speed(relSpeed * (berserkerMode? 6.0f : 1.0f)));
				locomotion.turn_in_movement_direction_2d();
				locomotion.dont_keep_distance();
			}
			else
			{
				locomotion.stop(); // wait
				keepFlying = false;
			}
		}
		LATENT_WAIT(updateTime);
		// keep on circling forever!
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

Vector3 get_ram_through_location(::Framework::RelativeToPresencePlacement const & _ramTarget, Vector3 const & _centreOffsetOS, Vector3 const & _targettingOffsetOS, ::Framework::IModulesOwner* _imo, float _atActualTarget = 0.2f)
{
	return _ramTarget.get_placement_in_owner_room().location_to_world(_centreOffsetOS * (1.0f - _atActualTarget) + _atActualTarget * _targettingOffsetOS);
}

bool will_hit_target(Framework::IModulesOwner* _enemy, ::Framework::IModulesOwner* _imo, Vector3 const & _targetWS)
{
	if (!_enemy)
	{
		return false;
	}
	Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::WS);
	collisionTrace.add_location(_imo->get_presence()->get_centre_of_presence_WS());
	collisionTrace.add_location(_targetWS);
	Framework::CheckSegmentResult result;
	int collisionTraceFlags = Framework::CollisionTraceFlag::ResultInPresenceRoom;
	Framework::CheckCollisionContext checkCollisionContext;
	checkCollisionContext.collision_info_needed();
	if (auto* collision = _imo->get_collision())
	{
		checkCollisionContext.use_collision_flags(collision->get_collides_with_flags());
	}
	checkCollisionContext.avoid(_imo, true);
	if (_imo->get_presence()->trace_collision(AgainstCollision::Movement, collisionTrace, result, collisionTraceFlags, checkCollisionContext))
	{
		if (auto* hitObject = fast_cast<Framework::IModulesOwner>(result.object))
		{
			if (hitObject->get_top_instigator() == _enemy->get_top_instigator())
			{
				return true;
			}
		}
	}
	return false;
}

LATENT_FUNCTION(ram_enemy)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("ram enemy"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(float, aggressiveness);
	LATENT_COMMON_VAR(bool, berserkerMode);
	LATENT_COMMON_VAR(PerceptionLock, perceptionEnemyLock);
	LATENT_COMMON_VAR_END();

	LATENT_OBJECT_VAR_BEGIN();
	LATENT_OBJECT_VAR(bool, allowAttack);
	LATENT_OBJECT_VAR_END();

	LATENT_VAR(::Framework::RelativeToPresencePlacement, ramTarget);
	LATENT_VAR(Vector3, ramCentreOffsetOS);
	LATENT_VAR(Vector3, ramTargettingOffsetOS);
	LATENT_VAR(Vector3, ramLocation);
	LATENT_VAR(Vector3, ramThrough);
	LATENT_VAR(Vector3, ramStart);
	LATENT_VAR(float, timeLeft);
	LATENT_VAR(::Framework::SoundSourcePtr, ramSound);
	LATENT_VAR(SafePtr<Framework::IModulesOwner>, chargeToRam);
	LATENT_VAR(int, alreadyRammingCount);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	LATENT_VAR(bool, allowOthersToRam);

	timeLeft -= LATENT_DELTA_TIME;

#ifdef AN_USE_AI_LOG
	Rammer* self = fast_cast<Rammer>(mind->get_logic());
#endif
	::Framework::IModulesOwner* imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	perceptionEnemyLock.override_lock(NAME(ram), AI::PerceptionLock::Lock);

	LATENT_SECTION_INFO(TXT("begin"));

	allowOthersToRam = false;

	ai_log(mind->get_logic(), TXT("ram enemy"));

	if (mind && enemyPlacement.is_active())
	{
		// ok
	}
	else
	{
		ai_log(self, TXT("missing target (1)"));
		LATENT_BREAK();
	}

	LATENT_SECTION_INFO(TXT("turn towards enemy"));
	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.stop();
		if (enemyPlacement.get_target())
		{
			locomotion.turn_towards_2d(enemyPlacement.get_target(), 1.0f);
		}
		else
		{
			locomotion.turn_towards_2d(AI::Targetting::get_enemy_centre_of_presence_in_owner_room(imo), 1.0f);
		}
	}

	if (mind && enemyPlacement.is_active())
	{
		ramThrough = get_ram_through_location(enemyPlacement, AI::Targetting::get_enemy_centre_of_presence_offset_os(imo), AI::Targetting::get_enemy_targetting_offset_os(imo), imo);
	}
	else
	{
		ai_log(self, TXT("missing target (2)"));
		LATENT_BREAK();
	}

	if (!will_hit_target(enemy.get(), imo, ramThrough))
	{
		ai_log(self, TXT("wouldn't hit target"));
		LATENT_BREAK();
	}

	if (!allowAttack)
	{
		ai_log(self, TXT("doesn't want to attack any more"));
		aggressiveness = 0.0f;
		LATENT_BREAK();
	}

	LATENT_SECTION_INFO(TXT("check if no one else wants to ram"));
	messageHandler.use_with(mind);
	if (!berserkerMode)
	{
		// upon start will send ai message that wants to ram
		// if no one replies, will continue with actual ramming
		// if anyone replies that is ramming, will stop
		auto * framePtr = &_frame;
		messageHandler.set(NAME(ram_IWantToRam), [mind, &allowOthersToRam](Framework::AI::Message const & _message)
		{
			if (allowOthersToRam)
			{
				return;
			}
			// this responds if any one else wants to ram
			if (auto* who = _message.get_param(NAME(who)))
			{
				auto* whoImo = who->get_as<SafePtr<Framework::IModulesOwner>>().get();
				if (whoImo != mind->get_owner_as_modules_owner())
				{
					if (auto* message = _message.create_response(NAME(ram_IAmAlreadyRamming)))
					{
						message->to_ai_object(whoImo);
						message->access_param(NAME(who)).access_as<SafePtr<Framework::IModulesOwner>>() = mind->get_owner_as_modules_owner();
					}
				}
			}
		}
		);
		messageHandler.set(NAME(ram_IAmAlreadyRamming), [
#ifdef AN_USE_AI_LOG
			mind,
#endif
			framePtr, &alreadyRammingCount](Framework::AI::Message const & _message)
		{
			++alreadyRammingCount;
			int limitOfConcurrentRammers = GameSettings::get().ai_agrressiveness_as_float() < 3.5f? (int)(GameSettings::get().ai_agrressiveness_as_float() + 0.5f) : 10;
			if (alreadyRammingCount >= limitOfConcurrentRammers)
			{
				framePtr->break_now(); // mark to break
				ai_log(mind->get_logic(), TXT("someone else wants to ram"));
			}
		}
		);
	}

	// send message to check if anyone else is ramming, if so, we will stop ramming now
	alreadyRammingCount = 0;
	if (!berserkerMode)
	{
		if (auto* message = imo->get_presence()->get_in_world()->create_ai_message(NAME(ram_IWantToRam)))
		{
			ai_log(mind->get_logic(), TXT("inform about our plans"));
			message->to_room(imo->get_presence()->get_in_room());
			message->access_param(NAME(who)).access_as<SafePtr<Framework::IModulesOwner>>() = mind->get_owner_as_modules_owner();
		}
	}

	LATENT_SECTION_INFO(TXT("wait for ramming response"));
	LATENT_WAIT(0.5f);

	LATENT_SECTION_INFO(TXT("wait for turn to end"));
	timeLeft = 2.0f;
	while (timeLeft > 0.0f)
	{
		if (mind->access_locomotion().has_finished_turn())
		{
			break;
		}
		LATENT_WAIT(0.1f);
	}

	LATENT_SECTION_INFO(TXT("check if target is still valid"));
	if (mind && enemyPlacement.is_active())
	{
		ramThrough = get_ram_through_location(enemyPlacement, AI::Targetting::get_enemy_centre_of_presence_offset_os(imo), AI::Targetting::get_enemy_targetting_offset_os(imo), imo);
		auto * presence = imo->get_presence();
		Vector3 ramLocationLocal = presence->get_placement().location_to_local(ramThrough);
		Rotator3 ramRotationLocal = ramLocationLocal.to_rotator();
		ramRotationLocal = ramRotationLocal.normal_axes();
		if ((abs(ramRotationLocal.pitch) > 15.0f && (ramLocationLocal * Vector3::xy).length() > 3.0f) ||
			abs(ramRotationLocal.pitch) > 30.0f ||
			abs(ramRotationLocal.yaw) > 30.0f)
		{
			ai_log(mind->get_logic(), TXT("not in front %S dist %.3f"), ramRotationLocal.to_string().to_char(), (ramLocationLocal * Vector3::xy).length());
			LATENT_BREAK();
		}
	}
	else
	{
		ai_log(self, TXT("missing target (3)"));
		LATENT_BREAK();
	}

	if (!will_hit_target(enemy.get(), imo, ramThrough))
	{
		ai_log(self, TXT("wouldn't hit target"));
		LATENT_BREAK();
	}

	if (!allowAttack)
	{
		ai_log(self, TXT("doesn't want to attack any more"));
		aggressiveness = 0.0f;
		LATENT_BREAK();
	}

	LATENT_SECTION_INFO(TXT("decision and telegraph!"));
	ai_log(mind->get_logic(), TXT("telegraph"));

	// decided!
	ramTarget = enemyPlacement;
	ramCentreOffsetOS = AI::Targetting::get_enemy_centre_of_presence_offset_os(imo);
	ramTargettingOffsetOS = AI::Targetting::get_enemy_targetting_offset_os(imo);

	// telegraph
	if (auto* sound = imo->get_sound())
	{
		ramSound = sound->play_sound(NAME(ramTelegraph));
	}
	if (auto* tos = imo->get_temporary_objects())
	{
		chargeToRam = tos->spawn_attached(NAME(chargeToRam));
	}

	if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
	{
		em->emissive_activate(NAME(ram));
	}

	if (berserkerMode)
	{
		LATENT_WAIT(1.5f);
	}
	else
	{
		LATENT_WAIT(2.0f);
	}

	timeLeft = 5.0f;

	LATENT_SECTION_INFO(TXT("ready to ram"));
	// update ram through last time
	if (mind && ramTarget.is_active())
	{
		debug_filter(ai_draw);
		debug_draw_time_based(timeLeft, debug_draw_arrow(true, Colour::green, ramTarget.get_placement_in_owner_room().get_translation(), ramTarget.get_placement_in_owner_room().location_to_world(ramCentreOffsetOS)));
		debug_draw_time_based(timeLeft, debug_draw_arrow(true, Colour::yellow, ramTarget.get_placement_in_owner_room().get_translation(), ramTarget.get_placement_in_owner_room().location_to_world(ramTargettingOffsetOS)));
		debug_no_filter();
		ramThrough = get_ram_through_location(ramTarget, ramCentreOffsetOS, ramTargettingOffsetOS, imo);
	}
	// if no enemy (although we're working on a copy!), still ram to last known location, we telegraphed that!

	{	// get ram location on other side of ram through
		auto * presence = imo->get_presence();
		ramStart = presence->get_centre_of_presence_WS();
		{
			float distance = (ramThrough - ramStart).length();
			ramLocation = ramThrough + (ramThrough - ramStart).normal() * min(distance * 1.4f, distance + 2.0f);
		}
		debug_filter(ai_draw);
		debug_draw_time_based(timeLeft, debug_draw_arrow(true, Colour::red, ramStart, ramLocation));
		debug_no_filter();

		Vector3 ramLocationLocal = presence->get_placement().location_to_local(ramLocation);
		Rotator3 ramRotationLocal = ramLocationLocal.to_rotator();
		ramRotationLocal = ramRotationLocal.normal_axes();
		if (abs(ramRotationLocal.pitch) > 30.0f || abs(ramRotationLocal.yaw) > 30.0f)
		{
			ai_log(mind->get_logic(), TXT("not in front %S"), ramRotationLocal.to_string().to_char());
			LATENT_BREAK();
		}
	}

	if (!allowAttack)
	{
		ai_log(self, TXT("doesn't want to attack any more"));
		aggressiveness = 0.0f;
		LATENT_BREAK();
	}

	LATENT_SECTION_INFO(TXT("ram"));
	imo->activate_movement(NAME(ram));
	{	// ram!
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.move_to_3d(ramLocation, 0.1f, Framework::MovementParameters().gait(NAME(ram)).full_speed()); // move through door!
		locomotion.turn_in_movement_direction_2d();
		locomotion.dont_keep_distance();
	}

	while (timeLeft > 0.0f)
	{
		if (auto * presence = imo->get_presence())
		{
			Vector3 currentLocation = presence->get_centre_of_presence_WS();
			if ((currentLocation - ramStart).length() >
				(ramThrough - ramStart).length() + 1.0f)
			{
				// we didn't hit, go back
				break;
			}
		}

		if (!imo->get_collision()->get_collided_with().is_empty())
		{
			Vector3 up = imo->get_presence()->get_environment_up_dir();
			float dot = Vector3::dot((imo->get_collision()->get_gradient().drop_using(up)).normal(), (imo->get_presence()->get_velocity_linear().drop_using(up)).normal());
			ai_log(self, TXT("check collision, dot hit %.3f"), dot);
			if (dot < -0.82f)// we hit in front and stopped
			{
				// collided with anything
				timeLeft = 0.0f;
				bool actualHit = false;
				if (auto * presence = imo->get_presence())
				{
					if (presence->get_velocity_linear().length() > 0.1f)
					{
						Array<Framework::IModulesOwner *> damageTo;
						Array<Framework::IModulesOwner *> skipDamageTo;
						for_every(collidedWith, imo->get_collision()->get_collided_with())
						{
							if (auto* obj = fast_cast<Framework::Object>(collidedWith->collidedWithObject.get()))
							{
								if (fast_cast<Framework::Scenery>(obj) == nullptr) // skip scenery
								{
									auto * tI = obj->get_top_instigator();
									if (mind->access_social().is_enemy(tI))
									{
										bool skipDamage = obj->get_gameplay_as<IShield>() != nullptr; 
										bool first = true;
										Framework::IModulesOwner* instigator = cast_to_nonconst(obj);
										while (instigator)
										{
											if (instigator->get_custom<CustomModules::Health>())
											{
												if (skipDamage && ! first)
												{
													skipDamageTo.push_back_unique(instigator);
												}
												else
												{
													damageTo.push_back_unique(instigator);
												}
											}
											instigator = instigator->get_instigator(true);
											first = false;
										}
									}
								}
								else
								{
									actualHit = true;
								}
							}
						}
						for_every_ptr(damageImo, damageTo)
						{
							if (!skipDamageTo.does_contain(damageImo))
							{
								if (auto *op = damageImo->get_presence())
								{
									Transform op2presence = Transform::identity;
									bool ok = true;
									if (op->get_in_room() != presence->get_in_room())
									{
										Framework::RelativeToPresencePlacement rpp;
										rpp.be_temporary_snapshot();
										if (rpp.find_path(damageImo, presence->get_in_room(), presence->get_centre_of_presence_transform_WS()))
										{
											op2presence = rpp.calculate_final_room_transform();
										}
										else
										{
											ok = false;
										}
									}
									if (ok)
									{
										Vector3 upOpWS = op2presence.vector_to_local(up);
										Vector3 presenceVelocityOpWS = op2presence.vector_to_local(presence->get_velocity_linear()); // in op's world space
										Vector3 toOpDiffXYNormal = ((op->get_centre_of_presence_WS() - op2presence.location_to_local(presence->get_centre_of_presence_WS())).drop_using(upOpWS)).normal();
										float dotRelVel = Vector3::dot((presenceVelocityOpWS - op->get_velocity_linear()).drop_using(upOpWS), toOpDiffXYNormal);
										float dotGradient = Vector3::dot((op2presence.vector_to_local(imo->get_collision()->get_gradient()).drop_using(upOpWS)).normal(), toOpDiffXYNormal);
										ai_log(self, TXT("ram hit at speed %.3f, dotRelVel %.3f dotGradient %.3f"), presence->get_velocity_linear().length(), dotRelVel, dotGradient);
										if (dotRelVel > 0.0f && dotGradient < -0.82f)
										{
											ai_log(self, TXT("hit!"));
											actualHit = true;
											if (auto* health = damageImo->get_custom<CustomModules::Health>())
											{
												Damage dealDamage;
												float const ramVelocity = magic_number 4.0f;
												float const thresholdUp = 0.4f;
												float const thresholdDown = 0.2f;
												dealDamage.damage = Energy(clamp((dotRelVel / ramVelocity) * (1.0f + thresholdUp + thresholdDown) - thresholdDown, 0.0f, 1.0f) * 40.0f);
												dealDamage.damageType = DamageType::Physical;
												dealDamage.meleeDamage = true;
												ai_log(self, TXT("ram damage %.3f"), dealDamage.damage);
												if (dealDamage.damage > Energy(10))
												{
													DamageInfo damageInfo;
													damageInfo.damager = imo;
													damageInfo.source = imo;
													damageInfo.instigator = imo;
													damageInfo.hitRelativeDir = op->get_placement().vector_to_local(presenceVelocityOpWS).normal();
													// no dir
													todo_note(TXT("adjust received damage"));

													ContinuousDamage dealContinuousDamage;
													dealContinuousDamage.setup_using_weapon_core_modifier_companion_for(dealDamage);

													health->adjust_damage_on_hit_with_extra_effects(REF_ dealDamage);
													health->deal_damage(dealDamage, dealDamage, damageInfo);
													if (dealContinuousDamage.is_valid())
													{
														health->add_continuous_damage(dealContinuousDamage, damageInfo);
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
				if (actualHit)
				{
					if (auto* sound = imo->get_sound())
					{
						sound->play_sound(NAME(ramHit));
					}
					if (auto* tos = imo->get_temporary_objects())
					{
						tos->spawn(NAME(ramHit));
					}
					imo->get_presence()->set_velocity_linear(-imo->get_presence()->get_velocity_linear().normal() * max(imo->get_presence()->get_velocity_linear().length() * 0.3f, 5.0f));
				}
			}
		}
		LATENT_YIELD();
	}

	LATENT_SECTION_INFO(TXT("recover"));
	imo->activate_default_movement();
	if (chargeToRam.is_set())
	{
		if (auto* to = fast_cast<Framework::TemporaryObject>(chargeToRam.get()))
		{
			to->desire_to_deactivate();
		}
		chargeToRam.clear();
	}
	// recover
	allowOthersToRam = true;
	if (mind)
	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.stop();
	}
	if (imo)
	{
		if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
		{
			em->emissive_deactivate(NAME(ram));
		}
	}
	if (berserkerMode)
	{
		LATENT_WAIT(0.2f / aggressiveness_coef(2.0f));
	}
	else
	{
		LATENT_WAIT(2.0f / aggressiveness_coef(2.0f));
	}

	LATENT_ON_BREAK();
	//
	if (ramSound.is_set())
	{
		ramSound->stop();
	}
	if (chargeToRam.is_set())
	{
		if (auto* to = fast_cast<Framework::TemporaryObject>(chargeToRam.get()))
		{
			to->desire_to_deactivate();
		}
		chargeToRam.clear();
	}
	if (mind)
	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.stop();
	}
	if (imo)
	{
		imo->activate_default_movement();
		if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
		{
			em->emissive_deactivate(NAME(ram));
		}
	}

	LATENT_END_CODE();
	perceptionEnemyLock.override_lock(NAME(ram));
	
	LATENT_RETURN();
}

static LATENT_FUNCTION(follow_enemy)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("follow enemy"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(float, aggressiveness);
	LATENT_COMMON_VAR(bool, berserkerMode);
	LATENT_COMMON_VAR_END();

	LATENT_OBJECT_VAR_BEGIN();
	LATENT_OBJECT_VAR(bool, allowAttack);
	LATENT_OBJECT_VAR_END();
	LATENT_VAR(bool, prevAllowAttack);

	LATENT_VAR(Vector3, currentLocation);
	LATENT_VAR(float, updateTime);
	LATENT_VAR(Name, gait);

	LATENT_BEGIN_CODE();

	ai_log(mind->get_logic(), TXT("follow enemy"));

	currentLocation = Vector3::zero;
	updateTime = 0.5f;

	LATENT_WAIT(Random::get_float(0.1f, 0.3f));

	prevAllowAttack = allowAttack;
	while (true)
	{
		{
			if (mind && enemyPlacement.is_active())
			{
				auto * presence = mind->get_owner_as_modules_owner()->get_presence();
				currentLocation = presence->get_centre_of_presence_WS();
				updateTime = 0.5f;
				float distance = enemyPlacement.calculate_string_pulled_distance();
				gait = distance > 6.0f ? NAME(fast) : NAME(normal);
				::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
				{
					updateTime = 0.1f;
					auto const * throughDoor = enemyPlacement.get_through_door();
					if (throughDoor)
					{
						Vector3 holeCentre = throughDoor->get_hole_centre_WS();
						Vector3 navPoint = holeCentre + throughDoor->get_dir_inside() * 0.5f;
						if (Segment(holeCentre, navPoint).get_distance(currentLocation) > 0.5f)
						{
							locomotion.move_to_3d(navPoint, 0.1f, Framework::MovementParameters().gait(gait).relative_speed(0.5f));
							locomotion.turn_in_movement_direction_2d();
							locomotion.dont_keep_distance();
						}
						else
						{
							locomotion.move_to_3d(holeCentre + throughDoor->get_dir_outside() * 2.0f, 0.1f, Framework::MovementParameters().gait(gait).relative_speed(0.5f)); // move through door!
							locomotion.turn_in_movement_direction_2d();
							locomotion.dont_keep_distance();
						}
					}
					else
					{
						gait = distance > 6.0f ? NAME(fast) : NAME(keepClose);
						if (auto * target = enemyPlacement.get_target())
						{
							locomotion.move_to_3d(target, 0.2f, Framework::MovementParameters().gait(gait).relative_speed(berserkerMode? 2.0f: 1.0f));
							locomotion.turn_towards_2d(target, 1.0f);
							locomotion.keep_distance_2d(target, 1.0f, 2.0f, 0.5f);
						}
						else
						{
							locomotion.move_to_3d(enemyPlacement.get_placement_in_owner_room().get_translation(), 0.2f, Framework::MovementParameters().gait(gait).relative_speed(berserkerMode ? 2.0f : 1.0f));
							locomotion.turn_towards_2d(enemyPlacement.get_placement_in_owner_room().get_translation(), 1.0f);
							locomotion.keep_distance_2d(enemyPlacement.get_placement_in_owner_room().get_translation(), 1.0f, 2.0f, 0.5f);
						}
					}
				}
			}
			else
			{
				LATENT_BREAK();
			}
		}
		if (!allowAttack && (prevAllowAttack ^ allowAttack))
		{
			ai_log(mind->get_logic(), TXT("no desire to attack enemy?"));
			if (Random::get_chance(0.8f))
			{
				ai_log(mind->get_logic(), TXT("doesn't want to attack any more"));
				aggressiveness = 0.0f;
				LATENT_BREAK();
			}
		}
		prevAllowAttack = allowAttack;

		LATENT_WAIT(updateTime);
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

static LATENT_FUNCTION(circle_around_enemy)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("circle around enemy"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_PARAM(bool, goRight);
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(float, aggressiveness);
	LATENT_COMMON_VAR_END();

	LATENT_OBJECT_VAR_BEGIN();
	LATENT_OBJECT_VAR(bool, allowAttack);
	LATENT_OBJECT_VAR_END();
	LATENT_VAR(bool, prevAllowAttack);

	LATENT_VAR(Vector3, currentLocation);
	LATENT_VAR(float, updateTime);
	LATENT_VAR(float, circlingTimeLeft);

	auto* imo = mind->get_owner_as_modules_owner();

	circlingTimeLeft -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	ai_log(mind->get_logic(), TXT("circle around enemy"));

	currentLocation = Vector3::zero;
	updateTime = 0.5f;
	circlingTimeLeft = Random::get_float(10.0f, 35.0f);

	prevAllowAttack = allowAttack;
	while (true)
	{
		{
			if (mind && enemyPlacement.is_active() && !enemyPlacement.get_through_door())
			{
				::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
				{
					auto * presence = imo->get_presence();
					currentLocation = presence->get_centre_of_presence_WS();
					updateTime = 0.5f;

					Vector3 targetLocation = AI::Targetting::get_enemy_centre_of_presence_in_owner_room(imo);
#ifdef AN_DEVELOPMENT
#ifndef AN_CLANG
					float radius = (currentLocation - targetLocation).length();
#endif
#endif
					Vector3 towardsTarget = targetLocation - currentLocation;
					Vector3 upDir = presence->get_environment_up_dir();
					Vector3 towardsTargetFlat = towardsTarget.drop_using(upDir);
					Vector3 rightDir = Vector3::cross(towardsTargetFlat, upDir);
					Vector3 navPoint = currentLocation + rightDir * 2.0f * (goRight ? 1.0f : -1.0f);
					navPoint += upDir * Random::get_float(-1.5f, 1.5f);

					locomotion.move_to_3d(navPoint, 0.4f, Framework::MovementParameters().full_speed());
					if (enemyPlacement.get_target())
					{
						locomotion.turn_towards_2d(enemyPlacement.get_target(), 1.0f);
						locomotion.keep_distance_2d(enemyPlacement.get_target(), 1.0f, 2.0f, 0.5f);
					}
					else
					{
						locomotion.turn_towards_2d(targetLocation, 1.0f);
						locomotion.keep_distance_2d(targetLocation, 1.0f, 2.0f, 0.5f);
					}
				}
			}
			else
			{
				LATENT_BREAK();
			}
		}
		if (!allowAttack && (prevAllowAttack ^ allowAttack))
		{
			ai_log(mind->get_logic(), TXT("no desire to attack enemy?"));
			if (Random::get_chance(0.8f))
			{
				ai_log(mind->get_logic(), TXT("doesn't want to attack any more"));
				aggressiveness = 0.0f;
				LATENT_BREAK();
			}
		}
		prevAllowAttack = allowAttack;

		updateTime = 0.2f;
		LATENT_WAIT(updateTime);
		if (circlingTimeLeft < 0.0f)
		{
			LATENT_BREAK();
		}
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Rammer::execute_logic)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
#ifdef AN_USE_AI_LOG
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
#else
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
#endif
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(float, aggressiveness);
	LATENT_COMMON_VAR(bool, berserkerMode);
	LATENT_COMMON_VAR(Name, scriptedBehaviour);
	LATENT_COMMON_VAR_END();

	LATENT_OBJECT_VAR_BEGIN();
	LATENT_OBJECT_VAR(bool, allowAttack);
	LATENT_OBJECT_VAR_END();

	LATENT_VAR(bool, chatter);
	LATENT_VAR(bool, enemyAvailable);
	LATENT_VAR(float, enemyUnavailableFor);
	LATENT_VAR(bool, dieNow);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, perceptionTask);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, updateBeaconTask);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);
	LATENT_VAR(::Framework::AI::LatentTaskInfoWithParams, nextTask);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);
	LATENT_VAR(float, timeToCircle);
	LATENT_VAR(float, timeToRam);
	LATENT_VAR(float, timeToUpdateBerserkerMode);
	LATENT_VAR(bool, ramNow);
	LATENT_VAR(float, reactionTimeLeft);

	LATENT_VAR(::Framework::SoundSourcePtr, hitSound);

	reactionTimeLeft -= LATENT_DELTA_TIME;
	timeToCircle -= LATENT_DELTA_TIME;
	if (!ramNow)
	{
		if (berserkerMode)
		{
			timeToRam = min(timeToRam, 5.0f);
		}
		timeToRam -= LATENT_DELTA_TIME;
		if (timeToRam <= 0.0f)
		{
			ramNow = true;
			timeToRam = Random::get_float(2.0f, max(5.0f, 20.0f - GameSettings::get().ai_agrressiveness_as_float() * 6.0f));
		}
	}
	timeToUpdateBerserkerMode -= LATENT_DELTA_TIME;

	aggressiveness = max(0.0f, aggressiveness - LATENT_DELTA_TIME * (aggressiveness < 3.0f? 0.25f : 0.1f) / aggressiveness_coef());

	if (enemyAvailable)
	{
		enemyUnavailableFor = 0.0f;
	}
	else
	{
		enemyUnavailableFor += LATENT_DELTA_TIME;
	}
	
	Rammer* self = fast_cast<Rammer>(mind->get_logic());

	LATENT_BEGIN_CODE();

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::perception));
		perceptionTask.start_latent_task(mind, taskInfo);
	}

	chatter = false;

	reactionTimeLeft = 0.0f;
	timeToCircle = Random::get_float(10.0f, 30.0f);
	timeToRam = 0.0f;
	ramNow = false;
	allowAttack = false;
	enemyUnavailableFor = 0.0f;
	enemyAvailable = false;

	messageHandler.use_with(mind);
	{
		auto * framePtr = &_frame;
		messageHandler.set(NAME(dealtDamage), [mind, &hitSound, &reactionTimeLeft](Framework::AI::Message const & _message)
		{
			reactionTimeLeft = 0.0f; // stop reaction
			if (hitSound.is_set() && hitSound->is_playing())
			{
				return;
			}
			if (auto* damageUnadjusted = _message.get_param(NAME(damageUnadjusted)))
			{
				if (auto* damage = _message.get_param(NAME(damage)))
				{
					if (damage->get_as<Damage>().damage > damageUnadjusted->get_as<Damage>().damage.halved())
					{
						if (auto* sound = mind->get_owner_as_modules_owner()->get_sound())
						{
							hitSound = sound->play_sound(NAME(hit));
						}
					}
				}
			}
		}
		);
		messageHandler.set(NAME(projectileHit), [mind, framePtr, &nextTask, &allowAttack, &aggressiveness](Framework::AI::Message const & _message)
		{
			if (auto* damageParam = _message.get_param(NAME(damage)))
			{
				auto & damage = damageParam->get_as<Damage>();
				float importance = damage.damage.as_float() / 25.0f;
				if (!allowAttack)
				{
					aggressiveness = 0.0f;
					ai_log(mind->get_logic(), TXT("hit, drop aggressiveness to 0 and get out of there"));
				}
				else
				{
					if (auto* instigator = _message.get_param(NAME(instigator)))
					{
						if (auto* hit = _message.get_param(NAME(hit)))
						{
							auto* hitImo = hit->get_as<SafePtr<Framework::IModulesOwner>>().get();
							if (hitImo == mind->get_owner_as_modules_owner())
							{
								aggressiveness += Random::get_float(1.2f, 4.0f) * aggressiveness_coef() * importance;
							}
							else if (GameSettings::get().ai_agrressiveness_as_float() >= 1.5f &&
								mind->access_social().is_enemy_or_owned_by_enemy(instigator->get_as<SafePtr<Framework::IModulesOwner>>().get()) &&
								!mind->access_social().is_enemy_or_owned_by_enemy(hitImo))
							{
								aggressiveness += Random::get_float(0.2f, 1.2f) * aggressiveness_coef() * importance;
							}
						}
					}
				}
				if (Tasks::handle_projectile_hit_message(mind, nextTask, _message, 0.6f, Tasks::avoid_projectile_hit_3d, 20))
				{
					aggressiveness += 0.3f * aggressiveness_coef() * importance;
					framePtr->end_waiting();
					AI_LATENT_STOP_LONG_RARE_ADVANCE();
				}
			}
		}
		);
		messageHandler.set(NAME(death), [mind, &aggressiveness](Framework::AI::Message const & _message)
		{
			if (auto * imo = mind->get_owner_as_modules_owner())
			{
				if (auto * ms = imo->get_sound())
				{
					ms->stop_sound(NAME(engine));
					ms->stop_sound(NAME(chatter));
				}
			}
			aggressiveness += Random::get_float(0.2f, 1.5f); // why?
		}
		);
		messageHandler.set(NAME(die), [framePtr, mind, &dieNow](Framework::AI::Message const & _message)
		{
			dieNow = true;
			framePtr->end_waiting();
			AI_LATENT_STOP_LONG_RARE_ADVANCE();
		}
		);
		/*
		messageHandler.set(NAME(entersRoom), [mind, framePtr, &nextTask, &aggressiveness, &followTarget](Framework::AI::Message const & _message)
		{
			if (GameSettings::get().ai_agrressiveness_as_float() >= 0.0f && !followTarget.has_target() && Random::get_chance(0.15f + GameSettings::get().ai_agrressiveness_as_float() * 0.2f))
			{
				if (auto* who = _message.get_param(NAME(who)))
				{
					auto* whoImo = who->get_as<SafePtr<Framework::IModulesOwner>>().get();
					if (whoImo == whoImo->get_top_instigator())
					{
						if (mind->access_social().is_enemy(whoImo))
						{
							ai_log(mind->get_logic(), TXT("entered room, follow!"));
							aggressiveness += Random::get_float(2.2f, 4.5f) * aggressiveness_coef();
							followTarget.find_path(mind->get_owner_as_modules_owner(), whoImo);
						}
					}
				}
			}
		}
		);
		*/
		messageHandler.set(NAME(rotateOnDamage), [
#ifdef AN_USE_AI_LOG
			logic,
#endif
			&currentTask, &reactionTimeLeft](Framework::AI::Message const & _message)
		{
			ai_log(logic, TXT("rotate on damage"));
			currentTask.stop();
			reactionTimeLeft = Random::get_float(2.0f, 3.0f);
		}
		);
	}

	{	// sound
		if (auto * imo = mind->get_owner_as_modules_owner())
		{
			if (auto * ms = imo->get_sound())
			{
				ms->play_sound(NAME(engine));
			}
		}
	}

	{	// update beacon task - the one we fly around when idling
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(update_beacon));
		updateBeaconTask.start_latent_task(mind, taskInfo);
	}

	aggressiveness = Random::get_float(0.0f, 1.1f);

	LATENT_WAIT(Random::get_float(0.1f, 0.6f));

	if (self->get_related_device_id() != 0)
	{
		self->watchedDevices.clear();
		if (auto* world = mind->get_owner_as_modules_owner()->get_in_world())
		{
			world->for_every_object_with_id(self->get_related_device_id_var_name(), self->get_related_device_id(),
				[self](Framework::Object* _object)
			{
				if (_object)
				{
					self->watchedDevices.push_back(SafePtr<Framework::IModulesOwner>(_object));
				}
				return false; // keep going on
			});
		}
	}

	while (true)
	{
		if (dieNow)
		{
			nextTask.propose(AI_LATENT_TASK_FUNCTION(death_sequence), NP, NP, false, true);
			currentTask.start_latent_task(mind, nextTask);
			while (true)
			{
				// just wait here until we die
				LATENT_YIELD();
			}
		}

		if (Common::handle_scripted_behaviour(mind, scriptedBehaviour, currentTask))
		{
			// doing scripted behaviour
		}
		else
		{
			if (reactionTimeLeft > 0.0f)
			{
				{	// stop any movement
					auto& locomotion = mind->access_locomotion();
					locomotion.stop();
					locomotion.turn_in_movement_direction_2d();
				}
				currentTask.stop();
				timeToCircle = 0.0f;
			}
			else
			{
				if (Random::get_chance(0.01f))
				{
					aggressiveness += Random::get_float(0.1f, 1.3f) * aggressiveness_coef();
					ai_log(logic, TXT("modify aggressiveness randomly to %.3f"), aggressiveness);
				}
				aggressiveness = clamp(aggressiveness, 0.0f, 5.0f);

				if (berserkerMode)
				{
					aggressiveness = max(aggressiveness, 5.0f);
				}

				{
					auto* imo = mind->get_owner_as_modules_owner();

					bool enemyWasAvailable = enemyAvailable;

					if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
					{
						if (berserkerMode)
						{
							em->emissive_activate(NAME(berserker));
						}
						else
						{
							em->emissive_deactivate(NAME(berserker));
						}
					}
					{
#ifdef AN_DEVELOPMENT
#ifndef AN_CLANG
						bool prevAllowAttack = allowAttack && enemyAvailable;
#endif
#endif
						if (self->is_ai_aggressive() || berserkerMode)
						{
							allowAttack = true;
							if (aggressiveness < 3.0f && Random::get_chance(0.1f))
							{
								aggressiveness += Random::get_float(0.1f, 0.3f);
							}
						}
						else
						{
							allowAttack = false;

							// check if there is anything damaged or disabled
							for_every_ref(watchedDevice, self->watchedDevices)
							{
								if (auto* od = watchedDevice->get_custom<CustomModules::OperativeDevice>())
								{
									if (od->is_not_fully_operative_because_of_user())
									{
										allowAttack = true;
										break;
									}
								}
							}

							if (allowAttack)
							{
								aggressiveness = max(2.0f, aggressiveness);
							}
						}
					}

					if (GameSettings::get().difficulty.aiAggressiveness == GameSettings::AIAgressiveness::Ignores)
					{
						aggressiveness = 0.0f;
					}

					if (enemyPlacement.is_active())
					{
						if (!enemyPlacement.get_through_door())
						{
							if (aggressiveness > 0.6f)
							{
								if (!enemyAvailable)
								{
									ai_log(logic, TXT("interested in target at %.3f"), aggressiveness);
								}
								enemyAvailable = true;
							}
						}
						else
						{
							if (enemyAvailable)
							{
								ai_log(logic, TXT("target left"));
							}
							enemyAvailable = false;
						}
					}

					if (aggressiveness < 0.4f && enemyAvailable)
					{
						ai_log(logic, TXT("resigned from target at %.3f"), aggressiveness);
						enemyAvailable = false;
						aggressiveness *= Random::get_float(0.0f, 0.8f);
					}

					/*
					if (enemyWasAvailable && !enemyAvailable)
					{
						ai_log(logic, TXT("lost target, cool down aggressiveness once"));
						aggressiveness *= Random::get_float(0.4f, 0.8f);
					}
					*/

					if (!enemyWasAvailable && enemyAvailable)
					{
						if (enemyUnavailableFor > 10.0f)
						{
							ai_log(logic, TXT("not seen enemy for a long time, be interested"));
							aggressiveness += Random::get_float(0.4f, 2.8f);
						}
					}

					bool shouldChatter = (allowAttack && enemyAvailable && enemyPlacement.get_target() != nullptr) || berserkerMode;

					if (shouldChatter ^ chatter)
					{
						chatter = shouldChatter;
						if (auto* ms = imo->get_sound())
						{
							if (chatter)
							{
								ms->play_sound(NAME(chatter));
							}
							else
							{
								ms->stop_sound(NAME(chatter));
							}
						}
					}

					if (currentTask.can_start_new())
					{
						// choose best action for now
						nextTask.propose(AI_LATENT_TASK_FUNCTION(fly_around_beacon));
						if (self->get_to_stay_in_room().is_active() &&
							self->get_to_stay_in_room().get_through_door())
						{
							if (nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::fly_to), 100))
							{
								ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::RelativeToPresencePlacement*, &self->get_to_stay_in_room());
								ADD_LATENT_TASK_INFO_PARAM(nextTask, bool, true); // fly just to room
							}
						}
						if (enemyPlacement.is_active() && enemyAvailable)
						{
							if (timeToCircle < 0.0f &&
								!enemyPlacement.get_through_door() &&
								enemyPlacement.calculate_string_pulled_distance() < 4.0f)
							{
								if (nextTask.propose(AI_LATENT_TASK_FUNCTION(circle_around_enemy), 5))
								{
									ADD_LATENT_TASK_INFO_PARAM(nextTask, bool, Random::get_chance(0.5f));
								}
								timeToCircle = Random::get_float(10.0f, 30.0f);
							}
							nextTask.propose(AI_LATENT_TASK_FUNCTION(follow_enemy));
							todo_note(TXT("evade on option"));
							/*
							if (Tasks::is_aiming_at_me(enemyPlacement, imo, 0.2f, 10.0f, 6.0f))
							{
								if (nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::evade_aiming_3d), 10, NP, false))
								{
									ai_log(logic, TXT("evade!"));
									ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::PresencePath*, &followTarget);
								}
							}
							*/
							if (allowAttack && ramNow && enemyPlacement.calculate_string_pulled_distance() < 10.0f)
							{
								if (GameSettings::get().ai_agrressiveness_as_float() > 0.0f)
								{
									nextTask.propose(AI_LATENT_TASK_FUNCTION(ram_enemy), 50);
								}
							}
						}

						// check if we can start new one, if so, start it
						if (currentTask.can_start(nextTask))
						{
							currentTask.start_latent_task(mind, nextTask);
						}
						nextTask.clear();
					}
				}
				ramNow = false;
			}
		}
		LATENT_WAIT(Random::get_float(0.05f, 0.25f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	updateBeaconTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(RammerData);

RammerData::RammerData()
{
}

RammerData::~RammerData()
{
}

bool RammerData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool RammerData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
