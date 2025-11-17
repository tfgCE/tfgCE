#include "aiAction_lightningStrike.h"

#include "..\aiLogic_npcBase.h"

#include "..\..\aiRayCasts.h"

#include "..\..\..\game\damage.h"
#include "..\..\..\game\energy.h"
#include "..\..\..\game\gameDirector.h"
#include "..\..\..\library\library.h"
#include "..\..\..\modules\custom\health\mc_health.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\collision\checkSegmentResult.h"
#include "..\..\..\..\framework\game\gameConfig.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleSound.h"
#include "..\..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\..\framework\module\custom\mc_lightningSpawner.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\..\framework\world\presenceLink.h"

#include "..\..\..\..\core\collision\iCollidableShape.h"

#include "..\..\..\..\core\debug\debugRenderer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// lightnings
DEFINE_STATIC_NAME(discharge);

// variables
DEFINE_STATIC_NAME(dischargeDamage);

// temporary objects
DEFINE_STATIC_NAME_STR(lightningHit, TXT("lightning hit"));

// global references
DEFINE_STATIC_NAME_STR(grLightningProjectileTypeTags, TXT("projectile; lightning; projectile type tags"));

//

bool Actions::do_lightning_strike(Framework::AI::MindInstance* mind, Framework::RelativeToPresencePlacement & enemyPlacement, float maxDist, Optional<LightningStrikeParams> const& _lightningStrikeParams)
{
	LightningStrikeParams lightningStrikeParams = _lightningStrikeParams.get(LightningStrikeParams());
	if (GameDirector::is_violence_disallowed() && !lightningStrikeParams.ignoreViolenceDisallowed.get(false) && !lightningStrikeParams.forceDischarge.get(false))
	{
		return false;
	}
	if (enemyPlacement.is_active())
	{
#ifdef AN_USE_AI_LOG
		auto* logic = mind->get_logic();
#endif
		auto* npcBase = fast_cast<NPCBase>(mind->get_logic());
		auto* imo = mind->get_owner_as_modules_owner();

		auto* lightningSpawner = imo->get_custom<Framework::CustomModules::LightningSpawner>();

		an_assert(lightningSpawner);
		if (!lightningSpawner)
		{
			return false;
		}

		Framework::CheckSegmentResult result;
		Framework::RelativeToPresencePlacement relativePlacement;
		relativePlacement.be_temporary_snapshot();
		relativePlacement.set_owner(imo);
		Transform startPlacement;
		if (lightningStrikeParams.fromSocket.is_set() && lightningStrikeParams.fromSocket.get().is_valid())
		{
			startPlacement = imo->get_presence()->get_placement().to_world(imo->get_appearance()->calculate_socket_os(lightningStrikeParams.fromSocket.get()));
		}
		else
		{
			startPlacement = imo->get_presence()->get_placement().to_world(imo->get_appearance()->calculate_socket_os(npcBase->get_perception_socket_index()));
		}
		Vector3 startLoc = startPlacement.get_translation();
		Transform targetPlacement = enemyPlacement.get_placement_in_owner_room();
		if (lightningStrikeParams.extraDistance.is_set())
		{
			Vector3 tLoc = targetPlacement.get_translation();
			float len = (tLoc - startLoc).length();
			if (len != 0.0f)
			{
				tLoc = startLoc + (tLoc - startLoc) * ((len + lightningStrikeParams.extraDistance.get()) / len);
				targetPlacement.set_translation(tLoc);
			}
		}
		bool hitSomething = AI::do_ray_cast(CastInfo().at_target().with_collision_flags(Framework::GameConfig::projectile_collides_with_flags()),
			startLoc, imo,
			enemyPlacement.get_target(),
			enemyPlacement.get_in_final_room(), targetPlacement, result, &relativePlacement);
		{
			/*
			debug_context(imo->get_presence()->get_in_room());
			debug_draw_time_based(10.0f, debug_draw_arrow(true, Colour::green, startLoc, targetPlacement.get_translation()));
			if (hitSomething)
			{
				debug_draw_time_based(10.0f, debug_draw_arrow(true, Colour::red, startLoc, relativePlacement.get_placement_in_owner_room().get_translation()));
			}
			debug_no_context();
			*/
		}
		if (hitSomething && relativePlacement.get_straight_length() < maxDist)
		{
			if (lightningStrikeParams.angleLimit.is_set() || lightningStrikeParams.verticalAngleLimit.is_set())
			{
				Vector3 hitLoc = relativePlacement.get_placement_in_owner_room().get_translation();
				hitLoc = startPlacement.location_to_local(hitLoc);
				Rotator3 hitDir = hitLoc.to_rotator();
				if (lightningStrikeParams.verticalAngleLimit.is_set())
				{
					if (abs(hitDir.pitch) > lightningStrikeParams.verticalAngleLimit.get())
					{
						return false;
					}
				}
				else if (lightningStrikeParams.angleLimit.is_set())
				{
					if (abs(hitDir.pitch) > lightningStrikeParams.angleLimit.get())
					{
						return false;
					}
				}
				if (lightningStrikeParams.angleLimit.is_set())
				{
					if (abs(hitDir.yaw) > lightningStrikeParams.angleLimit.get())
					{
						return false;
					}
				}
			}
			auto & usePlacement = relativePlacement;// hitSomething ? relativePlacement : enemyPlacement;
			auto* enemy = fast_cast<::Framework::IModulesOwner>(result.object);// hitSomething ? fast_cast<::Framework::IModulesOwner>(result.object) : usePlacement.get_target()
			ai_log(logic, TXT("discharge"));
			lightningSpawner->single(lightningStrikeParams.lightningSpawnID.get(NAME(discharge)), Framework::CustomModules::LightningSpawner::LightningParams().with_target(usePlacement, hitSomething));
			if (auto* s = imo->get_sound())
			{
				s->play_sound(lightningStrikeParams.lightningSpawnID.get(NAME(discharge)));
			}
			if (enemy)
			{
				ai_log(logic, TXT("hit %S"), enemy->ai_get_name().to_char());
				auto* check = enemy;

				Tags lightningProjectileTypeTags = Library::get_current()->get_global_references().get_tags(NAME(grLightningProjectileTypeTags));

				while (check)
				{
					if (auto* health = check->get_custom<CustomModules::Health>())
					{
						Damage dealDamage;
						if (lightningStrikeParams.dischargeDamage.is_set())
						{
							dealDamage.damage = lightningStrikeParams.dischargeDamage.get();
						}
						else
						{
							dealDamage.damage = imo->get_variables().get_value(NAME(dischargeDamage), lightningStrikeParams.dischargeDamageFallback.get(Energy::zero()));
						}
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
						ai_log(logic, TXT("do damage %.0f to %S"), dealDamage.damage.as_float(), check->ai_get_name().to_char());
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
						break;
					}
					check = check->get_instigator(true);
				}
			}
			// play temporary object - lightning hit
			if (!lightningStrikeParams.noHitTemporaryObject.get(false))
			{
				if (auto* tos = imo->get_temporary_objects())
				{
					auto* lightningHit = tos->spawn(lightningStrikeParams.hitTemporaryObjectID.get(NAME(lightningHit)));
					if (lightningHit)
					{
						Transform placement = matrix_from_up(result.hitLocation, result.hitNormal).to_transform();
						if (auto* at = result.presenceLink ? result.presenceLink->get_modules_owner() : nullptr)
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
			//
			return true;
		}
	}

	return false;
}
