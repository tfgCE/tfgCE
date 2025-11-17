#include "aiLogicTask_lightningRandomDischarge.h"

#include "..\..\aiRayCasts.h"
#include "..\..\..\game\damage.h"
#include "..\..\..\game\gameSettings.h"
#include "..\..\..\library\library.h"
#include "..\..\..\modules\custom\health\mc_health.h"

#include "..\..\..\..\core\collision\iCollidableShape.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\ai\aiTaskHandle.h"
#include "..\..\..\..\framework\collision\checkSegmentResult.h"
#include "..\..\..\..\framework\game\gameConfig.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\..\framework\module\custom\mc_lightningSpawner.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\..\framework\presence\presencePath.h"
#include "..\..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\..\framework\world\presenceLink.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// variables
DEFINE_STATIC_NAME(dischargeDamage);

// lightnings
DEFINE_STATIC_NAME(discharge);
DEFINE_STATIC_NAME_STR(dischargeMiss, TXT("discharge miss"));

// temporary objects
DEFINE_STATIC_NAME_STR(lightningHit, TXT("lightning hit"));

// global references
DEFINE_STATIC_NAME_STR(grLightningProjectileTypeTags, TXT("projectile; lightning; projectile type tags"));

//

LATENT_FUNCTION(TeaForGodEmperor::AI::Logics::Tasks::lightning_random_discharge)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("lightning random discharge"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("lightning random discharge"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::RelativeToPresencePlacement, aimAt);
	LATENT_VAR(Framework::CustomModules::LightningSpawner*, lightningSpawner);
	LATENT_VAR(int, quickDischargesLeft);

	LATENT_VAR(Tags, lightningProjectileTypeTags);

	auto* imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	lightningSpawner = imo->get_custom<Framework::CustomModules::LightningSpawner>();

	lightningProjectileTypeTags = Library::get_current()->get_global_references().get_tags(NAME(grLightningProjectileTypeTags));

	quickDischargesLeft = 0;

	while (true)
	{
		if (quickDischargesLeft > 0)
		{
			--quickDischargesLeft;
		}
		if (quickDischargesLeft <= 0)
		{
			quickDischargesLeft = 0;
			aimAt.clear_target();
		}
		LATENT_WAIT(quickDischargesLeft ? Random::get_float(0.05f, 0.3f) : Random::get_float(0.2f, 0.8f));

		LATENT_CLEAR_LOG();

		{
			PERFORMANCE_GUARD_LIMIT(0.005f, TXT("lightning random discharge [discharge]"));
			//debug_context(imo->get_presence()->get_in_room());
			Framework::CheckSegmentResult result;
			Framework::RelativeToPresencePlacement relativePlacement;
			relativePlacement.set_owner(imo);
			Transform centrePlacement = imo->get_presence()->get_centre_of_presence_transform_WS();
			Transform strikePlacement = look_at_matrix(centrePlacement.get_translation(),
				centrePlacement.location_to_world(Rotator3(Random::get_float(-15.0f, 40.0f), Random::get_chance(0.7f)? Random::get_float(-20.0f, 20.0f) : Random::get_float(-180.0f, 180.0f), 0.0f).get_forward()),
				centrePlacement.get_axis(Axis::Up)).to_transform();
			Vector3 startLoc = strikePlacement.get_translation();
			Vector3 dir = strikePlacement.get_axis(Axis::Forward);
			Transform targetPlacement = strikePlacement;
			float maxDist = 2.5f;
			Framework::IModulesOwner* attackTarget = nullptr;
			Framework::Room* attackInRoom = nullptr;

			if (aimAt.is_active() && Random::get_chance(0.9f))
			{
				attackTarget = aimAt.get_target();
				attackInRoom = aimAt.get_in_final_room();
				targetPlacement = aimAt.get_placement_in_owner_room();
			}

			if (attackTarget)
			{
				dir = (targetPlacement.get_translation() + Random::get_float(0.0f, 0.3f) * Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-2.0f, 2.0f), Random::get_float(-1.0f, 1.0f)).normal() - startLoc).normal();
			}
			targetPlacement.set_translation(startLoc + (dir * maxDist));
			bool hitSomething = false;
			{
				PERFORMANCE_GUARD_LIMIT(0.001f, TXT("[lightning random discharge] ray cast"));
				hitSomething = do_ray_cast(CastInfo().at_target().with_collision_flags(Framework::GameConfig::projectile_collides_with_flags()),
					startLoc, imo,
					attackTarget,
					attackInRoom,
					targetPlacement, result, &relativePlacement);
			}
			if (hitSomething && relativePlacement.get_straight_length() < maxDist)
			{
				PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[lightning random discharge] discharge HIT"));
				auto& usePlacement = relativePlacement;
				auto* enemy = fast_cast<::Framework::IModulesOwner>(result.object);
				LATENT_LOG(TXT("discharge"));
				lightningSpawner->single(NAME(discharge), Framework::CustomModules::LightningSpawner::LightningParams().with_target(usePlacement, hitSomething));
				bool didHitSomethingWithHealth = false;
				if (enemy)
				{
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
								if (PhysicalMaterial::ForProjectile const* useForProjectile = PhysicalMaterial::get_for_projectile(result.material, dealDamage, lightningProjectileTypeTags))
								{
									dealDamage.apply(useForProjectile, nullptr);
								}
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
				}
				// hit
				if (didHitSomethingWithHealth)
				{
					if (quickDischargesLeft == 0)
					{
						aimAt = relativePlacement;
						quickDischargesLeft = Random::get_int_from_range(4, 6);
					}
					if (auto* tos = imo->get_temporary_objects())
					{
						if (result.hitNormal.is_normalised())
						{
							auto* lightningHit = tos->spawn(NAME(lightningHit));
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
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	//

	LATENT_RETURN();
}

//
