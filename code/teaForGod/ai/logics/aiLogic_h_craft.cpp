#include "aiLogic_h_craft.h"

#include "..\..\game\playerSetup.h"
#include "..\..\modules\moduleAI.h"
#include "..\..\modules\moduleMovementH_Craft.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\health\mc_health.h"
#include "..\..\sound\subtitleSystem.h"

#include "..\..\..\framework\framework.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\text\stringFormatter.h"
#include "..\..\..\framework\world\room.h"

#include "..\..\..\core\debug\debugRenderer.h"

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
DEFINE_STATIC_NAME_STR(aim_die, TXT("die"));
DEFINE_STATIC_NAME_STR(aim_devTeleportEverywhere, TXT("h_craft; dev teleport everywhere"));
DEFINE_STATIC_NAME_STR(aim_goTo, TXT("h_craft; go to"));
DEFINE_STATIC_NAME_STR(aim_rotation, TXT("h_craft; rotation"));
DEFINE_STATIC_NAME_STR(aim_follow, TXT("h_craft; follow"));
DEFINE_STATIC_NAME_STR(aim_followPOIs, TXT("h_craft; follow POIs"));

// ai message params
DEFINE_STATIC_NAME(devTeleport);
DEFINE_STATIC_NAME(relativeToEnvironment);
DEFINE_STATIC_NAME(keepActiveRequest);
DEFINE_STATIC_NAME(noLocationGoForward);
DEFINE_STATIC_NAME(targetYaw);
DEFINE_STATIC_NAME(targetPitch);
DEFINE_STATIC_NAME(targetLoc);
DEFINE_STATIC_NAME(targetSocketOffset);
DEFINE_STATIC_NAME(targetSocketOffsetYaw);
DEFINE_STATIC_NAME(offsetYaw);
DEFINE_STATIC_NAME(offsetLocOS);
DEFINE_STATIC_NAME(forcedYawSpeed);
DEFINE_STATIC_NAME(yawSpeed);
DEFINE_STATIC_NAME(yawAcceleration);
DEFINE_STATIC_NAME(pitchAcceleration);
DEFINE_STATIC_NAME(beSilent);
DEFINE_STATIC_NAME(restartFlyingEngineSounds);
DEFINE_STATIC_NAME(instantFacing);
DEFINE_STATIC_NAME(instantFollowing);
DEFINE_STATIC_NAME(ignoreYawMatchForTrigger);
DEFINE_STATIC_NAME(instantManeuverSpeed);
DEFINE_STATIC_NAME(instantTransitSpeed);
DEFINE_STATIC_NAME(maintainManeuverSpeed);
DEFINE_STATIC_NAME(requestedManeuverSpeed);
DEFINE_STATIC_NAME(requestedManeuverSpeedPt);
DEFINE_STATIC_NAME(requestedManeuverAccelPt);
DEFINE_STATIC_NAME(maintainTransitSpeed);
DEFINE_STATIC_NAME(requestedTransitSpeed);
DEFINE_STATIC_NAME(requestedTransitSpeedPt);
DEFINE_STATIC_NAME(requestedTransitAccelPt);
DEFINE_STATIC_NAME(transitThrough);
DEFINE_STATIC_NAME(maneuverThrough);
DEFINE_STATIC_NAME(triggerGameScriptTrap);
DEFINE_STATIC_NAME(triggerOwnGameScriptTrap);
DEFINE_STATIC_NAME(triggerBelowDistance);
DEFINE_STATIC_NAME(matchSocket);
DEFINE_STATIC_NAME(matchSockets);
DEFINE_STATIC_NAME(matchToIMOSocket);
DEFINE_STATIC_NAME(matchToIMO);
DEFINE_STATIC_NAME(toPOI);
DEFINE_STATIC_NAME(followIMO);
DEFINE_STATIC_NAME(followIMOInTransit);
DEFINE_STATIC_NAME(clearFollowIMOInTransit);
DEFINE_STATIC_NAME(followIMOIgnoreForward);
DEFINE_STATIC_NAME(followPOIs);
DEFINE_STATIC_NAME(limitFollowRelVelocity);
DEFINE_STATIC_NAME(followRelVelocity);
DEFINE_STATIC_NAME(keepFollowOffsetAdditional);
DEFINE_STATIC_NAME(followOffset);
DEFINE_STATIC_NAME(followOffsetFromCurrent);
DEFINE_STATIC_NAME(followOffsetAdditionalRange);
DEFINE_STATIC_NAME(followOffsetAdditionalInterval);
DEFINE_STATIC_NAME(blockExtraRotation);

// layout elements (string formatter
DEFINE_STATIC_NAME(s);// speed
DEFINE_STATIC_NAME(h);// heading
DEFINE_STATIC_NAME(a);// altitude
DEFINE_STATIC_NAME(b);// battery
DEFINE_STATIC_NAME(p);// power
DEFINE_STATIC_NAME(i);// id

// variables
DEFINE_STATIC_NAME(upgradeCanisterId);
DEFINE_STATIC_NAME(craftOff);

// emissive layers
DEFINE_STATIC_NAME(eyes);
DEFINE_STATIC_NAME(lights);
DEFINE_STATIC_NAME(engine);

// room tags
DEFINE_STATIC_NAME(openWorldExterior);

// POIs
DEFINE_STATIC_NAME_STR(environmentAnchor, TXT("environment anchor"));

// localised strings
DEFINE_STATIC_NAME_STR(lsVignetteForMovementOn, TXT("pi.ov.in; vignette for movement; on"));
DEFINE_STATIC_NAME_STR(lsVignetteForMovementOff, TXT("pi.ov.in; vignette for movement; off"));

// colours
DEFINE_STATIC_NAME_STR(rcPilgrimageOverlaySystem, TXT("pilgrim_overlay_system"));

//

REGISTER_FOR_FAST_CAST(H_Craft);

H_Craft::H_Craft(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	h_craftData = fast_cast<H_CraftData>(_logicData);

	if (h_craftData)
	{
		setup = h_craftData->setup;
	}

	if (auto* imo = _mind->get_owner_as_modules_owner())
	{
		rg = imo->get_individual_random_generator();
		rg.advance_seed(234805, 9172);
	}
}

H_Craft::~H_Craft()
{
	stop_observing_presence();
}

Transform H_Craft::get_placement() const
{
	Rotator3 r = rotation.get(Rotator3::zero);
	r.roll = rotationOffsetFromRotation.roll + rotationOffsetFromLinear.roll;
	r.pitch = rotationOffsetFromLinear.pitch;
	return Transform(location.get(Vector3::zero), r.to_quat());
}

void H_Craft::on_presence_changed_room(Framework::ModulePresence* _presence, Framework::Room* _intoRoom, Transform const& _intoRoomTransform, Framework::DoorInRoom* _exitThrough, Framework::DoorInRoom* _enterThrough, Framework::DoorInRoomArrayPtr const* _throughDoors)
{
	// clear, we will read new
	location.clear();
	rotation.clear();
	velocityLinear = _intoRoomTransform.vector_to_local(velocityLinear);
	// velocityRotation is relative;

	/*
	if (auto* imo = _presence->get_owner())
	{
		if (auto* m = fast_cast<ModuleMovementH_Craft>(imo->get_movement()))
		{
			m->blend_in(0.5f);
		}
	}
	*/
}

void H_Craft::on_presence_destroy(Framework::ModulePresence* _presence)
{
	an_assert(observingPresence == _presence);
	stop_observing_presence();
}

void H_Craft::observe_presence(Framework::ModulePresence* _presence)
{
	if (_presence != observingPresence)
	{
		stop_observing_presence();
		if (_presence)
		{
			observingPresence = _presence;
			observingPresence->add_presence_observer(this);
		}
	}
}

void H_Craft::stop_observing_presence()
{
	if (observingPresence)
	{
		observingPresence->remove_presence_observer(this);
		observingPresence = nullptr;
	}
}

void H_Craft::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	extraRotationBlocked = blend_to_using_speed_based_on_time(extraRotationBlocked, blockExtraRotation ? 1.0f : 0.0f, 1.0f, _deltaTime);

	if (useCanister)
	{
		canister.set_active(display && display->is_on());
		canister.advance(_deltaTime);

		if (display &&
			display->is_on() &&
			canister.should_give_content())
		{
			if (canister.give_content(nullptr, false, true))
			{
				auto & psp = PlayerSetup::access_current().access_preferences();
				psp.useVignetteForMovement = !psp.useVignetteForMovement;

				if (auto* ss = SubtitleSystem::get())
				{
					Name showSubtitle = psp.useVignetteForMovement ? NAME(lsVignetteForMovementOn) : NAME(lsVignetteForMovementOff);
					Name otherSubtitle = psp.useVignetteForMovement ? NAME(lsVignetteForMovementOff) : NAME(lsVignetteForMovementOn);
					{
						SubtitleId sid = ss->get(otherSubtitle);
						if (sid != NONE)
						{
							ss->remove(sid);
						}
					}
					{
						SubtitleId sid = ss->get(showSubtitle);
						if (sid == NONE)
						{
							sid = ss->add(showSubtitle, NP, true, RegisteredColours::get_colour(NAME(rcPilgrimageOverlaySystem)), Colour::black.with_alpha(0.25f));
						}
						ss->remove(sid, 2.0f);
					}
				}

			}
		}
	}

	Framework::IModulesOwner* imo = nullptr;
	if (auto* mind = get_mind())
	{
		imo = mind->get_owner_as_modules_owner();
	}

	if (imo)
	{
		observe_presence(imo->get_presence());
		bool prevCraftOff = craftOff;
		craftOff = imo->get_value<bool>(NAME(craftOff), craftOff) || dying;
		if (prevCraftOff != craftOff)
		{
			if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
			{
				if (craftOff)
				{
					if (dying)
					{
						em->emissive_deactivate_all(0.3f);
					}
					else
					{
						em->emissive_deactivate_all();
					}
				}
				else
				{
					em->emissive_activate(NAME(eyes));
					em->emissive_activate(NAME(lights));
					em->emissive_activate(NAME(engine));
				}
			}
		}
	}
	else
	{
		stop_observing_presence();
	}

	bool blendingIntoProcessAsUsual = false;

	if (imo)
	{
		if (auto* m = fast_cast<ModuleMovementH_Craft>(imo->get_movement()))
		{
			blendingIntoProcessAsUsual = m->is_blending_in();
		}
		if (auto* p = imo->get_presence())
		{
			auto* inRoom = imo->get_presence()->get_in_room();
			if (!location.is_set() || p->has_teleported() || blendingIntoProcessAsUsual)
			{
				location = p->get_placement().get_translation();
			}
			if (!rotation.is_set() || p->has_teleported() || blendingIntoProcessAsUsual)
			{
				rotation = p->get_placement().get_orientation().to_rotator();
			}
			if (p->does_request_teleport())
			{
				// wait until teleported
				return;
			}
			if (inRoom && location.is_set())
			{
				location = location.get() - _deltaTime * inRoom->get_room_velocity();
			}
		}

#ifdef AN_DEVELOPMENT_OR_PROFILER	
		if (::Framework::is_preview_game())
		{
			static float changeTarget = 0.0f;
			changeTarget -= _deltaTime;
			if (changeTarget < 0.0f)
			{
				changeTarget = 2.5f;
				if (!matchYaw.is_set() || !matchLocation.is_set() ||
					((matchLocation.get() - location.get()).length() < 0.01f &&
						abs(Rotator3::normalise_axis(matchYaw.get() - rotation.get().yaw)) < 0.01f))
				{
					matchYaw = rg.get_float(0.0f, 360.0f);
					float xy = 300.0f;
					float z = 30.0f;
					matchLocation = Vector3(rg.get_float(-xy, xy), rg.get_float(-xy, xy), 0.0f).normal() * (rg.get_float(0.5f, 1.0f) * xy);
					matchLocation.access().z = rg.get_float(5.0f, z);
				}
			}
		}
#endif

		// get targets
		Optional<float> targetYaw;
		Optional<float> targetPitch;
		Optional<Vector3> targetLocation;
		bool forceFollowInTransit = false;
		Optional<Vector3> followInTransitVelocity;
		Optional<Vector3> followInTransitOffsetOS;
		if (! craftOff && ! dying)
		{
			if (matchYaw.is_set())
			{
				targetYaw = matchYaw;
			}
			if (matchPitch.is_set())
			{
				targetPitch = matchPitch;
			}
			if (matchLocation.is_set())
			{
				targetLocation = matchLocation;
			}

			if (followIMO.get())
			{
				Transform followPlacement = followIMO->get_presence()->get_placement();
				if (followIMOIgnoreForward)
				{
					Vector3 fwdWS = (followPlacement.get_translation() - location.get(imo->get_presence()->get_placement().get_translation())).normal();
					Vector3 upWS = Vector3::zAxis;
					followPlacement = look_matrix(followPlacement.get_translation(), fwdWS.drop_using(upWS).normal(), upWS).to_transform();
				}
				if (followOffset.is_set())
				{
					followOffsetAdditionalIntervalLeft -= _deltaTime;
					if (followOffsetAdditionalIntervalLeft < 0.0f)
					{
						if (followOffsetAdditionalRange.is_set() &&
							followOffsetAdditionalInterval.is_set())
						{
							followOffsetAdditional = Vector3(
								rg.get(followOffsetAdditionalRange.get().x),
								rg.get(followOffsetAdditionalRange.get().y),
								rg.get(followOffsetAdditionalRange.get().z));
							followOffsetAdditionalIntervalWhole = rg.get(followOffsetAdditionalInterval.get());
							followOffsetAdditionalIntervalLeft = followOffsetAdditionalIntervalWhole;
						}
						else
						{
							followOffsetAdditional.clear();
							followOffsetAdditionalIntervalLeft = 0.0f;
							followOffsetAdditionalIntervalWhole = 0.0f;
						}
					}

					Vector3 useOffset = followOffset.get();
					if (followOffsetAdditionalIntervalWhole > 0.0f)
					{
						float atT = clamp(1.0f - followOffsetAdditionalIntervalLeft / followOffsetAdditionalIntervalWhole, 0.0f, 1.0f);
						atT = BlendCurve::cubic(atT);
						atT = BlendCurve::cubic(atT);
						useOffset += followOffsetAdditional.get(Vector3::zero) * atT;
					}
					targetLocation = followPlacement.location_to_world(useOffset);
				}
				else
				{
					targetLocation = followPlacement.get_translation();// followIMO->get_presence()->get_centre_of_presence_WS();
				}
				followInTransitVelocity = followIMO->get_presence()->get_velocity_linear();
				if (auto* r = followIMO->get_presence()->get_in_room())
				{
					followInTransitVelocity = followInTransitVelocity.get() + r->get_room_velocity();
				}
				if (followRelVelocity.is_set() &&
					!followRelVelocity.get().is_zero() &&
					location.is_set() &&
					!instantFollowing) // ignore for instantFollowing
				{
					an_assert(targetLocation.is_set());
					Vector3 inDir = followRelVelocity.get().normal();
					Vector3 toTarget = targetLocation.get() - location.get();
					float inDirToTarget = Vector3::dot(inDir, toTarget.normal());
					if (inDirToTarget < 0.0f)
					{
						targetLocation = location.get() + followRelVelocity.get();
					}
					else
					{
						targetLocation = targetLocation.get() + followRelVelocity.get();
					}
					followInTransitVelocity = followInTransitVelocity.get() + followRelVelocity.get(Vector3::zero);
				}
				forceFollowInTransit = followIMOInTransit.get(followInTransitVelocity.get().length() > 3.0f);
				if (followInTransitVelocity.get().is_almost_zero() || ! forceFollowInTransit)
				{
					targetYaw = followPlacement.get_axis(Axis::Forward).to_rotator().yaw;
					forceFollowInTransit = false;
				}
				else
				{
					targetYaw = followInTransitVelocity.get().to_rotator().yaw;
				}
			}

			if (matchToIMO.is_set())
			{
				Transform mPlacement = matchToIMO->get_presence()->get_placement();
				if (matchToIMOSocket.is_valid())
				{
					Transform sPlacementOS = matchToIMO->get_appearance()->calculate_socket_os(matchToIMOSocket);
					sPlacementOS.set_translation(sPlacementOS.get_translation() + sPlacementOS.vector_to_world(targetSocketOffset.get(Vector3::zero)));
					mPlacement = mPlacement.to_world(sPlacementOS);
				}
				if (!targetYaw.is_set())
				{
					targetYaw = mPlacement.get_axis(Axis::Forward).to_rotator().yaw + targetSocketOffsetYaw.get(0.0f);
				}
				if (!targetLocation.is_set())
				{
					targetLocation = mPlacement.get_translation();
				}
			}

			if (matchSocket.is_valid())
			{
				Transform sPlacementOS = imo->get_appearance()->calculate_socket_os(matchSocket);
				float offsetYaw = sPlacementOS.get_axis(Axis::Forward).to_rotator().yaw;
				if (targetYaw.is_set())
				{
					targetYaw = targetYaw.get() - offsetYaw;
				}
				if (targetLocation.is_set())
				{
					float atYaw = targetYaw.get(rotation.get().yaw);
					Rotator3 atRotation(0.0f, atYaw, 0.0f);
					Vector3 socketOffsetWS = atRotation.to_quat().to_world(sPlacementOS.get_translation());
					targetLocation = targetLocation.get() - socketOffsetWS;
				}
			}

			if (followPOIs.is_set())
			{
				Vector3 pAt = imo->get_presence()->get_placement().get_translation();
				float relativeDistance = followPOIsRelativeDistance.get(2.0f);
				float maxDist = max(-relativeDistance, relativeDistance) + 5.0f;

				ArrayStatic<FollowPOIPath::FoundPOI, 32> foundPOIs;
				imo->get_presence()->get_in_room()->for_every_point_of_interest(followPOIs.get(),
					[maxDist, pAt, &foundPOIs](Framework::PointOfInterestInstance* _fpoi)
					{
						Transform poiPlacement = _fpoi->calculate_placement();
						float dist = (poiPlacement.get_translation() - pAt).length();
						if (dist < maxDist)
						{
							an_assert(foundPOIs.has_place_left());
							if (foundPOIs.has_place_left())
							{
								FollowPOIPath::FoundPOI fp;
								fp.placement = poiPlacement;
								fp.relativePlacement = -poiPlacement.location_to_local(pAt).y;
								foundPOIs.push_back(fp);
							}
						}
					});
				if (!foundPOIs.is_empty())
				{
					sort(foundPOIs);

					{
						{
							Vector3 towardsWS = FollowPOIPath::process(foundPOIs.get_data(), foundPOIs.get_size(), relativeDistance);
#ifdef DEBUG_DRAW_PRESENCE_FORCE_DISPLACEMENT_VR_ANCHOR
							debug_draw_arrow(true, Colour::magenta, pAt, towardsWS);
#endif
							targetLocation = towardsWS;
						}
						Rotator3 resultDirection = (targetLocation.get() - pAt).to_rotator();
						targetYaw = resultDirection.yaw;
						targetPitch = resultDirection.pitch;
					}
				}
			}
		}

#ifdef AN_DEVELOPMENT_OR_PROFILER
		if (devTeleport || devTeleportEverywhere)
		{
			instantFacing = false;
			instantManeuverSpeed.clear();
			instantTransitSpeed.clear();
			instantFollowing = false;
			inTransit = false;
			transitThrough = false;
			maneuverThrough = false;
			velocityLinear = Vector3::zero;
			velocityRotation = Rotator3::zero;
			location = targetLocation.get(location.get());
			rotation.access().yaw = targetYaw.get(rotation.get().yaw);
		}
#endif

		if (craftOff)
		{
			inTransit = false;
		}
		else
		{
			if (instantTransitSpeed.is_set())
			{
				// force into transit before we calculate actual target yaw
				inTransit = true;
			}
		}

		Rotator3 prevVelocityRotation = velocityRotation;
		Vector3 prevVelocityLinear = velocityLinear;

		bool atYawTarget = true;
		bool atLocTarget = true;

		bool toBeInTransit = false;
		{
			if (!targetLocation.is_set() && targetYaw.is_set() && noLocationGoForward)
			{
				if (!inTransit)
				{
					float yawToTarget = targetYaw.get();
					float yawDiff = Rotator3::normalise_axis(yawToTarget - rotation.get().yaw);
					toBeInTransit = true;
					if (abs(yawDiff) < 3.0f || forceFollowInTransit)
					{
						inTransit = true;
					}
				}
			}
			else
			{
				Vector3 toTarget = (targetLocation.get(location.get()) - location.get());
				float distance2d = toTarget.length_2d();
				if (inTransit)
				{
					if (!forceFollowInTransit &&
						!transitThrough &&
						distance2d < setup.changeToManeuverDistance &&
						velocityLinear.length() <= setup.maneuver.linSpeed)
					{
						inTransit = false;
					}
				}
				else
				{
					float yawToTarget = toTarget.to_rotator().yaw;
					float yawDiff = Rotator3::normalise_axis(yawToTarget - rotation.get().yaw);
					if (distance2d > setup.changeToTransitDistance || forceFollowInTransit)
					{
						toBeInTransit = true;
						if (abs(yawDiff) < 3.0f || forceFollowInTransit)
						{
							inTransit = true;
						}
					}
				}
			}
		}

		// current mode
		H_CraftMovementParams const& use = inTransit ? setup.transit : setup.maneuver;
		float yawAccel = yawAcceleration.get(use.yawAcceleration);
		float pitchAccel = pitchAcceleration.get(0.0f);
		float useYawSpeed = yawSpeed.get(use.yawSpeed);
		float maxYawSpeed = useYawSpeed;

		float linAccel = use.linAcceleration;
		float maxLinSpeed = use.linSpeed;

		if (inTransit)
		{
			maxLinSpeed *= requestedTransitSpeedPt.get(1.0f);
			maxLinSpeed = requestedTransitSpeed.get(maxLinSpeed);
			linAccel *= requestedTransitAccelPt.get(1.0f);
		}
		else
		{
			maxLinSpeed *= requestedManeuverSpeedPt.get(1.0f);
			maxLinSpeed = requestedManeuverSpeed.get(maxLinSpeed);
			linAccel *= requestedManeuverAccelPt.get(1.0f);
		}

#ifdef AN_DEBUG_RENDERER
		Optional<Vector3> orgTargetLocation = targetLocation;
#endif

		if (inTransit || toBeInTransit)
		{
			inTransitOffsetTimeLeft -= _deltaTime;
			if (inTransitOffsetTimeLeft <= 0.0f)
			{
				inTransitOffsetTimeLeft = rg.get(setup.inTransitOffsetPeriod);
				inTransitOffset.yaw = rg.get_float(-setup.inTransitOffset.yaw, setup.inTransitOffset.yaw);
				inTransitOffset.pitch = rg.get_float(-setup.inTransitOffset.pitch, setup.inTransitOffset.pitch);
			}

			if (targetLocation.is_set())
			{
				Vector3 toTarget = (targetLocation.get() - location.get());
				Rotator3 towardsTargetLocation = toTarget.to_rotator();

				if (forceFollowInTransit)
				{
					an_assert(followInTransitVelocity.is_set());
					targetYaw = followInTransitVelocity.get().to_rotator().yaw;

					if (rotation.is_set() && location.is_set() &&
						!instantFollowing) // ignore for following
					{
						Vector3 relativeTargetLocation = get_placement().location_to_local(targetLocation.get());
						an_assert(followIMO.get());
						Transform fimoPlacement = followIMO->get_presence()->get_placement();
						Vector3 upWS = fimoPlacement.get_axis(Axis::Up);
						Transform targetPlacement = look_matrix(targetLocation.get(), followInTransitVelocity.get().drop_using(upWS).normal(), upWS).to_transform();
						Vector3 relativeToTarget = targetPlacement.location_to_local(location.get());
						if (relativeTargetLocation.length_2d() < setup.changeToManeuverDistance ||
							(abs(relativeToTarget.x) < setup.changeToManeuverDistance && relativeToTarget.y > 0.0f))
						{
							// try to match direction and speed
							targetYaw = targetYaw.get() + clamp(relativeTargetLocation.x * 3.0f, -10.0f, 10.0f);
							followInTransitOffsetOS = relativeTargetLocation;
						}
						else
						{
							// fly towards the target
							targetYaw = towardsTargetLocation.yaw;
							followInTransitOffsetOS = relativeTargetLocation;
						}
					}
				}
				else
				{
					if (inTransit)
					{
						towardsTargetLocation += inTransitOffset * (1.0f - extraRotationBlocked);
					}

					targetYaw = towardsTargetLocation.yaw;

					targetLocation = location.get() + towardsTargetLocation.get_forward() * toTarget.length();
				}
			}
		}

		if (instantFacing)
		{
			instantFacing = false;
			if (targetYaw.is_set())
			{
				rotation.access() = Rotator3(0.0f, targetYaw.get(), 0.0f);
			}
			velocityRotation = Rotator3::zero;
			prevVelocityRotation = velocityRotation;
			rotationOffsetFromRotation = Rotator3::zero;
		}

		if (instantManeuverSpeed.is_set())
		{
			restartFlyingEngineSounds = true;
			inTransit = false;
			instantManeuverSpeed.clear();
			velocityLinear = rotation.get().get_forward() * setup.maneuver.linSpeed * instantManeuverSpeed.get(1.0f) * requestedManeuverSpeedPt.get(1.0f);
			prevVelocityLinear = velocityLinear;
			rotationOffsetFromLinear = Rotator3::zero;
		}

		if (instantTransitSpeed.is_set())
		{
			restartFlyingEngineSounds = true;
			inTransit = true;
			instantTransitSpeed.clear();
			velocityLinear = rotation.get().get_forward() * setup.transit.linSpeed * instantTransitSpeed.get(1.0f) * requestedTransitSpeedPt.get(1.0f);
			prevVelocityLinear = velocityLinear;
			rotationOffsetFromLinear = Rotator3::zero;
		}

		if (instantFollowing && followInTransitVelocity.is_set())
		{
			restartFlyingEngineSounds = true;
			instantFollowing = false;
			if (targetLocation.is_set())
			{
				location = targetLocation.get();
			}
			velocityLinear = followInTransitVelocity.get(Vector3::zero);
			prevVelocityLinear = velocityLinear;
			rotationOffsetFromLinear = Rotator3::zero;
			if (targetYaw.is_set())
			{
				rotation = Rotator3(0.0f, targetYaw.get(), 0.0f);
			}
			velocityRotation = Rotator3::zero;
			prevVelocityRotation = velocityRotation;
			rotationOffsetFromRotation = Rotator3::zero;
		}

		if (dying)
		{
			if (dyingTimeLeft >= 0.0f)
			{
				dyingTimeLeft -= _deltaTime;
				if (dyingTimeLeft < 0.0f)
				{
					if (auto* h = imo->get_custom<CustomModules::Health>())
					{
						h->perform_death();
					}
				}
			}
			dyingTime += _deltaTime;
			float affectLinear = dyingFullAffectVelocityLinearTime != 0.0f ? clamp(dyingTime / dyingFullAffectVelocityLinearTime, 0.0f, 1.0f) : 1.0f;
			float affectRotation = dyingFullAffectVelocityRotationTime != 0.0f ? clamp(dyingTime / dyingFullAffectVelocityRotationTime, 0.0f, 1.0f) : 1.0f;
			velocityLinear = velocityLinear.normal() * max(0.0f, velocityLinear.length() + affectLinear * dyingAcceleration * _deltaTime);
			velocityRotation += affectRotation * dyingRotation * _deltaTime;
		}
		else
		{
			// meet target
			if (forcedYawSpeed.is_set())
			{
				float tgt = forcedYawSpeed.get();
				if (velocityRotation.yaw > tgt)
				{
					velocityRotation.yaw = max(tgt, velocityRotation.yaw - yawAccel * _deltaTime);
				}
				else
				{
					velocityRotation.yaw = min(tgt, velocityRotation.yaw + yawAccel * _deltaTime);
				}
			}
			else if (targetYaw.is_set())
			{
				float yawOff = Rotator3::normalise_axis(targetYaw.get() - rotation.get().yaw);

				if (abs(yawOff) > 3.0f || abs(velocityRotation.yaw) > 1.0f)
				{
					float time0ToStop = abs(velocityRotation.yaw) / yawAccel;
					float time1ToStop = time0ToStop + 0.5f; // should start slowing down

					float acc0 = yawAccel;
					float acc1 = abs(velocityRotation.yaw) / time1ToStop;
				
					float yawNeeded = abs(yawOff);

					float yawCovered0 = velocityRotation.yaw * sign(yawOff) * time0ToStop - acc0 * sqr(time0ToStop) / 2.0f;
					float yawCovered1 = velocityRotation.yaw * sign(yawOff) * time1ToStop - acc1 * sqr(time1ToStop) / 2.0f;
				
					float accToStop = 0.0f;
					if (yawCovered0 >= yawNeeded) accToStop = acc0; else
					if (yawCovered1 >= yawNeeded) accToStop = acc1;

					if (accToStop > 0.0f)
					{
						// slow down to stop
						velocityRotation.yaw -= sign(velocityRotation.yaw) * accToStop * _deltaTime;
					}
					else
					{
						if (abs(velocityRotation.yaw) > maxYawSpeed)
						{
							// slow down
							velocityRotation.yaw -= sign(velocityRotation.yaw) * yawAccel * _deltaTime;
							if (abs(velocityRotation.yaw) < maxYawSpeed)
							{
								// but maintain
								velocityRotation.yaw = sign(velocityRotation.yaw) * maxYawSpeed;
							}
						}
						else
						{
							// speed up
							velocityRotation.yaw += sign(yawOff) * yawAccel * clamp(abs(yawOff) / maxYawSpeed, 0.05f, 1.0f) * _deltaTime;
							velocityRotation.yaw = clamp(velocityRotation.yaw, -maxYawSpeed, maxYawSpeed);
						}
					}
				}
				else
				{
					velocityRotation.yaw = blend_to_using_time(velocityRotation.yaw, yawOff * 0.8f, 0.05f, _deltaTime);
					//velocityRotation.yaw = yawOff;
				}

				atYawTarget = abs(yawOff) < (inTransit ? 90.0f : 0.1f);
			}
			else
			{
				if (velocityRotation.yaw > 0.0f)
				{
					velocityRotation.yaw = max(0.0f, velocityRotation.yaw - yawAccel * _deltaTime);
				}
				else
				{
					velocityRotation.yaw = min(0.0f, velocityRotation.yaw + yawAccel * _deltaTime);
				}
			}
			
			if (targetLocation.is_set())
			{
				Vector3 locOff = targetLocation.get() - location.get();

				atLocTarget = locOff.length() < triggerBelowDistance.get(0.005f);

				if (followInTransitVelocity.is_set() && followInTransitOffsetOS.is_set())
				{
					Vector3 targetVelocity = followInTransitVelocity.get();
					{
						// we shouldn't slow down too much
						Vector3 followInTransitOffsetWS = get_placement().vector_to_world(followInTransitOffsetOS.get());
						Vector3 off = followInTransitOffsetWS * clamp(followInTransitOffsetWS.length() / 10.0f, 0.5f, 2.0f);
						Vector3 offAlong = off.along(targetVelocity);
						Vector3 offPerp = off - offAlong;
						if (Vector3::dot(offAlong, targetVelocity) < 0.0f)
						{
							offAlong = offAlong.normal() * min(offAlong.length(), targetVelocity.length() * 0.5f);
						}
						targetVelocity += offPerp + offAlong;
					}

					if (limitFollowRelVelocity.is_set())
					{
						Vector3 velDiff = targetVelocity - followInTransitVelocity.get();
						if (velDiff.length() > limitFollowRelVelocity.get())
						{
							targetVelocity = followInTransitVelocity.get() + velDiff.normal() * limitFollowRelVelocity.get();
						}
					}

					if (targetVelocity.length() > maxLinSpeed)
					{
						targetVelocity = targetVelocity.normal() * maxLinSpeed;
					}

					Vector3 changeRequired = (targetVelocity - velocityLinear);

					Vector3 changeVelocity = changeRequired.normal() * min(linAccel * _deltaTime, changeRequired.length());

#ifdef AN_DEBUG_RENDERER
					debug_filter(ai_draw);
					debug_context(imo->get_presence()->get_in_room());
					{
						Vector3 at = imo->get_presence()->get_placement().get_translation();
						if (targetLocation.is_set())
						{
							debug_draw_arrow(true, Colour::c64Blue, at, targetLocation.get());
						}
						debug_draw_arrow(true, Colour::purple, at, get_placement().location_to_world(followInTransitOffsetOS.get()));
						debug_draw_arrow(true, Colour::mint, at, at + velocityLinear);
						debug_draw_arrow(true, Colour::white, at, at + targetVelocity);
						debug_draw_arrow(true, Colour::c64Brown, at + velocityLinear, at + velocityLinear + changeVelocity);
						debug_draw_arrow(true, Colour::c64LightGreen, at + velocityLinear + changeVelocity, at + velocityLinear + changeRequired);
						if (rotation.is_set())
						{
							debug_draw_arrow(true, Colour::yellow, at, at + rotation.get().get_forward() * 10.0f);
						}
						if (targetYaw.is_set())
						{
							debug_draw_arrow(true, Colour::red, at, at + Rotator3(0.0f, targetYaw.get(), 0.0f).get_forward() * 10.0f);
						}
						if (auto* fimo = followIMO.get())
						{
							Transform followPlacement = fimo->get_presence()->get_placement();
							debug_draw_arrow(true, Colour::greyLight, at, followPlacement.get_translation());
							debug_draw_transform_size(true, followPlacement, 2.0f);
						}
					}
					debug_no_context();
					debug_no_filter();
#endif

					velocityLinear += changeVelocity;
				}
				else
				{
					if (inTransit)
					{
						locOff = locOff.normal() * max(0.0f, locOff.length() - (transitThrough ? 0.0f : setup.inTransitEarlyStop));
						Rotator3 r = rotation.get();
						r.pitch = clamp(locOff.to_rotator().pitch, -setup.inTransitMaxPitch, setup.inTransitMaxPitch);
						locOff = r.get_forward() * locOff.length();
					}

					float linSpeed = velocityLinear.length();
					if (transitThrough || locOff.length() > 0.01f || linSpeed > 0.1f)
					{
						// S = Vt - at^2 / 2
						// V - at = 0
						// therefore
						// V = a/t
						// V^2 = 2aS
						// V = \/2aS
						float distanceReq = locOff.length();
						float targetSpeed = transitThrough || (maneuverThrough && ! inTransit)? maxLinSpeed : min(maxLinSpeed, sqrt(2.0f * linAccel * distanceReq * (0.8f)));

						if (!inTransit)
						{
							//targetSpeed *= (0.3f + 1.2f * clamp(Vector3::dot(velocityLinear.normal(), locOff.normal()), 0.0f, 1.0f));
						}
						Vector3 targetVelocity = locOff.normal() * targetSpeed;

						Vector3 changeRequired = targetVelocity - velocityLinear;

						Vector3 changeVelocity = changeRequired.normal() * min(linAccel * _deltaTime, changeRequired.length());

#ifdef AN_DEBUG_RENDERER
						debug_filter(ai_draw);
						debug_context(imo->get_presence()->get_in_room());
						{
							Vector3 at = imo->get_presence()->get_placement().get_translation();
							debug_draw_arrow(true, Colour::mint, at, at + velocityLinear);
							debug_draw_arrow(true, Colour::white, at, at + targetVelocity);
							debug_draw_arrow(true, Colour::c64Brown, at + velocityLinear, at + velocityLinear + changeVelocity);
							debug_draw_arrow(true, Colour::c64LightGreen, at + velocityLinear + changeVelocity, at + velocityLinear + changeRequired);
						}
						debug_no_context();
						debug_no_filter();
#endif

						velocityLinear += changeVelocity;
					}
					else
					{
						velocityLinear = blend_to_using_time(velocityLinear, locOff * 0.5f, 0.2f, _deltaTime);
					}

					/*
					if (inTransit)
					{
						float diff = max(setup.maneuver.linSpeed * 2.0f, setup.transit.linSpeed * 0.5f) - setup.maneuver.linSpeed;
						float useAligned = clamp((velocityLinear.length() - setup.maneuver.linSpeed) / diff, 0.0f, 1.0f);
						if (useAligned > 0.0f)
						{
							Vector3 alignedVelocity = (Vector3::yAxis.rotated_by_yaw(rotation.get().yaw) + Vector3::zAxis * velocityLinear.z).normal() * velocityLinear.length();
							velocityLinear = velocityLinear * (1.0f - useAligned) + velocityLinear * useAligned;
						}
					}
					*/
				}
			}
			else if (noLocationGoForward)
			{
				if (rotation.is_set())
				{
					float targetSpeed = maxLinSpeed;

					Rotator3 forward = rotation.get();

					if (pitchAccel <= 0.0f)
					{
						forward.pitch = targetPitch.get(0.0f);
					}
					else
					{
						float curPitch = velocityLinear.to_rotator().pitch;
						float tgtPitch = targetPitch.get(0.0f);
						forward.pitch = blend_to_using_speed(curPitch, tgtPitch, pitchAccel, _deltaTime);
					}
					Vector3 targetVelocity = forward.get_forward().normal() * targetSpeed;

					Vector3 changeRequired = targetVelocity - velocityLinear;

					Vector3 changeVelocity = changeRequired.normal() * min(linAccel * _deltaTime, changeRequired.length());

					velocityLinear += changeVelocity;
				}
			}
			else
			{
				if (! velocityLinear.is_zero())
				{
					velocityLinear = velocityLinear.normal() * max(0.0f, velocityLinear.length() - linAccel * _deltaTime);
				}
			}
		}

		float useRotAccel = clamp(abs(velocityRotation.yaw - prevVelocityRotation.yaw) / (use.yawAcceleration * _deltaTime), 0.0f, 1.0f);
		float useLinAccel = clamp((velocityLinear - prevVelocityLinear).length() / (use.linAcceleration * _deltaTime), 0.0f, 1.0f);

		if (atLocTarget && (atYawTarget || ignoreYawMatchForTrigger))
		{
			if (triggerGameScriptTrap.is_valid())
			{
				Framework::GameScript::ScriptExecution::trigger_execution_trap(triggerGameScriptTrap);
				triggerGameScriptTrap = Name(); // clear trap
			}
			if (triggerOwnGameScriptTrap.is_valid())
			{
				if (auto* ai = fast_cast<ModuleAI>(imo->get_ai()))
				{
					if (auto* gse = ai->get_game_script_execution())
					{
						gse->trigger_own_execution_trap(triggerOwnGameScriptTrap);
						triggerOwnGameScriptTrap = Name(); // clear trap
					}
				}
			}
		}

		// move
		if (_deltaTime != 0.0f)
		{
			Vector3 velocityAcceleration = (velocityLinear - prevVelocityLinear) / _deltaTime;
			Vector3 velocityAccelerationOS = rotation.get().to_quat().to_local(velocityAcceleration);

			{
				Rotator3 targetRotationOffset = Rotator3::zero;
				if (forceFollowInTransit)
				{
					targetRotationOffset.pitch += clamp((targetLocation.get() + velocityLinear * 1.0f - location.get()).to_rotator().pitch * setup.inTransitApplyPitch, -setup.inTransitMaxPitch, setup.inTransitMaxPitch);
				}
				else if (inTransit && targetLocation.is_set())
				{
					targetRotationOffset.pitch += clamp((targetLocation.get() - location.get()).to_rotator().pitch * setup.inTransitApplyPitch, -setup.inTransitMaxPitch, setup.inTransitMaxPitch);
				}
				targetRotationOffset.roll += velocityAccelerationOS.x * use.xAccelToRoll;
				targetRotationOffset.pitch += velocityAccelerationOS.y * use.yAccelToPitch;
				rotationOffsetFromLinear = blend_to_using_time(rotationOffsetFromLinear, targetRotationOffset * (1.0f - 0.3f * extraRotationBlocked), 0.5f, _deltaTime);
			}

			{
				Rotator3 targetRotationOffset = Rotator3::zero;
				targetRotationOffset.roll += velocityRotation.yaw * use.velocityYawToRoll;
				rotationOffsetFromRotation = blend_to_using_time(rotationOffsetFromRotation, targetRotationOffset * (1.0f - 0.3f * extraRotationBlocked), 0.2f, _deltaTime);
			}

			location = location.get() + velocityLinear * _deltaTime;
			rotation = rotation.get() + velocityRotation * _deltaTime;
		}

		{
			struct Utils
			{
				static void setup(Framework::IModulesOwner* imo, Framework::SoundSourcePtr& soundSourcePtr, Name& setupSound, Framework::MeshGenParam<Name> const& soundParam, bool _craftOff, bool _hardRestart)
				{
					if (setupSound.is_valid() ||
						soundParam.is_set())
					{
						if (_craftOff || _hardRestart)
						{
							if (auto* s = soundSourcePtr.get())
							{
								s->stop(_hardRestart? Optional<float>(0.0f) : NP);
								soundSourcePtr.clear();
							}
						}
						if (!_craftOff && !soundSourcePtr.get())
						{
							if (soundParam.is_set())
							{
								setupSound = soundParam.get(imo->get_variables());
							}
							if (setupSound.is_valid())
							{
								if (auto* s = imo->get_sound())
								{
									soundSourcePtr = s->play_sound(setupSound);
								}
							}
						}
					}
				}
			};
			Utils::setup(imo, engineSound, setup.engineSound, h_craftData->engineSound, craftOff || beSilent, restartFlyingEngineSounds);
			Utils::setup(imo, flyingSound, setup.flyingSound, h_craftData->flyingSound, craftOff || beSilent, restartFlyingEngineSounds);
			restartFlyingEngineSounds = false;
		}

		if (h_craftData->outputEnginePowerVar.is_valid() ||
			h_craftData->outputSpeedVar.is_valid())
		{
			if (!outputEnginePowerVar.is_valid())
			{
				outputEnginePowerVar.set_name(h_craftData->outputEnginePowerVar);
				outputEnginePowerVar.look_up<float>(imo->access_variables());
			}
			if (!outputSpeedVar.is_valid())
			{
				outputSpeedVar.set_name(h_craftData->outputSpeedVar);
				outputSpeedVar.look_up<float>(imo->access_variables());
			}
			float useSpeed = (velocityLinear.length()) / setup.transit.linSpeed;
			if (outputEnginePowerVar.is_valid())
			{
				float enginePowerTarget = useSpeed;
				enginePowerTarget += (abs(velocityRotation.yaw) / useYawSpeed) * h_craftData->useRotationForEnginePower;
				enginePowerTarget += useLinAccel * h_craftData->useLinearAccelerationForEnginePower;
				enginePowerTarget += useRotAccel * h_craftData->useRotationAccelerationForEnginePower;
				if (enginePowerTarget < 1.0f && h_craftData->minEnginePower > 0.0f)
				{
					enginePowerTarget = h_craftData->minEnginePower + (1.0f - h_craftData->minEnginePower) * enginePowerTarget;
				}
				enginePower = blend_to_using_time(enginePower, enginePowerTarget, h_craftData->enginePowerBlendTime, _deltaTime);
				outputEnginePowerVar.access<float>() = enginePower;

				dPower = clamp(0.2f + enginePower * 0.8f, 0.0f, 3.0f) * 300.0f;
			}
			if (outputSpeedVar.is_valid())
			{
				outputSpeedVar.access<float>() = useSpeed;
			}
		}

		// display
		{
			dSpeed = velocityLinear.length() * 72.0f / 20.0f; // to kmh
			dHeading = mod(rotation.get().yaw + 245.0f, 360.0f);
			dAltitude = location.get().z + 152.0f;
			dBattery = 47.0f;
			if (dId == 0.0f)
			{
				auto rg = imo->get_individual_random_generator();
				dId = (float)rg.get_int(1000);
			}
		}

		if (h_craftData->displayMessageVar.is_valid() && ! craftOff)
		{
			if (!displayMessageVar.is_valid())
			{
				displayMessageVar.set_name(h_craftData->displayMessageVar);
				displayMessageVar.look_up<int>(imo->access_variables());
			}
			if (displayMessageVar.is_valid())
			{
				int newValue = displayMessageVar.get<int>();
				if (displayMessageVarValue != newValue)
				{
					displayMessageVarValue = newValue;
					redrawNow = true;
				}
			}
		}

		if (craftOff)
		{
			if (display)
			{
				display->turn_off();
			}
			displayActive = false;
		}
		else
		{
			if (h_craftData->displayActiveVar.is_valid())
			{
				if (!displayActiveVar.is_valid())
				{
					displayActiveVar.set_name(h_craftData->displayActiveVar);
					displayActiveVar.look_up<bool>(imo->access_variables());
					if (!displayActiveVar.get<bool>() && display)
					{
						display->turn_off();
					}
				}
				if (displayActiveVar.is_valid())
				{
					bool newValue = displayActiveVar.get<bool>();
					if (displayActive != newValue)
					{
						displayActive = newValue;
						redrawNow = true;
						if (display)
						{
							if (displayActive)
							{
								display->turn_on();
							}
							else
							{
								display->turn_off();
							}
						}
					}
				}
			}
			else
			{
				displayActive = true;
			}
		}

#ifdef AN_DEBUG_RENDERER
		debug_filter(ai_draw);
		debug_context(imo->get_presence()->get_in_room());
		if (false)
		{
			Vector3 at = imo->get_presence()->get_placement().get_translation();
			if (orgTargetLocation.is_set())
			{
				if (triggerBelowDistance.is_set())
				{
					Vector3 mid = orgTargetLocation.get() + (at - orgTargetLocation.get()).normal() * min(triggerBelowDistance.get(), (at - orgTargetLocation.get()).length());
					debug_draw_line(true, Colour::purple, at, mid);
					debug_draw_arrow(true, Colour::lerp(0.6f, Colour::purple, Colour::white), mid, orgTargetLocation.get());
				}
				debug_draw_arrow(true, Colour::purple.with_alpha(0.2f), at, targetLocation.get());
			}
			{
				Vector3 atvl = at + velocityLinear;
				if (velocityLinear.length() > setup.maneuver.linSpeed)
				{
					Vector3 atvlm = at + velocityLinear.normal() * setup.maneuver.linSpeed;
					debug_draw_line(true, Colour::cyan, at, atvlm);
					debug_draw_arrow(true, Colour::orange, atvlm, atvl);
				}
				else
				{
					debug_draw_arrow(true, Colour::cyan, at, atvl);
				}
			}
			debug_draw_arrow(true, Colour::green, at, at + rotation.get().get_forward() * 5.0f);
			debug_draw_arrow(true, Colour::red, at, at + Rotator3(0.0f, targetYaw.get(0.0f), 0.0f).get_forward() * 5.0f);
			debug_draw_sphere(true, true, inTransit? Colour::cyan : Colour::red, 0.3f, Sphere(location.get(), 0.1f));
		}
		debug_no_context();
		debug_no_filter();
#endif
	}

	updateDisplayTime -= _deltaTime;
	if (updateDisplayTime <= 0.0f)
	{
		redrawNow = true;
		updateDisplayTime += 1.0f;
	}

	if (!display && displayMessageVar.is_valid() && (!displayActiveVar.is_valid() || displayActiveVar.get<bool>()))
	{
		if (imo)
		{
			if (auto* displayModule = imo->get_custom<::Framework::CustomModules::Display>())
			{
				display = displayModule->get_display();
				redrawNow = true;
				if (display)
				{
					if (displayActive)
					{
						display->turn_on();
					}
					else
					{
						display->turn_off();
					}
					display->draw_all_commands_immediately();
					display->set_on_update_display(this,
						[this](Framework::Display* _display)
						{
							if (!redrawNow)
							{
								return;
							}
							redrawNow = false;

							_display->drop_all_draw_commands();
							_display->add((new Framework::DisplayDrawCommands::CLS(Colour::black))->immediate_draw());
							_display->add((new Framework::DisplayDrawCommands::Border(Colour::black))->immediate_draw());

							if (displayActive)
							{
								{
									String text = Framework::StringFormatter::format_loc_str(h_craftData->layout, Framework::StringFormatterParams()
										.add(NAME(s), String::printf(TXT("%03.0f"), dSpeed))
										.add(NAME(h), String::printf(TXT("%03.0f"), dHeading))
										.add(NAME(a), String::printf(TXT("%03.0f"), dAltitude))
										.add(NAME(b), String::printf(TXT("%03.0f"), dBattery))
										.add(NAME(p), String::printf(TXT("%03.0f"), dPower))
										.add(NAME(i), String::printf(TXT("%03.0f"), dId))
									);
									_display->add((new Framework::DisplayDrawCommands::TextAtMultiline())
										->text(text)
										->in_whole_display(display)
										->v_align_top()
										->h_align_left()
										->immediate_draw());
								}

								for_every(msg, h_craftData->messages)
								{
									if (msg->varValue == displayMessageVarValue)
									{
										if (h_craftData->messagesAt.is_empty())
										{
											_display->add((new Framework::DisplayDrawCommands::TextAtMultiline())
												->text(msg->message.get())
												->in_whole_display(display)
												->v_align_centre()
												->h_align_centre()
												->immediate_draw());
										}
										else
										{
											_display->add((new Framework::DisplayDrawCommands::TextAtMultiline())
												->text(msg->message.get())
												->in_region(h_craftData->messagesAt)
												->v_align_centre()
												->h_align_centre()
												->immediate_draw());
										}
									}
								}
							}
						});
				}
			}
		}
	}
}

void H_Craft::reset_requests(bool _keepFollowOffsetAdditional)
{
	devTeleport = false;
	
	instantFacing = false;
	instantManeuverSpeed.clear();
	instantTransitSpeed.clear();
	instantFollowing = false;

	forcedYawSpeed.clear();
	yawSpeed.clear();
	yawAcceleration.clear();
	pitchAcceleration.clear();

	noLocationGoForward = false;
	matchYaw.clear();
	matchPitch.clear();
	matchLocation.clear();
	matchToIMO.clear();
	ignoreYawMatchForTrigger = false;
	followIMO.clear();
	followIMOInTransit.clear();
	followIMOIgnoreForward = false;
	followPOIs.clear();
	limitFollowRelVelocity.clear();
	followRelVelocity.clear();
	followOffset.clear();
	if (!_keepFollowOffsetAdditional)
	{
		followOffsetAdditionalRange.clear();
		followOffsetAdditionalInterval.clear();
	}

	matchSocket = Name();
	matchToIMOSocket = Name();
	targetSocketOffset.clear();
	targetSocketOffsetYaw.clear();

	requestedManeuverSpeed.clear();
	requestedManeuverSpeedPt.clear();
	requestedManeuverAccelPt.clear();
	requestedTransitSpeed.clear();
	requestedTransitSpeedPt.clear();
	requestedTransitAccelPt.clear();
	transitThrough = false;
	maneuverThrough = false;

	triggerGameScriptTrap = Name();
	triggerOwnGameScriptTrap = Name();
	triggerBelowDistance.clear();
}

#define READ_PARAM_IF_BOOL(_where) \
	auto* (ptr_##_where) = _message.get_param(NAME(_where)); \
	if ((ptr_##_where) && (ptr_##_where)->get_as<bool>())

#define READ_PARAM(_type, _where) \
	if (auto* ptr = _message.get_param(NAME(_where))) \
	{ \
		self->_where = ptr->get_as<_type>(); \
	}

#define READ_PARAM_EX(_type, _where, _name) \
	if (auto* ptr = _message.get_param(NAME(_name))) \
	{ \
		self->_where = ptr->get_as<_type>(); \
	}

LATENT_FUNCTION(H_Craft::execute_logic)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto* imo = mind->get_owner_as_modules_owner();

	auto* self = fast_cast<H_Craft>(logic);

	LATENT_BEGIN_CODE();

	if (auto* ai = imo->get_ai())
	{
		auto& arla = ai->access_rare_logic_advance();
		arla.reset_to_no_rare_advancement();
	}

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(aim_die), [self](Framework::AI::Message const& _message)
		{
			self->dying = true;
			Random::Generator rg;
			self->dyingTimeLeft = rg.get_float(0.5f, 1.25f);
			self->dyingFullAffectVelocityLinearTime = 0.3f;
			self->dyingFullAffectVelocityRotationTime = 0.0f;
			self->dyingAcceleration = rg.get_float(-25.0f, -20.0f);
			self->dyingRotation.pitch = rg.get_float(10.0f, 40.0f) * (rg.get_bool() ? 1.0f : -1.0f);
			self->dyingRotation.yaw = rg.get_float(10.0f, 30.0f) * (rg.get_bool() ? 1.0f : -1.0f);
			self->dyingRotation.roll = rg.get_float(10.0f, 40.0f) * (rg.get_bool() ? 1.0f : -1.0f);
			if (rg.get_chance(0.4f))
			{
				self->dyingRotation.pitch = rg.get_float(120.0f, 180.0f);
			}
			else
			{
				if (rg.get_chance(0.3f))
				{
					self->dyingRotation.yaw = rg.get_float(120.0f, 180.0f) * (rg.get_bool() ? 1.0f : -1.0f);
				}
				else if (rg.get_chance(0.8f))
				{
					self->dyingRotation.roll = rg.get_float(300.0f, 500.0f) * (rg.get_bool() ? 1.0f : -1.0f);
					self->dyingRotation.pitch *= 0.3f;
					self->dyingRotation.yaw *= 0.3f;
				}
			}
		});
		messageHandler.set(NAME(aim_devTeleportEverywhere), [self](Framework::AI::Message const& _message)
		{
			self->devTeleportEverywhere = true;
		});
		messageHandler.set(NAME(aim_rotation), [self](Framework::AI::Message const& _message)
		{
			if (auto* ptr = _message.get_param(NAME(blockExtraRotation)))
			{
				self->blockExtraRotation = ptr->get_as<bool>();
			}
		});
		messageHandler.set(NAME(aim_goTo), [self](Framework::AI::Message const& _message)
		{
			bool keepActiveRequest = false;
			if (auto * ptr = _message.get_param(NAME(keepActiveRequest)))
			{
				keepActiveRequest = ptr->get_as<bool>();
			}
			if (!keepActiveRequest)
			{
				self->reset_requests();
			}

			if (auto* ptr = _message.get_param(NAME(offsetYaw)))
			{
				if (self->rotation.is_set())
				{
					self->matchYaw = self->rotation.get().yaw + ptr->get_as<float>();
				}
			}
				
			if (auto* ptr = _message.get_param(NAME(offsetLocOS)))
			{
				if (self->location.is_set() &&
					self->rotation.is_set())
				{
					self->matchLocation = self->location.get() + self->rotation.get().to_quat().to_world(ptr->get_as<Vector3>());
				}
			}
				
			READ_PARAM(bool, noLocationGoForward);
			if (auto* targetYawPtr = _message.get_param(NAME(targetYaw)))
			{
				bool relativeToEnvironment = false;
				if (auto* rtePtr = _message.get_param(NAME(relativeToEnvironment)))
				{
					relativeToEnvironment = rtePtr->get_as<bool>();
				}
				float offsetYaw = 0.0f;
				if (relativeToEnvironment)
				{
					if (auto* imo = self->get_imo())
					{
						if (auto* r = imo->get_presence()->get_in_room())
						{
							if (r->get_tags().has_tag(NAME(openWorldExterior)))
							{
								bool found = false;
								{
									Framework::PointOfInterestInstance* foundPOI = nullptr;
									if (!found && r->find_any_point_of_interest(NAME(environmentAnchor), foundPOI))
									{
										Transform ea = foundPOI->calculate_placement();
										offsetYaw -= ea.get_orientation().to_rotator().yaw; // to local
										found = true;
									}
								}
								if (!found)
								{
									if (auto* rt = r->get_room_type())
									{
										if (!found && rt->get_use_environment().anchorPOI.is_valid())
										{
											Framework::PointOfInterestInstance* foundPOI = nullptr;
											if (r->find_any_point_of_interest(rt->get_use_environment().anchorPOI, foundPOI))
											{
												Transform ea = foundPOI->calculate_placement();
												offsetYaw -= ea.get_orientation().to_rotator().yaw; // to local
												found = true;
											}
										}
										if (!found && rt->get_use_environment().anchor.is_set())
										{
											Transform placement = rt->get_use_environment().anchor.get(r, Transform::zero, AllowToFail);
											if (!placement.is_zero())
											{
												offsetYaw -= placement.get_orientation().to_rotator().yaw; // to local
												found = true;
											}
										}
									}
								}
							}
						}
					}
				}
				if (targetYawPtr->get_type() == type_id<float>())
				{
					self->matchYaw = offsetYaw + targetYawPtr->get_as<float>();
				}
				else if (targetYawPtr->get_type() == type_id<Range>())
				{
					self->matchYaw = offsetYaw + self->rg.get(targetYawPtr->get_as<Range>());
				}
			}
			READ_PARAM_EX(float, matchPitch, targetPitch);
			READ_PARAM_EX(Vector3, matchLocation, targetLoc);
			READ_PARAM(Vector3, targetSocketOffset);
			READ_PARAM(float, targetSocketOffsetYaw);
			READ_PARAM(float, yawSpeed);
			READ_PARAM(float, yawAcceleration);
			READ_PARAM(float, pitchAcceleration);
			READ_PARAM(float, requestedManeuverSpeed);
			READ_PARAM(float, requestedManeuverSpeedPt);
			READ_PARAM(float, requestedManeuverAccelPt);
			READ_PARAM(float, requestedTransitSpeed);
			READ_PARAM(float, requestedTransitSpeedPt);
			READ_PARAM(float, requestedTransitAccelPt);
			READ_PARAM(bool, instantFacing);
			READ_PARAM(bool, ignoreYawMatchForTrigger);
			READ_PARAM(bool, devTeleport);
			
			READ_PARAM(float, forcedYawSpeed);

			READ_PARAM_IF_BOOL(maintainManeuverSpeed)
			{
				self->requestedManeuverSpeed = self->get_velocity_linear().length();
			}

			READ_PARAM_IF_BOOL(maintainTransitSpeed)
			{
				self->requestedTransitSpeed = self->get_velocity_linear().length();
			}

			if (auto* toPOIPtr = _message.get_param(NAME(toPOI)))
			{
				if (toPOIPtr->get_type() == type_id<Name>())
				{
					Name toPOI = toPOIPtr->get_as<Name>();
					if (auto* imo = self->get_imo())
					{
						if (auto* r = imo->get_presence()->get_in_room())
						{
							Framework::PointOfInterestInstance* foundPOI = nullptr;
							if (r->find_any_point_of_interest(toPOI, foundPOI))
							{
								self->matchLocation = foundPOI->calculate_placement().get_translation();
							}
							else
							{
								error(TXT("POI \"%S\" not found"), toPOI.to_char());
							}
						}
					}
				}
			}

			if (auto* instantManeuverSpeedPtr = _message.get_param(NAME(instantManeuverSpeed)))
			{
				if (instantManeuverSpeedPtr->get_type() == type_id<bool>())
				{
					if (instantManeuverSpeedPtr->get_as<bool>())
					{
						self->instantManeuverSpeed = 1.0f;
					}
				}
				if (instantManeuverSpeedPtr->get_type() == type_id<float>())
				{
					self->instantManeuverSpeed = instantManeuverSpeedPtr->get_as<float>();
				}
			}

			if (auto* instantTransitSpeedPtr = _message.get_param(NAME(instantTransitSpeed)))
			{
				if (instantTransitSpeedPtr->get_type() == type_id<bool>())
				{
					if (instantTransitSpeedPtr->get_as<bool>())
					{
						self->instantTransitSpeed = 1.0f;
					}
				}
				if (instantTransitSpeedPtr->get_type() == type_id<float>())
				{
					self->instantTransitSpeed = instantTransitSpeedPtr->get_as<float>();
				}
			}

			READ_PARAM(bool, transitThrough);
			READ_PARAM(bool, maneuverThrough);
			READ_PARAM(Name, triggerGameScriptTrap);
			READ_PARAM(Name, triggerOwnGameScriptTrap);
			READ_PARAM(float, triggerBelowDistance);

			READ_PARAM_EX(Name, matchSocket, matchSockets);
			READ_PARAM_EX(Name, matchToIMOSocket, matchSockets);
			READ_PARAM(Name, matchSocket);
			READ_PARAM(Name, matchToIMOSocket);

			if (auto* matchToIMOPtr = _message.get_param(NAME(matchToIMO)))
			{
				auto * imo = matchToIMOPtr->get_as< SafePtr<Framework::IModulesOwner> >().get();
				if (imo)
				{
					self->matchToIMO = imo;
				}
				else
				{
					self->matchToIMO.clear();
				}
			}
		});
		messageHandler.set(NAME(aim_follow), [self](Framework::AI::Message const& _message)
		{
			bool keepFollowOffsetAdditional = false;
			if (auto* ptr = _message.get_param(NAME(keepFollowOffsetAdditional)))
			{
				keepFollowOffsetAdditional = ptr->get_as<bool>();
			}

			bool keepActiveRequest = false;
			if (auto * ptr = _message.get_param(NAME(keepActiveRequest)))
			{
				keepActiveRequest = ptr->get_as<bool>();
			}
			if (!keepActiveRequest)
			{
				self->reset_requests(keepFollowOffsetAdditional);
			}

			if (auto* followIMOPtr = _message.get_param(NAME(followIMO)))
			{
				self->followIMOInTransit.clear();
				auto* imo = followIMOPtr->get_as< SafePtr<Framework::IModulesOwner> >().get();
				if (imo)
				{
					self->followIMO = imo;
				}
				else
				{
					self->followIMO.clear();
				}
			}
			if (auto* followIMOInTransitPtr = _message.get_param(NAME(followIMOInTransit)))
			{
				self->followIMOInTransit = followIMOInTransitPtr->get_as<bool>();
			}
			if (auto* clearFollowIMOInTransitPtr = _message.get_param(NAME(clearFollowIMOInTransit)))
			{
				if (clearFollowIMOInTransitPtr->get_as<bool>())
				{
					self->followIMOInTransit.clear();
				}
			}
			if (auto* followIMOIgnoreForwardPtr = _message.get_param(NAME(followIMOIgnoreForward)))
			{
				self->followIMOIgnoreForward = followIMOIgnoreForwardPtr->get_as<bool>();
			}

			READ_PARAM(float, limitFollowRelVelocity);
			READ_PARAM(Vector3, followRelVelocity);
			READ_PARAM(Vector3, followOffset);
			if (auto* ptr = _message.get_param(NAME(followOffsetFromCurrent)))
			{
				if (ptr->get_as<bool>())
				{
					if (self->followIMO.get() && self->location.is_set())
					{
						Transform followPlacement = self->followIMO->get_presence()->get_placement();
						self->followOffset = followPlacement.location_to_local(self->location.get());
					}
				}
			}
			READ_PARAM(Range3, followOffsetAdditionalRange);
			READ_PARAM(Range, followOffsetAdditionalInterval);
			READ_PARAM(float, requestedManeuverSpeed);
			READ_PARAM(float, requestedManeuverSpeedPt);
			READ_PARAM(float, requestedManeuverAccelPt);
			READ_PARAM(float, requestedTransitSpeed);
			READ_PARAM(float, requestedTransitSpeedPt);
			READ_PARAM(float, requestedTransitAccelPt);
			self->beSilent = false;
			READ_PARAM(bool, beSilent);
			self->restartFlyingEngineSounds = false;
			READ_PARAM(bool, restartFlyingEngineSounds);
			READ_PARAM(bool, instantFacing);
			READ_PARAM(bool, instantFollowing);
			READ_PARAM(bool, ignoreYawMatchForTrigger);
			READ_PARAM(bool, devTeleport);

			if (auto* instantManeuverSpeedPtr = _message.get_param(NAME(instantManeuverSpeed)))
			{
				if (instantManeuverSpeedPtr->get_type() == type_id<bool>())
				{
					if (instantManeuverSpeedPtr->get_as<bool>())
					{
						self->instantManeuverSpeed = 1.0f;
					}
				}
				if (instantManeuverSpeedPtr->get_type() == type_id<float>())
				{
					self->instantManeuverSpeed = instantManeuverSpeedPtr->get_as<float>();
				}
			}

			if (auto* instantTransitSpeedPtr = _message.get_param(NAME(instantTransitSpeed)))
			{
				if (instantTransitSpeedPtr->get_type() == type_id<bool>())
				{
					if (instantTransitSpeedPtr->get_as<bool>())
					{
						self->instantTransitSpeed = 1.0f;
					}
				}
				if (instantTransitSpeedPtr->get_type() == type_id<float>())
				{
					self->instantTransitSpeed = instantTransitSpeedPtr->get_as<float>();
				}
			}

			READ_PARAM(Name, triggerGameScriptTrap);
			READ_PARAM(Name, triggerOwnGameScriptTrap);
			READ_PARAM(float, triggerBelowDistance);
		});
		messageHandler.set(NAME(aim_followPOIs), [self](Framework::AI::Message const& _message)
		{
			bool keepActiveRequest = false;
			if (auto * ptr = _message.get_param(NAME(keepActiveRequest)))
			{
				keepActiveRequest = ptr->get_as<bool>();
			}
			if (!keepActiveRequest)
			{
				self->reset_requests();
			}

			if (auto* followPOIsPtr = _message.get_param(NAME(followPOIs)))
			{
				self->followPOIs = followPOIsPtr->get_as<Name>();
			}

			READ_PARAM(float, requestedManeuverSpeed);
			READ_PARAM(float, requestedManeuverSpeedPt);
			READ_PARAM(float, requestedManeuverAccelPt);
			READ_PARAM(float, requestedTransitSpeed);
			READ_PARAM(float, requestedTransitSpeedPt);
			READ_PARAM(float, requestedTransitAccelPt);
			self->beSilent = false;
			READ_PARAM(bool, beSilent);
			self->restartFlyingEngineSounds = false;
			READ_PARAM(bool, restartFlyingEngineSounds);
		});
	}

	if (self->useCanister)
	{
		self->canister.initialise(imo, NAME(upgradeCanisterId));
	}

	while (true)
	{
		LATENT_WAIT_NO_RARE_ADVANCE(self->rg.get_float(0.1f, 0.5f));
	}

	LATENT_ON_BREAK();
	//
	
	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//--

Vector3 H_Craft::FollowPOIPath::process(FoundPOI const* _pois, int _poiCount, float _relativeDistance)
{
	Transform placementWS = _pois->placement;

	{
		FoundPOI const* poi = _pois;
		FoundPOI const* poiNext = poi;
		++poiNext;

		float useRelativeDistance = _relativeDistance;

		bool found = false;
		for_count(int, i, _poiCount - 1)
		{
			float poiR = poi->relativePlacement - useRelativeDistance;
			float poiNR = poiNext->relativePlacement - useRelativeDistance;
			if (poiR <= 0.0f && poiNR >= 0.0f)
			{
				float pt = clamp(poiNR != poiR ? (0.0f - poiR) / (poiNR - poiR) : 0.5f, 0.0f, 1.0f);
				placementWS = Transform::lerp(pt, poi->placement, poiNext->placement);
				found = true;
				break;
			}
			++poi;
			++poiNext;
		}

		if (!found)
		{
			an_assert(false, TXT("use larger range"));
			if (useRelativeDistance < _pois->relativePlacement)
			{
				placementWS = _pois->placement;
			}
			else
			{
				placementWS = _pois[_poiCount - 1].placement;
			}
		}
	}

	Vector3 locWS = placementWS.get_translation();

	return locWS;
}

//--

REGISTER_FOR_FAST_CAST(H_CraftData);

bool H_CraftData::load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= setup.load_from_xml(_node);

	outputEnginePowerVar = _node->get_name_attribute_or_from_child(TXT("outputEnginePowerVar"), outputEnginePowerVar);
	outputSpeedVar = _node->get_name_attribute_or_from_child(TXT("outputSpeedVar"), outputSpeedVar);
	minEnginePower = _node->get_float_attribute_or_from_child(TXT("minEnginePower"), minEnginePower);
	useRotationForEnginePower = _node->get_float_attribute_or_from_child(TXT("useRotationForEnginePower"), useRotationForEnginePower);
	useLinearAccelerationForEnginePower = _node->get_float_attribute_or_from_child(TXT("useLinearAccelerationForEnginePower"), useLinearAccelerationForEnginePower);
	useRotationAccelerationForEnginePower = _node->get_float_attribute_or_from_child(TXT("useRotationAccelerationForEnginePower"), useRotationAccelerationForEnginePower);
	enginePowerBlendTime = _node->get_float_attribute_or_from_child(TXT("enginePowerBlendTime"), enginePowerBlendTime);

	for_every(node, _node->children_named(TXT("engine")))
	{
		engineSound.load_from_xml(node, TXT("sound"));
	}

	for_every(node, _node->children_named(TXT("flying")))
	{
		flyingSound.load_from_xml(node, TXT("sound"));
	}

	for_every(node, _node->children_named(TXT("display")))
	{
		displayMessageVar = node->get_name_attribute(TXT("var"), displayMessageVar);
		displayActiveVar = node->get_name_attribute(TXT("activeVar"), displayActiveVar);
		result &= layout.load_from_xml_child(node, TXT("layout"), _lc, TXT("layout"));
		messagesAt.load_from_xml_child_node(node, TXT("at"));
		for_every(mNode, node->children_named(TXT("message")))
		{
			Message m;
			String messageId = String::printf(TXT("%i"), messages.get_size());
			if (m.message.load_from_xml(mNode, _lc, messageId.to_char()))
			{
				m.varValue = mNode->get_int_attribute(TXT("varValue"), m.varValue);
				messages.push_back(m);
			}
			else
			{
				error_loading_xml(node, TXT("couldn't load message"));
				result = false;
			}
		}
	}

	return result;
}

bool H_CraftData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

//--

bool H_CraftMovementParams::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	{
		tchar const * readNodeName[] = { TXT("yawRotation"), TXT("yaw"), TXT("rotation"), nullptr };
		for (int i = 0; readNodeName[i]; ++i)
		{
			for_every(node, _node->children_named(readNodeName[i]))
			{
				yawSpeed = node->get_float_attribute(TXT("speed"), yawSpeed);
				yawAcceleration = node->get_float_attribute(TXT("accel"), yawAcceleration);
				yawAcceleration = node->get_float_attribute(TXT("acceleration"), yawAcceleration);
			}
		}
	}

	for_every(node, _node->children_named(TXT("linear")))
	{
		linSpeed = node->get_float_attribute(TXT("speed"), linSpeed);
		linAcceleration = node->get_float_attribute(TXT("accel"), linAcceleration);
		linAcceleration = node->get_float_attribute(TXT("acceleration"), linAcceleration);
	}

	for_every(node, _node->children_named(TXT("rotation")))
	{
		velocityYawToRoll = node->get_float_attribute(TXT("velocityYawToRoll"), velocityYawToRoll);
		yAccelToPitch = node->get_float_attribute(TXT("yAccelToPitch"), yAccelToPitch);
		xAccelToRoll = node->get_float_attribute(TXT("accel"), xAccelToRoll);
	}

	return result;
}

//--

bool H_CraftSetup::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	changeToManeuverDistance = _node->get_float_attribute(TXT("changeToManeuverDistance"), changeToManeuverDistance);
	changeToTransitDistance = _node->get_float_attribute(TXT("changeToTransitDistance"), changeToTransitDistance);

	for_every(node, _node->children_named(TXT("maneuver")))
	{
		result &= maneuver.load_from_xml(node);
		changeToTransitDistance = node->get_float_attribute(TXT("changeToTransitDistance"), changeToTransitDistance);
	}

	{
		tchar const* readNodeName[] = { TXT("transit"), TXT("inTransit"), nullptr };
		for (int i = 0; readNodeName[i]; ++i)
		{
			for_every(node, _node->children_named(readNodeName[i]))
			{
				inTransitEarlyStop = node->get_float_attribute(TXT("earlyStop"), inTransitEarlyStop);
				inTransitMaxPitch = node->get_float_attribute(TXT("maxPitch"), inTransitMaxPitch);
				inTransitApplyPitch = node->get_float_attribute(TXT("applyPitch"), inTransitApplyPitch);
				for_every(onode, node->children_named(TXT("offset")))
				{
					inTransitOffsetPeriod.load_from_xml(onode, TXT("period"));
					inTransitOffset.load_from_xml(onode);
				}
				result &= transit.load_from_xml(node);

				changeToManeuverDistance = node->get_float_attribute(TXT("changeToManeuverDistance"), changeToManeuverDistance);
			}
		}
	}

	for_every(node, _node->children_named(TXT("engine")))
	{
		engineSound = node->get_name_attribute(TXT("sound"), engineSound);
	}

	for_every(node, _node->children_named(TXT("flying")))
	{
		flyingSound = node->get_name_attribute(TXT("sound"), flyingSound);
	}

	return result;
}
