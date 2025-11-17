#include "lightningDischarge.h"

#include "..\library\library.h"
#include "..\modules\custom\health\mc_health.h"
#include "..\utils\collectInTargetCone.h"

#include "..\game\gameDirector.h"

#include "..\..\core\collision\shape.h"

#include "..\..\framework\framework.h"
#include "..\..\framework\ai\aiMindInstance.h"
#include "..\..\framework\game\gameConfig.h"
#include "..\..\framework\module\moduleAppearance.h"
#include "..\..\framework\module\moduleSound.h"
#include "..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\framework\module\custom\mc_lightningSpawner.h"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\framework\object\temporaryObject.h"
#include "..\..\framework\world\presenceLink.h"
#include "..\..\framework\world\room.h"

#include "..\..\core\profilePerformanceSettings.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// temporary objects
DEFINE_STATIC_NAME(discharge);
DEFINE_STATIC_NAME_STR(dischargeMiss, TXT("discharge miss"));
DEFINE_STATIC_NAME_STR(lightningHit, TXT("lightning hit"));

// global references
DEFINE_STATIC_NAME_STR(grLightningProjectileTypeTags, TXT("projectile; lightning; projectile type tags"));

//

bool LightningDischarge::perform(Params const& _params)
{
	if (! _params.ignoreNarrative.get(false) && GameDirector::is_violence_disallowed() && !_params.ignoreViolenceDisallowed.get(false))
	{
		return false;
	}

	bool discharged = false;

	auto* imo = _params.imo;
	auto* instigator = _params.instigator? _params.instigator : imo;

	// for lightning we do actual gun fire ONLY if we hit something, we also use ammo only if we hit something
	auto* lightningSpawner = imo->get_custom<Framework::CustomModules::LightningSpawner>();

	an_assert(lightningSpawner, TXT("gun requires lightning"));
	if (lightningSpawner)
	{
		Transform startPlacementOS = _params.startPlacementOS;
		Transform startPlacement = imo->get_presence()->get_placement().to_world(startPlacementOS);
		Vector3 preStartLoc = imo->get_presence()->get_centre_of_presence_WS();
		Vector3 startLoc = startPlacement.get_translation();
		Vector3 endLoc = startPlacement.location_to_world(Vector3::yAxis * _params.maxDist + _params.endPlacementOffset);
		Vector3 missEndLoc = endLoc;

		bool hitSomething = false;
		Framework::CheckSegmentResult result;
		Framework::RelativeToPresencePlacement relativePlacement;
		relativePlacement.be_temporary_snapshot();
		relativePlacement.set_owner(imo);

		struct RayCastCheck
		{
			static void perform(
				Params const& _params,
				Framework::IModulesOwner* imo,
				Vector3 const& preStartLoc,
				Vector3 const& startLoc,
				Vector3 const& endLoc,
				OUT_ Vector3 & missEndLoc,
				OUT_ bool& hitSomething,
				OUT_ Framework::CheckSegmentResult & result,
				OUT_ Framework::RelativeToPresencePlacement & relativePlacement
			)
			{
				Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::WS);
				collisionTrace.add_location(preStartLoc);
				collisionTrace.add_location(startLoc);
				collisionTrace.add_location(endLoc);
				int collisionTraceFlags = Framework::CollisionTraceFlag::ResultInPresenceRoom | Framework::CollisionTraceFlag::FirstTraceThroughDoorsOnly;

				Framework::CheckCollisionContext checkCollisionContext;
				checkCollisionContext.collision_info_needed();
				checkCollisionContext.use_collision_flags(Framework::GameConfig::projectile_collides_with_flags());
				checkCollisionContext.avoid_up_to_top_instigator(imo); // so we won't hit the hand or body capsule

				relativePlacement.reset();
				relativePlacement.set_owner(imo);

				if (imo->get_presence()->trace_collision(AgainstCollision::Precise, collisionTrace, result, collisionTraceFlags, checkCollisionContext, &relativePlacement))
				{
					if (!_params.singleRoom.get(false) || !result.changedRoom)
					{
						missEndLoc = result.hitLocation; // we hit something else, show that
						// check if we hit an object with health module, accept only such
						auto* check = fast_cast<::Framework::IModulesOwner>(result.object);
						while (check)
						{
							if (check->get_custom<CustomModules::Health>())
							{
								hitSomething = true;
								break;
							}
							//check = check->get_instigator(true);
							check = check->get_presence()->get_attached_to();
						}
					}
				}
			}
		};

		// straight line
		if (!hitSomething && _params.rayCastSearchForObjects)
		{
			RayCastCheck::perform(_params, imo, preStartLoc, startLoc, endLoc, OUT_ missEndLoc, OUT_ hitSomething, OUT_ result, OUT_ relativePlacement);
		}

		// wide search, get via cone
		if (!hitSomething && _params.wideSearchForObjects)
		{
			Framework::Room* startInRoom = imo->get_presence()->get_in_room();

			Transform intoRoomTransform = Transform::identity;

			ArrayStatic<Framework::DoorInRoom*, CollectInTargetCone::Entry::MAX_DOORS> tempThroughDoorsSoFar; SET_EXTRA_DEBUG_INFO(tempThroughDoorsSoFar, TXT("Gun::shoot_lightning.tempThroughDoorsSoFar"));
			startInRoom->move_through_doors(Transform(preStartLoc, Quat::identity), Transform(startLoc, Quat::identity), OUT_ startInRoom, OUT_ intoRoomTransform);

			float distance = (endLoc - startLoc).length();
			ARRAY_STACK(CollectInTargetCone::Entry, collected, 32);
			{
				CollectInTargetCone::Params citcParams;
				citcParams.in_single_room(_params.singleRoom);
				citcParams.collect_health_owners();
				CollectInTargetCone::collect(intoRoomTransform.location_to_local(startLoc), intoRoomTransform.vector_to_local((endLoc - startLoc).normal()),
					distance, 15.0f, 0.25f, startInRoom, distance, collected, nullptr, intoRoomTransform, citcParams);
			}

			float bestScore = 0.0f;
			Optional<Vector3> bestLocWS;
			for_every(c, collected)
			{
				if (!bestLocWS.is_set() || c->score > bestScore)
				{
					bestScore = c->score;
					bestLocWS = c->closestLocationWS;
					/*
					debug_context(imo->get_presence()->get_in_room());
					debug_draw_time_based(1.0f, debug_draw_arrow(true, Colour::green, startLoc, bestLocWS.get()));
					debug_no_context();
					*/
				}
			}

			// confirm by checking if we actually hit something and what it was
			if (bestLocWS.is_set())
			{
				RayCastCheck::perform(_params, imo, preStartLoc, startLoc, bestLocWS.get() + (bestLocWS.get() - startLoc).normal() * 0.1f, OUT_ missEndLoc, OUT_ hitSomething, OUT_ result, OUT_ relativePlacement);
			}
		}

		// straight line again (if didn't hit anything else)
		if (!hitSomething && 
			! _params.rayCastSearchForObjects && _params.fallbackRayCastSearchForObjects) // no need to do it twice
		{
			RayCastCheck::perform(_params, imo, preStartLoc, startLoc, endLoc, OUT_ missEndLoc, OUT_ hitSomething, OUT_ result, OUT_ relativePlacement);
		}

		if (hitSomething)
		{
			auto* enemy = fast_cast<::Framework::IModulesOwner>(result.object);

			endLoc = result.hitLocation;

			Energy damageDealt = Energy::zero();

			if (enemy)
			{
				// check all as we could use ray, not cone search
				auto* check = enemy;

				Tags lightningProjectileTypeTags = Library::get_current()->get_global_references().get_tags(NAME(grLightningProjectileTypeTags));

				while (check)
				{
					if (auto* health = check->get_custom<CustomModules::Health>())
					{
						Damage dealDamage;

						dealDamage.damage = Energy(1);
						dealDamage.cost = Energy(1);

						dealDamage.damageType = DamageType::Lightning;
						dealDamage.armourPiercing = EnergyCoef::one();

						auto* ep = check->get_presence();
						auto* presence = imo->get_presence();

						DamageInfo damageInfo;
						damageInfo.damager = imo;
						damageInfo.source = imo;
						damageInfo.instigator = instigator;

						if (_params.setup_damage)
						{
							_params.setup_damage(dealDamage, damageInfo);
						}

						damageDealt = dealDamage.damage;

						ContinuousDamage dealContinuousDamage;
						dealContinuousDamage.setup_using_weapon_core_modifier_companion_for(dealDamage);

						health->adjust_damage_on_hit_with_extra_effects(REF_ dealDamage);

						if (PhysicalMaterial::ForProjectile const* useForProjectile = PhysicalMaterial::get_for_projectile(result.material, dealDamage, lightningProjectileTypeTags))
						{
							dealDamage.apply(useForProjectile, nullptr);
						}
						if (!dealDamage.damage.is_zero())
						{
							{
								if (ep->get_in_room() == presence->get_in_room())
								{
									damageInfo.hitRelativeLocation = ep->get_placement().location_to_local(presence->get_centre_of_presence_WS());
								}
								else
								{
									Framework::RelativeToPresencePlacement rpp;
									rpp.be_temporary_snapshot();
									if (rpp.find_path(cast_to_nonconst(check), presence->get_in_room(), presence->get_centre_of_presence_transform_WS()))
									{
										damageInfo.hitRelativeLocation = ep->get_placement().location_to_local(rpp.get_placement_in_owner_room().get_translation());
									}
								}
							}
							health->deal_damage(dealDamage, dealDamage, damageInfo);
							if (dealContinuousDamage.is_valid())
							{
								health->add_continuous_damage(dealContinuousDamage, damageInfo);
							}
							discharged = true;
						}
						break;
					}
					if (auto* p = check->get_presence())
					{
						check = p->get_attached_to();
					}
				}
			}

			if (discharged)
			{
				int lightningCount = 1;
				if (_params.lightningCount.is_set())
				{
					lightningCount = _params.lightningCount.get();
				}
				else
				{
					// we want to have varied number of lightnings, depending on the actual damage
					// damage -> *2 -> sqrt -> into cells -> clamped
					//  1.0 ->  1 -> 1
					//  2.0 ->  4 -> 2
					//  4.5 ->  9 -> 3
					//  8.0 -> 16 -> 4
					// 12.5 -> 25 -> 5
					// 18.0 -> 36 -> 6
					// 24.5 -> 49 -> 7
					// 32.0 -> 64 -> 8
					float normalised = (damageDealt * 2).as_float();
					float unexp = sqrt(max(0.0f, normalised));
					int rawCount = TypeConversions::Normal::f_i_cells(unexp);
					lightningCount = clamp(rawCount, 1, hardcoded 8);
				}
				lightningSpawner->single(_params.hitID.get(NAME(discharge)), Framework::CustomModules::LightningSpawner::LightningParams()
					.with_count(lightningCount)
					.with_target(relativePlacement, true)
					.with_start_placement_os(startPlacementOS));
				if (auto* s = imo->get_sound())
				{
					s->play_sound(_params.hitID.get(NAME(discharge)));
				}

#ifndef NO_TEMPORARY_OBJECTS_ON_PROJECTILE_HIT
				if (relativePlacement.is_active())
				{
					// play temporary object - lightning hit
					if (auto* tos = imo->get_temporary_objects())
					{
						auto* lightningHit = tos->spawn(_params.lightningHitTOID.get(NAME(lightningHit)));
						if (lightningHit)
						{
							Transform placementWS = matrix_from_up(relativePlacement.location_from_owner_to_target(result.hitLocation), relativePlacement.location_from_owner_to_target(result.hitNormal)).to_transform();
							bool placeInWorld = true;
							if (auto* at = result.presenceLink ? result.presenceLink->get_modules_owner() : nullptr)
							{
								placeInWorld = false;
								Transform placementRelativeToWS = result.presenceLink->get_placement_in_room();
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
									Transform boneWS = placementRelativeToWS.to_world(boneOS);
									placementRelativeToWS = boneWS;
									Transform relativePlacement = placementRelativeToWS.to_local(placementWS);
									lightningHit->on_activate_attach_to_bone(at, bone, false, relativePlacement);
								}
								else
								{
									Transform relativePlacement = placementRelativeToWS.to_local(placementWS);
									lightningHit->on_activate_attach_to(at, false, relativePlacement);
								}
							}
							if (placeInWorld)
							{
								lightningHit->on_activate_place_in(cast_to_nonconst(relativePlacement.get_in_final_room()), placementWS);
							}
						}
					}
				}
				else
				{
					error(TXT("no relative placement available, should be always provided on hit"));
				}
#endif
			}
		}
		if (!discharged)
		{
			Transform missPlacement = startPlacement;
			missPlacement.set_translation(missEndLoc);
			relativePlacement.find_path(imo, imo->get_presence()->get_in_room(), missPlacement);
			lightningSpawner->single(_params.missID.get(NAME(dischargeMiss)), Framework::CustomModules::LightningSpawner::LightningParams()
				.with_target(relativePlacement, true)
				.with_start_placement_os(startPlacementOS)
			);
		}
	}

	return discharged;
}

