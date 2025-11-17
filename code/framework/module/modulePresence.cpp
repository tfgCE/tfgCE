#include "modulePresence.h"

#include "modules.h"
#include "moduleCollisionHeaded.h"
#include "moduleCustom.h"
#include "moduleDataImpl.inl"
#include "registeredModule.h"

#include "custom\mc_lightSources.h"

#include "..\advance\advanceContext.h"

#include "..\ai\aiMessage.h"

#include "..\collision\checkCollisionCache.h"

#include "..\debug\previewGame.h"

#include "..\game\gameUtils.h"

#include "..\interfaces\presenceObserver.h"

#include "..\modulesOwner\modulesOwnerImpl.inl"

#include "..\presence\relativeToPresencePlacement.h"

#include "..\world\doorInRoom.h"
#include "..\world\presenceLink.h"
#include "..\world\room.h"
#include "..\world\world.h"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\mesh\pose.h"
#include "..\..\core\types\names.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#ifdef ALLOW_DETAILED_DEBUG
		#define INSPECT_ATTACHMENTS
		#define INSPECT_PLACING_AT
	#endif
#endif

#ifdef AN_DEVELOPMENT
//#define FORCE_ALWAYS_REBUILD_PRESENCE_LINKS
#endif

//#define DEBUG_MOVEMENT_FOR_VR_ELEVATOR_ISSUES

//

using namespace Framework;

//

DEFINE_STATIC_NAME(teleports);
DEFINE_STATIC_NAME(moves);
DEFINE_STATIC_NAME(leavesRoom);
DEFINE_STATIC_NAME(entersRoom);
DEFINE_STATIC_NAME(who);
DEFINE_STATIC_NAME(reason);
DEFINE_STATIC_NAME(toRoom);
DEFINE_STATIC_NAME(throughDoor);
DEFINE_STATIC_NAME(floorLevel);
DEFINE_STATIC_NAME(floorLevelMatchOffsetLimit);
DEFINE_STATIC_NAME(floorLevelMatchOffsetLimitMin);
DEFINE_STATIC_NAME(floorLevelMatchOffsetLimitMax);
DEFINE_STATIC_NAME(canChangeRoom);
DEFINE_STATIC_NAME(cantChangeRoom);
DEFINE_STATIC_NAME(lockAsBase);
DEFINE_STATIC_NAME(canBeBase);
DEFINE_STATIC_NAME(baseThroughDoors);
DEFINE_STATIC_NAME(canBeBaseForPlayer);
DEFINE_STATIC_NAME(cantBeBase);
DEFINE_STATIC_NAME(cantBeBaseForPlayer);
DEFINE_STATIC_NAME(canBeBasedOn);
DEFINE_STATIC_NAME(canBeVolumetricBase);
DEFINE_STATIC_NAME(canBeBasedOnVolumetric);
DEFINE_STATIC_NAME(requiresUpdatePresence);
DEFINE_STATIC_NAME(doesntRequireUpdatePresence);
DEFINE_STATIC_NAME(immuneToKillGravity);
DEFINE_STATIC_NAME(vulnerableToKillGravity);
DEFINE_STATIC_NAME(volumetricBase);
DEFINE_STATIC_NAME(shouldAvoidGoingThroughCollision);
DEFINE_STATIC_NAME(shouldAvoidGoingThroughCollisionForSlidingLocomotion);
DEFINE_STATIC_NAME(shouldAvoidGoingThroughCollisionForVROnLockedBase);
DEFINE_STATIC_NAME(beAlwaysAtVRAnchorLevel);
DEFINE_STATIC_NAME(keepWithinSafeVolumeOnLockedBase);
DEFINE_STATIC_NAME(volumetricBaseFromAppearancesMesh);
DEFINE_STATIC_NAME(safeVolumeFromMeshNodes);
DEFINE_STATIC_NAME(safeVolumeFromVolumetricBase);
DEFINE_STATIC_NAME(safeVolumeExpandBy);
DEFINE_STATIC_NAME(safeVolumeExpandByX);
DEFINE_STATIC_NAME(safeVolumeExpandByY);
DEFINE_STATIC_NAME(safeVolumeExpandByZ);
DEFINE_STATIC_NAME(safeVolumeExpandByXY);
DEFINE_STATIC_NAME(updatePresenceLinkSetupContinuously);
DEFINE_STATIC_NAME(updatePresenceLinkSetupMakeBigger);
DEFINE_STATIC_NAME(updatePresenceLinkSetupFromMovementCollisionPrimitive);
DEFINE_STATIC_NAME(updatePresenceLinkSetupFromMovementCollisionPrimitiveAsSphere);
DEFINE_STATIC_NAME(updatePresenceLinkSetupFromMovementCollisionBoundingBox);
DEFINE_STATIC_NAME(updatePresenceLinkSetupFromPreciseCollisionBoundingBox);
DEFINE_STATIC_NAME(updatePresenceLinkSetupFromAppearancesBonesBoundingBox);
DEFINE_STATIC_NAME(moveUsingEyes);
DEFINE_STATIC_NAME(moveUsingEyesZOnly);
DEFINE_STATIC_NAME(ignoreImpulses);
DEFINE_STATIC_NAME(noVerticalMovement);
DEFINE_STATIC_NAME(alwaysRebuildPresenceLinks);
DEFINE_STATIC_NAME(presenceInSingleRoom);
DEFINE_STATIC_NAME(presenceLinkAccumulateDoorClipPlanes);
DEFINE_STATIC_NAME(presenceLinkClipLess);
DEFINE_STATIC_NAME(presenceLinkDoNotClip);
DEFINE_STATIC_NAME(presenceLinkAlwaysClipAgainstThroughDoor);
DEFINE_STATIC_NAME(presenceLinkRadius);
DEFINE_STATIC_NAME(presenceLinkRadiusClipLessPt);
DEFINE_STATIC_NAME(presenceLinkRadiusClipLessRadius);
DEFINE_STATIC_NAME(presenceLinkHeight);
DEFINE_STATIC_NAME(presenceLinkLength);
DEFINE_STATIC_NAME(presenceLinkCentreDistance);
DEFINE_STATIC_NAME(presenceLinkCentreDistanceX);
DEFINE_STATIC_NAME(presenceLinkCentreDistanceY);
DEFINE_STATIC_NAME(presenceLinkCentreDistanceZ);
DEFINE_STATIC_NAME(presenceLinkVerticalCentreDistance);
DEFINE_STATIC_NAME(presenceLinkCentreOffset);
DEFINE_STATIC_NAME(presenceLinkCentreOffsetX);
DEFINE_STATIC_NAME(presenceLinkCentreOffsetY);
DEFINE_STATIC_NAME(presenceLinkCentreOffsetZ);
DEFINE_STATIC_NAME(presenceLinkCentreOS);
DEFINE_STATIC_NAME(presenceLinkCentreOSX);
DEFINE_STATIC_NAME(presenceLinkCentreOSY);
DEFINE_STATIC_NAME(presenceLinkCentreOSZ);
DEFINE_STATIC_NAME(initialVelocityLinearOS);
DEFINE_STATIC_NAME(initialVelocityLinearOSX);
DEFINE_STATIC_NAME(initialVelocityLinearOSY);
DEFINE_STATIC_NAME(initialVelocityLinearOSZ);
DEFINE_STATIC_NAME(centreOfMass);
DEFINE_STATIC_NAME(centreOfMassX);
DEFINE_STATIC_NAME(centreOfMassY);
DEFINE_STATIC_NAME(centreOfMassZ);
DEFINE_STATIC_NAME(eyesLookRelativeAppearance);
DEFINE_STATIC_NAME(eyesLookRelativeBone);
DEFINE_STATIC_NAME(eyesLookRelativeSocket);
DEFINE_STATIC_NAME(eyesFixedLocation);
DEFINE_STATIC_NAME(ceaseToExistWhenNotAttached);
DEFINE_STATIC_NAME(useRoomVelocity);
DEFINE_STATIC_NAME(useRoomVelocityInTime);
DEFINE_STATIC_NAME(provideLinearSpeedVar);

// appearance name
DEFINE_STATIC_NAME_STR(presenceIndicator, TXT("presence indicator"));

// material params
DEFINE_STATIC_NAME(indicatorColour);

// mesh node names
DEFINE_STATIC_NAME(safeVolume);

//

#ifdef AN_DEVELOPMENT_OR_PROFILER
//#define DEBUG_BUILD_PRESENCE_LINKS
#endif

// detailed is continuous, normal is just when stuff changes
//#define DEBUG_PLAYER_BASE_DETAILED
//#define DEBUG_PLAYER_BASE

#ifdef DEBUG_ELEVATOR_MOVING_ALONE
#define DEBUG_PLAYER_BASE
#endif

//

Transform ModulePresence::Attachment::calculate_placement_of_attach(IModulesOwner* _owner, bool _calculateCurrentPlacement) const
{
	an_assert_immediate(attachedTo);
	Transform result = _calculateCurrentPlacement? attachedTo->get_presence()->calculate_current_placement() : attachedTo->get_presence()->placement;
	if (toSocket.is_valid() &&
		viaSocket.is_valid())
	{
		if (auto * appearance = attachedTo->get_appearance())
		{
			result = result.to_world(appearance->calculate_socket_os(toSocket.get_index()).make_non_zero_scale(makeZeroScaleValid? 1.0f : 0.0f));
			result = result.to_world(relativePlacement);
			if (auto* ownerAppearance = _owner->get_appearance())
			{
				result = result.to_world(ownerAppearance->calculate_socket_os(viaSocket.get_index()).make_non_zero_scale(makeZeroScaleValid ? 1.0f : 0.0f).inverted());
			}
		}
	}
	else if (toSocket.is_valid())
	{
		if (auto * appearance = attachedTo->get_appearance())
		{
			result = result.to_world(appearance->calculate_socket_os(toSocket.get_index()));
			result = result.to_world(relativePlacement);
		}
	}
	else if (toBoneIdx != NONE)
	{
		if (auto * appearance = attachedTo->get_appearance())
		{
			if (Meshes::Pose const * finalPoseLS = appearance->get_final_pose_LS())
			{
				if (false)
				{
					// if we have zero scale bones, pretend we're good with them
					int nowBoneIdx = toBoneIdx;
					Transform skipNonScaleBones = Transform::identity;
					if (auto* fs = appearance->get_skeleton())
					{
						if (auto* as = fs->get_skeleton())
						{
							while (finalPoseLS->get_bone(nowBoneIdx).get_scale() == 0.0f)
							{
								nowBoneIdx = as->get_parent_of(nowBoneIdx);
								Transform pretendBoneLS = finalPoseLS->get_bone(nowBoneIdx);
								pretendBoneLS.make_non_zero_scale(makeZeroScaleValid ? 1.0f : 0.0f);
								skipNonScaleBones = skipNonScaleBones.to_world(pretendBoneLS);
							}
						}
					}
					result = result.to_world(finalPoseLS->get_bone_ms_from_ls(nowBoneIdx)).to_world(skipNonScaleBones);
				}
				else
				{
					result = result.to_world(finalPoseLS->get_bone_ms_from_ls(toBoneIdx));
				}
			}
		}
		result = result.to_world(relativePlacement);
	}
	else
	{
		result = result.to_world(relativePlacement);
	}
	return result.make_non_zero_scale(makeZeroScaleValid ? 1.0f : 0.0f);
}

//--

Transform ModulePresence::BasedOn::transform_from_owner_to_base(Transform const& _ws) const
{
	if (pathToBase.is_active())
	{
		return pathToBase.from_owner_to_target(_ws);
	}
	else
	{
		return _ws;
	}
}

Transform ModulePresence::BasedOn::transform_from_base_to_owner(Transform const& _ws) const
{
	if (pathToBase.is_active())
	{
		return pathToBase.from_target_to_owner(_ws);
	}
	else
	{
		return _ws;
	}
}

void ModulePresence::BasedOn::set(IModulesOwner* _basedOn, Transform const & _placementWhenStartedBasingOn)
{
	placementWhenStartedBasingOn = _placementWhenStartedBasingOn; // always update

	if (_basedOn == basedOn)
	{
		return;
	}

	Concurrency::ScopedSpinLock lock(ownerPresence->basedLock);

	if (basedOn)
	{
		an_assert_immediate(presence == basedOn->get_presence());
		presence->remove_presence_observer(this);
		presence->remove_based(owner);
		pathToBase.reset();
	}

#ifdef DEBUG_PLAYER_BASE
	if (GameUtils::is_player(owner))
	{
		output(TXT("[PLAYER'S PRESENCE][CHANGE] change base <%S> to <%S>"),
			basedOn? basedOn->ai_get_name().to_char() : TXT(">none<"),
			_basedOn ? _basedOn->ai_get_name().to_char() : TXT(">none<"));
		output(TXT("[PLAYER'S PRESENCE][CHANGE] player at %S"),
			owner && owner->get_presence()? owner->get_presence()->get_placement().get_translation().to_string().to_char() : TXT("--"));
	}
#endif

	basedOn = _basedOn;
	presence = basedOn ? basedOn->get_presence() : nullptr;
	basedOnThroughDoors = false;

	if (basedOn)
	{
		an_assert_immediate(presence);
		presence->add_presence_observer(this);
		presence->add_based(owner);

		basedOnThroughDoors = basedOn->get_presence()->is_base_through_doors();
		if (basedOnThroughDoors)
		{
			pathToBase.find_path(owner, basedOn, true);
		}
	}
}

void ModulePresence::BasedOn::on_presence_changed_room(ModulePresence* _presence, Room* _intoRoom, Transform const& _intoRoomTransform, DoorInRoom* _exitThrough, DoorInRoom* _enterThrough, DoorInRoomArrayPtr const* _throughDoors)
{
	if (!basedOnThroughDoors)
	{
		clear();
	}
}

//--

int ModulePresence::s_ignoreAutoVelocities = 0;

#ifdef WITH_PRESENCE_INDICATOR
bool ModulePresence::showPresenceIndicator = false;
#endif

REGISTER_FOR_FAST_CAST(ModulePresence);

static ModulePresence* create_module(IModulesOwner* _owner)
{
	return new ModulePresence(_owner);
}

RegisteredModule<ModulePresence> & ModulePresence::register_itself()
{
	return Modules::presence.register_module(String(TXT("base")), create_module);
}

ModulePresence::ModulePresence(IModulesOwner* _owner)
: base(_owner)
, basedOn(_owner, this)
{
	requestedRelativeHands[0] = Transform::identity;
	requestedRelativeHands[1] = Transform::identity;
	requestedRelativeHandsAccurate[0] = false;
	requestedRelativeHandsAccurate[1] = false;
}

ModulePresence::~ModulePresence()
{
	ASSERT_SYNC_OR(! get_in_world() || get_in_world()->is_being_destroyed());
	an_assert_immediate(!inRoom);
	if (presenceLinks)
	{
		an_assert_log_always(presenceLinks->prevInObject == nullptr, TXT("presenceLinks should be first, always"));
		presenceLinks->invalidate_object();
		presenceLinks = nullptr;
	}
	if (attachment.attachedTo)
	{
		detach();
	}
	detach_attached();
	drop_based();
	while (! observers.is_empty())
	{
		Concurrency::ScopedSpinLock lock(observers.get_first()->access_presence_observer_lock());
		observers.get_first()->on_presence_destroy(this);
	}
}

void ModulePresence::detach_attached()
{
	while (true)
	{
		IModulesOwner* attachedImo = nullptr;
		{
			Concurrency::ScopedSpinLock lock(attachedLock);
			attachedImo = ! attached.is_empty()? attached.get_first() : nullptr;
		}
		if (attachedImo)
		{
			attachedImo->get_presence()->detach();
		}
		else
		{
			break;
		}
	}
}

void ModulePresence::reset()
{
	base::reset();
	placement = Transform::identity;
	prevPlacementOffset = Transform::identity;
	prevPlacementOffsetDeltaTime = 0.0f;
	velocityLinear = Vector3::zero;
	useInitialVelocityLinearOS = initialVelocityLinearOS;
	velocityRotation = Rotator3::zero;
	nextVelocityLinear = Vector3::zero;
	nextVelocityRotation = Rotator3::zero;
	nextLocationDisplacement = Vector3::zero;
	nextOrientationDisplacement = Quat::identity;
	prevVelocities.clear();
	post_placement_change();

	memory_zero(doesRequireBuildingPresenceLinks, sizeof(bool) * PresenceLinkOperation::COUNT);
}

void ModulePresence::setup_with(ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		modulePresenceData = fast_cast<ModulePresenceData>(_moduleData);
		shouldAvoidGoingThroughCollision = _moduleData->get_parameter<bool>(this, NAME(shouldAvoidGoingThroughCollision), shouldAvoidGoingThroughCollision);
		shouldAvoidGoingThroughCollisionForSlidingLocomotion = _moduleData->get_parameter<bool>(this, NAME(shouldAvoidGoingThroughCollisionForSlidingLocomotion), shouldAvoidGoingThroughCollisionForSlidingLocomotion);
		shouldAvoidGoingThroughCollisionForVROnLockedBase = _moduleData->get_parameter<bool>(this, NAME(shouldAvoidGoingThroughCollisionForVROnLockedBase), shouldAvoidGoingThroughCollisionForVROnLockedBase);
		beAlwaysAtVRAnchorLevel = _moduleData->get_parameter<bool>(this, NAME(beAlwaysAtVRAnchorLevel), beAlwaysAtVRAnchorLevel);
		keepWithinSafeVolumeOnLockedBase = _moduleData->get_parameter<bool>(this, NAME(keepWithinSafeVolumeOnLockedBase), keepWithinSafeVolumeOnLockedBase);
		requiresUpdatePresence = _moduleData->get_parameter<bool>(this, NAME(requiresUpdatePresence), requiresUpdatePresence);
		requiresUpdatePresence = ! _moduleData->get_parameter<bool>(this, NAME(doesntRequireUpdatePresence), ! requiresUpdatePresence);
		immuneToKillGravity = _moduleData->get_parameter<bool>(this, NAME(immuneToKillGravity), immuneToKillGravity);
		{
			bool vulnerableToKillGravity = !immuneToKillGravity;
			vulnerableToKillGravity = _moduleData->get_parameter<bool>(this, NAME(vulnerableToKillGravity), vulnerableToKillGravity);
			immuneToKillGravity = !vulnerableToKillGravity;
		}
		floorLevel = _moduleData->get_parameter<float>(this, NAME(floorLevel), floorLevel);
		floorLevelMatchOffsetLimit.min = _moduleData->get_parameter<float>(this, NAME(floorLevelMatchOffsetLimitMin), floorLevelMatchOffsetLimit.min);
		floorLevelMatchOffsetLimit.max = _moduleData->get_parameter<float>(this, NAME(floorLevelMatchOffsetLimitMax), floorLevelMatchOffsetLimit.max);
		floorLevelMatchOffsetLimit = _moduleData->get_parameter<Range>(this, NAME(floorLevelMatchOffsetLimit), floorLevelMatchOffsetLimit);
		canChangeRoom = _moduleData->get_parameter<bool>(this, NAME(canChangeRoom), canChangeRoom);
		canChangeRoom = ! _moduleData->get_parameter<bool>(this, NAME(cantChangeRoom), ! canChangeRoom);
		lockAsBase = _moduleData->get_parameter<bool>(this, NAME(lockAsBase), lockAsBase);
		canBeBase = _moduleData->get_parameter<bool>(this, NAME(canBeBase), canBeBase);
		canBeBase = ! _moduleData->get_parameter<bool>(this, NAME(cantBeBase), !canBeBase);
		baseThroughDoors = _moduleData->get_parameter<bool>(this, NAME(baseThroughDoors), baseThroughDoors);
		if (canBeBase && canChangeRoom)
		{
			if (baseThroughDoors)
			{
				// ok
			}
			else
			{
				error(TXT("cannot be base and change room (if you want to have that, add baseThroughDoors) \"%S\""), get_owner()->get_library_name().to_string().to_char());
			}
		}
		canBeBaseForPlayer = canBeBase;
		canBeBaseForPlayer = _moduleData->get_parameter<bool>(this, NAME(canBeBaseForPlayer), canBeBaseForPlayer);
		canBeBaseForPlayer = !_moduleData->get_parameter<bool>(this, NAME(cantBeBaseForPlayer), !canBeBaseForPlayer);
		canBeBasedOn = _moduleData->get_parameter<bool>(this, NAME(canBeBasedOn), canBeBasedOn);
		canBeVolumetricBase = _moduleData->get_parameter<bool>(this, NAME(canBeVolumetricBase), canBeVolumetricBase);
		canBeBasedOnVolumetric = _moduleData->get_parameter<bool>(this, NAME(canBeBasedOnVolumetric), canBeBasedOnVolumetric);
		{
			Range3 vb = Range3::empty;
			if (volumetricBaseProvided.is_set())
			{
				vb = volumetricBaseProvided.get();
			}
			vb = _moduleData->get_parameter<Range3>(this, NAME(volumetricBase), vb);
			if (vb.has_something())
			{
				volumetricBaseProvided = vb;
			}
		}
		volumetricBaseFromAppearancesMesh = _moduleData->get_parameter<bool>(this, NAME(volumetricBaseFromAppearancesMesh), volumetricBaseFromAppearancesMesh);
		safeVolumeFromMeshNodes = _moduleData->get_parameter<bool>(this, NAME(safeVolumeFromMeshNodes), safeVolumeFromMeshNodes);
		safeVolumeFromVolumetricBase = _moduleData->get_parameter<bool>(this, NAME(safeVolumeFromVolumetricBase), safeVolumeFromVolumetricBase);
		{
			safeVolumeExpandBy.x = _moduleData->get_parameter<float>(this, NAME(safeVolumeExpandByX), safeVolumeExpandBy.x);
			safeVolumeExpandBy.y = _moduleData->get_parameter<float>(this, NAME(safeVolumeExpandByY), safeVolumeExpandBy.y);
			safeVolumeExpandBy.z = _moduleData->get_parameter<float>(this, NAME(safeVolumeExpandByZ), safeVolumeExpandBy.z);
			safeVolumeExpandBy.x = _moduleData->get_parameter<float>(this, NAME(safeVolumeExpandByXY), safeVolumeExpandBy.x);
			safeVolumeExpandBy.y = _moduleData->get_parameter<float>(this, NAME(safeVolumeExpandByXY), safeVolumeExpandBy.y);
			safeVolumeExpandBy = _moduleData->get_parameter<Vector3>(this, NAME(safeVolumeExpandBy), safeVolumeExpandBy);			
		}
		updatePresenceLinkSetupMakeBigger = _moduleData->get_parameter<float>(this, NAME(updatePresenceLinkSetupMakeBigger), updatePresenceLinkSetupMakeBigger);
		updatePresenceLinkSetupFromMovementCollisionPrimitive = _moduleData->get_parameter<bool>(this, NAME(updatePresenceLinkSetupFromMovementCollisionPrimitive), updatePresenceLinkSetupFromMovementCollisionPrimitive);
		updatePresenceLinkSetupFromMovementCollisionPrimitiveAsSphere = _moduleData->get_parameter<bool>(this, NAME(updatePresenceLinkSetupFromMovementCollisionPrimitiveAsSphere), updatePresenceLinkSetupFromMovementCollisionPrimitiveAsSphere);
		updatePresenceLinkSetupFromMovementCollisionBoundingBox = _moduleData->get_parameter<bool>(this, NAME(updatePresenceLinkSetupFromMovementCollisionBoundingBox), updatePresenceLinkSetupFromMovementCollisionBoundingBox);
		updatePresenceLinkSetupFromPreciseCollisionBoundingBox = _moduleData->get_parameter<bool>(this, NAME(updatePresenceLinkSetupFromPreciseCollisionBoundingBox), updatePresenceLinkSetupFromPreciseCollisionBoundingBox);
		updatePresenceLinkSetupFromAppearancesBonesBoundingBox = _moduleData->get_parameter<bool>(this, NAME(updatePresenceLinkSetupFromAppearancesBonesBoundingBox), updatePresenceLinkSetupFromAppearancesBonesBoundingBox);
		updatePresenceLinkSetupContinuously |= updatePresenceLinkSetupFromAppearancesBonesBoundingBox;
		updatePresenceLinkSetupContinuously = _moduleData->get_parameter<bool>(this, NAME(updatePresenceLinkSetupContinuously), updatePresenceLinkSetupContinuously);
		moveUsingEyes = _moduleData->get_parameter<bool>(this, NAME(moveUsingEyes), moveUsingEyes);
		moveUsingEyesZOnly = _moduleData->get_parameter<bool>(this, NAME(moveUsingEyesZOnly), moveUsingEyesZOnly);
		ignoreImpulses = _moduleData->get_parameter<bool>(this, NAME(ignoreImpulses), ignoreImpulses);
		noVerticalMovement = _moduleData->get_parameter<bool>(this, NAME(noVerticalMovement), noVerticalMovement);
		alwaysRebuildPresenceLinks = _moduleData->get_parameter<bool>(this, NAME(alwaysRebuildPresenceLinks), alwaysRebuildPresenceLinks);
		presenceInSingleRoom = _moduleData->get_parameter<bool>(this, NAME(presenceInSingleRoom), presenceInSingleRoom);
		presenceInSingleRoomTO = false;
		if (presenceInSingleRoom && fast_cast<TemporaryObject>(get_owner()))
		{
			// otherwise we will use excess build but for temporary objects it doesn't make sense
			presenceInSingleRoomTO = true;
		}
		presenceLinkAccumulateDoorClipPlanes = _moduleData->get_parameter<bool>(this, NAME(presenceLinkAccumulateDoorClipPlanes), presenceLinkAccumulateDoorClipPlanes);
		if (get_owner()->get_as_actor())
		{
			// actors by default clip less
			presenceLinkClipLess = true;
		}
		presenceLinkClipLess = _moduleData->get_parameter<bool>(this, NAME(presenceLinkClipLess), presenceLinkClipLess);
		presenceLinkDoNotClip = _moduleData->get_parameter<bool>(this, NAME(presenceLinkDoNotClip), presenceLinkDoNotClip);
		presenceLinkAlwaysClipAgainstThroughDoor = _moduleData->get_parameter<bool>(this, NAME(presenceLinkAlwaysClipAgainstThroughDoor), presenceLinkAlwaysClipAgainstThroughDoor);
		presenceLinkRadius = _moduleData->get_parameter<float>(this, NAME(presenceLinkRadius), presenceLinkRadius);
		presenceLinkRadiusClipLessPt = _moduleData->get_parameter<float>(this, NAME(presenceLinkRadiusClipLessPt), presenceLinkRadiusClipLessPt);
		presenceLinkRadiusClipLessRadius = _moduleData->get_parameter<float>(this, NAME(presenceLinkRadiusClipLessRadius), presenceLinkRadiusClipLessRadius);
		presenceLinkCentreDistance.z = max(0.0f, _moduleData->get_parameter<float>(this, NAME(presenceLinkHeight), presenceLinkCentreDistance.z + presenceLinkRadius * 2.0f) - presenceLinkRadius * 2.0f);
		presenceLinkCentreDistance.z = max(0.0f, _moduleData->get_parameter<float>(this, NAME(presenceLinkVerticalCentreDistance), presenceLinkCentreDistance.z));
		presenceLinkCentreDistance.y = max(0.0f, _moduleData->get_parameter<float>(this, NAME(presenceLinkLength), presenceLinkCentreDistance.y + presenceLinkRadius * 2.0f) - presenceLinkRadius * 2.0f);
		{
			presenceLinkCentreDistance.x = _moduleData->get_parameter<float>(this, NAME(presenceLinkCentreDistanceX), presenceLinkCentreDistance.x);
			presenceLinkCentreDistance.y = _moduleData->get_parameter<float>(this, NAME(presenceLinkCentreDistanceY), presenceLinkCentreDistance.y);
			presenceLinkCentreDistance.z = _moduleData->get_parameter<float>(this, NAME(presenceLinkCentreDistanceZ), presenceLinkCentreDistance.z);
			presenceLinkCentreDistance = _moduleData->get_parameter<Vector3>(this, NAME(presenceLinkCentreDistance), presenceLinkCentreDistance);
		}
		{
			Vector3 tempPresenceLinkCentreOS = presenceLinkCentreOS.get_translation();
			tempPresenceLinkCentreOS.x = _moduleData->get_parameter<float>(this, NAME(presenceLinkCentreOffsetX), tempPresenceLinkCentreOS.x);
			tempPresenceLinkCentreOS.y = _moduleData->get_parameter<float>(this, NAME(presenceLinkCentreOffsetY), tempPresenceLinkCentreOS.y);
			tempPresenceLinkCentreOS.z = _moduleData->get_parameter<float>(this, NAME(presenceLinkCentreOffsetZ), tempPresenceLinkCentreOS.z);
			tempPresenceLinkCentreOS = _moduleData->get_parameter<Vector3>(this, NAME(presenceLinkCentreOffset), tempPresenceLinkCentreOS);
			tempPresenceLinkCentreOS.x = _moduleData->get_parameter<float>(this, NAME(presenceLinkCentreOSX), tempPresenceLinkCentreOS.x);
			tempPresenceLinkCentreOS.y = _moduleData->get_parameter<float>(this, NAME(presenceLinkCentreOSY), tempPresenceLinkCentreOS.y);
			tempPresenceLinkCentreOS.z = _moduleData->get_parameter<float>(this, NAME(presenceLinkCentreOSZ), tempPresenceLinkCentreOS.z);
			tempPresenceLinkCentreOS = _moduleData->get_parameter<Vector3>(this, NAME(presenceLinkCentreOS), tempPresenceLinkCentreOS);
			presenceLinkCentreOS.set_translation(tempPresenceLinkCentreOS);
		}
		{
			Vector3 tempInitialVelocityLinearOS = initialVelocityLinearOS.get(Vector3::zero);
			tempInitialVelocityLinearOS.x = _moduleData->get_parameter<float>(this, NAME(initialVelocityLinearOSX), tempInitialVelocityLinearOS.x);
			tempInitialVelocityLinearOS.y = _moduleData->get_parameter<float>(this, NAME(initialVelocityLinearOSY), tempInitialVelocityLinearOS.y);
			tempInitialVelocityLinearOS.z = _moduleData->get_parameter<float>(this, NAME(initialVelocityLinearOSZ), tempInitialVelocityLinearOS.z);
			tempInitialVelocityLinearOS = _moduleData->get_parameter<Vector3>(this, NAME(initialVelocityLinearOS), tempInitialVelocityLinearOS);
			if (!tempInitialVelocityLinearOS.is_zero())
			{
				initialVelocityLinearOS = tempInitialVelocityLinearOS;
				useInitialVelocityLinearOS = initialVelocityLinearOS;
			}
		}
		{
			centreOfMass.x = _moduleData->get_parameter<float>(this, NAME(centreOfMassX), centreOfMass.x);
			centreOfMass.y = _moduleData->get_parameter<float>(this, NAME(centreOfMassY), centreOfMass.y);
			centreOfMass.z = _moduleData->get_parameter<float>(this, NAME(centreOfMassZ), centreOfMass.z);
			centreOfMass = _moduleData->get_parameter<Vector3>(this, NAME(centreOfMass), centreOfMass);
		}
		useRoomVelocity = _moduleData->get_parameter<float>(this, NAME(useRoomVelocity), useRoomVelocity);
		useRoomVelocityInTime = _moduleData->get_parameter<float>(this, NAME(useRoomVelocityInTime), useRoomVelocityInTime);
		eyesLookRelativeAppearance = _moduleData->get_parameter<Name>(this, NAME(eyesLookRelativeAppearance), eyesLookRelativeAppearance);
		eyesLookRelativeBone = _moduleData->get_parameter<Name>(this, NAME(eyesLookRelativeBone), eyesLookRelativeBone.get_name());
		eyesLookRelativeSocket = _moduleData->get_parameter<Name>(this, NAME(eyesLookRelativeSocket), eyesLookRelativeSocket.get_name());
		eyesFixedLocation = _moduleData->get_parameter<bool>(this, NAME(eyesFixedLocation), eyesFixedLocation);
		bool ceaseToExistWhenNotAttachedHelper = is_flag_set(ceaseToExistWhenNotAttached, CeaseToExistWhenNotAttachedFlag::Param);
		ceaseToExistWhenNotAttachedHelper = _moduleData->get_parameter<bool>(this, NAME(ceaseToExistWhenNotAttached), ceaseToExistWhenNotAttachedHelper);
		cease_to_exist_when_not_attached(ceaseToExistWhenNotAttachedHelper, CeaseToExistWhenNotAttachedFlag::Param);

		provideLinearSpeedVar = _moduleData->get_parameter<Name>(this, NAME(provideLinearSpeedVar), provideLinearSpeedVar.get_name());
		provideLinearSpeedVar.look_up<float>(get_owner()->access_variables());

		CACHED_ updatePresenceLinkSetup = false
			| updatePresenceLinkSetupFromMovementCollisionPrimitive
			| updatePresenceLinkSetupFromMovementCollisionPrimitiveAsSphere
			| updatePresenceLinkSetupFromMovementCollisionBoundingBox
			| updatePresenceLinkSetupFromPreciseCollisionBoundingBox
			| updatePresenceLinkSetupFromAppearancesBonesBoundingBox
			;

		get_owner()->update_presence_cached();
	}
}

void ModulePresence::place_in_room(Room* _inRoom, Vector3 const & _location, Name const & _reason)
{
	if (_location.is_zero())
	{
		// place in the middle of the room
		//location.y = _inRoom.actualSize.depth * 0.5f;
	}
	place_in_room(_inRoom, Transform(_location, placement.get_orientation()), _reason);
}

void ModulePresence::place_in_room(Room* _inRoom, Transform const & _placement, Name const & _reason)
{
	an_assert_immediate(_inRoom != nullptr);
	add_to_room(_inRoom, nullptr, _reason);
	placement = _placement;
	an_assert_immediate(placement.is_ok());
	an_assert_immediate(placement.get_scale() != 0.0f);
	placement.set_translation(placement.get_translation() - placement.get_axis(Axis::Z) * floorLevel);
	nextPlacement = placement;
	an_assert_immediate(nextPlacement.is_ok());
	an_assert_immediate(nextPlacement.get_scale() != 0.0f);
	prevPlacementOffset = Transform::identity;
	prevPlacementOffsetDeltaTime = 0.0f;
	update_vr_anchor_from_room();
	post_placement_change();
	post_placed();
}

void ModulePresence::place_within_room(Transform const & _placement)
{
	placement = _placement;
	an_assert_immediate(placement.is_ok());
	an_assert_immediate(placement.get_scale() != 0.0f);
	placement.set_translation(placement.get_translation() - placement.get_axis(Axis::Z) * floorLevel);
	nextPlacement = placement;
	an_assert_immediate(nextPlacement.is_ok());
	an_assert_immediate(nextPlacement.get_scale() != 0.0f);
	prevPlacementOffset = Transform::identity;
	prevPlacementOffsetDeltaTime = 0.0f;
	post_placement_change();
	post_placed(); // although it should already be called, we should already be in a room
}

Transform ModulePresence::place_at(IModulesOwner const * _object, Name const & _socketName, Optional<Transform> const & _relativePlacement, Optional<Vector3> const& _absoluteLocationOffset)
{
#ifdef INSPECT_PLACING_AT
	if (!fast_cast<TemporaryObject>(get_owner()))
	{
		output(TXT("[presence placement] place o%p at o%p's socket \"%S\""), get_owner(), _object, _socketName.to_char());
	}
#endif
	bool wasPlaced = inRoom != nullptr;
	Transform offset = Transform::identity;
	if (ModulePresence* objectsPresence = _object->get_presence())
	{
		an_assert(objectsPresence->get_in_room());
		add_to_room(objectsPresence->get_in_room());
		placement = objectsPresence->placement;
		an_assert_immediate(placement.is_ok());
		an_assert_immediate(placement.get_scale() != 0.0f);
		nextPlacement = objectsPresence->placement;
		an_assert_immediate(nextPlacement.is_ok());
		an_assert_immediate(nextPlacement.get_scale() != 0.0f);
		prevPlacementOffset = Transform::identity;
		prevPlacementOffsetDeltaTime = 0.0f;
		velocityLinear = Vector3::zero;
		velocityRotation = Rotator3::zero;
		nextVelocityLinear = Vector3::zero;
		nextVelocityRotation = Rotator3::zero;
		prevVelocities.clear();
		{
			Transform offsetPlacement = _relativePlacement.get(Transform::identity);
			if (_socketName.is_valid())
			{
				if (auto const * appearance = _object->get_appearance())
				{
					offsetPlacement = offsetPlacement.to_world(appearance->calculate_socket_os(_socketName));
				}
			}
			{
				nextPlacement = placement.to_world(offsetPlacement);
				if (_absoluteLocationOffset.is_set())
				{
					nextPlacement.set_translation(nextPlacement.get_translation() + _absoluteLocationOffset.get());
				}
				an_assert_immediate(nextPlacement.is_ok());
				an_assert_immediate(nextPlacement.get_scale() != 0.0f);

#ifdef INSPECT_PLACING_AT
				if (!fast_cast<TemporaryObject>(get_owner()))
				{
					output(TXT("[presence placement] place o%p at o%p in room r%p, next placement [%S][%S]"), get_owner(), _object, inRoom, nextPlacement.get_translation().to_string().to_char(), nextPlacement.get_orientation().to_rotator().to_string().to_char());
				}
#endif
				Room * intoRoom;
				Transform intoRoomTransform;
				DoorInRoom * exitThrough;
				DoorInRoom * enterThrough;
				int const throughDoorsLimit = 32;
				allocate_stack_var(Framework::DoorInRoom const*, throughDoors, throughDoorsLimit);
				Framework::DoorInRoomArrayPtr throughDoorsArrayPtr(throughDoors, throughDoorsLimit);

				// don't forget about presence centre offset!
				Transform placePlacement = placement;
				Transform nextPlacePlacement = nextPlacement;
				placePlacement.set_translation(placePlacement.get_translation() + placePlacement.vector_to_world(objectsPresence->presenceLinkCentreOS.get_translation())); // place at object, use it's centre
				nextPlacePlacement.set_translation(nextPlacePlacement.get_translation() + nextPlacePlacement.vector_to_world(presenceLinkCentreOS.get_translation())); // use our centre

				if (can_change_rooms() && inRoom->move_through_doors(placePlacement, nextPlacePlacement, OUT_ intoRoom, OUT_ intoRoomTransform, OUT_ &exitThrough, OUT_ &enterThrough, OUT_ &throughDoorsArrayPtr))
				{
#ifdef INSPECT_PLACING_AT
					if (!fast_cast<TemporaryObject>(get_owner()))
					{
						output(TXT("[presence placement] place o%p through door"), get_owner());
					}
#endif
					internal_change_room(intoRoom, intoRoomTransform, exitThrough, enterThrough, &throughDoorsArrayPtr);
					offset = intoRoomTransform;
				}

				// so we will always have orientation normalised
				nextPlacement.normalise_orientation();

				// update placement to next
				placement = nextPlacement;
				an_assert_immediate(placement.is_ok());
				an_assert_immediate(placement.get_scale() != 0.0f);

#ifdef INSPECT_PLACING_AT
				if (!fast_cast<TemporaryObject>(get_owner()))
				{
					output(TXT("[presence placement] placed o%p at o%p in room r%p, placement [%S][%S]"), get_owner(), _object, inRoom, placement.get_translation().to_string().to_char(), placement.get_orientation().to_rotator().to_string().to_char());
				}
#endif
			}
		}
		update_vr_anchor_from_room();
		post_placement_change();
		post_placed();
	}
	if (inRoom && !wasPlaced)
	{
		for_every_ptr(custom, get_owner()->get_customs())
		{
			custom->on_placed_in_world();
		}
	}
	return offset;
}

bool ModulePresence::get_placement_to_place_at(IModulesOwner const * _object, Name const & _socketName, Optional<Transform> const & _relativePlacement, OUT_ Room*& _inRoom, OUT_ Transform & _placement) const
{
	if (ModulePresence* objectsPresence = _object->get_presence())
	{
		todo_note(TXT("maybe combine this into one method that just has lots of output params?"));
		_inRoom = objectsPresence->get_in_room();
		_placement = objectsPresence->get_placement();
		an_assert_immediate(_placement.get_scale() != 0.0f);
		{
			Transform offsetPlacement = _relativePlacement.get(Transform::identity);
			if (_socketName.is_valid())
			{
				if (auto const * appearance = _object->get_appearance())
				{
					offsetPlacement = offsetPlacement.to_world(appearance->calculate_socket_os(_socketName));
				}
			}
			{
				Transform nextPlacement = _placement.to_world(offsetPlacement);
				Room * intoRoom;
				Transform intoRoomTransform;
				DoorInRoom * exitThrough;
				DoorInRoom * enterThrough;
				int const throughDoorsLimit = 32;
				allocate_stack_var(Framework::DoorInRoom const*, throughDoors, throughDoorsLimit);
				Framework::DoorInRoomArrayPtr throughDoorsArrayPtr(throughDoors, throughDoorsLimit);

				// don't forget about presence centre offset!
				Transform placePlacement = _placement;
				Transform nextPlacePlacement = nextPlacement;
				placePlacement.set_translation(placePlacement.get_translation() + placePlacement.vector_to_world(objectsPresence->presenceLinkCentreOS.get_translation())); // place at object, use it's centre
				nextPlacePlacement.set_translation(nextPlacePlacement.get_translation() + nextPlacePlacement.vector_to_world(presenceLinkCentreOS.get_translation())); // use our centre

				if (can_change_rooms() && _inRoom->move_through_doors(placePlacement, nextPlacePlacement, OUT_ intoRoom, OUT_ intoRoomTransform, OUT_ &exitThrough, OUT_ &enterThrough, OUT_ &throughDoorsArrayPtr))
				{
					_inRoom = intoRoom;
					nextPlacement = intoRoomTransform.to_local(nextPlacement);
				}

				// so we will always have orientation normalised
				nextPlacement.normalise_orientation();

				// update placement to next
				_placement = nextPlacement;
				an_assert_immediate(_placement.get_scale() != 0.0f);
			}
		}
		return true;
	}
	return false;
}

void ModulePresence::add_to_room(Room* _room, DoorInRoom * _enteredThroughDoor, Name const & _reason)
{
	scoped_call_stack_info(TXT("add to room"));
	if (inRoom != nullptr)
	{
		ai_create_message_leaves_room(inRoom, nullptr, _reason);
	}
	internal_add_to_room(_room);
	ai_create_message_enters_room(_enteredThroughDoor, _reason);

	// inform other modules
	if (IModulesOwner * owner = get_owner())
	{
		if (ModuleAI* ai = owner->get_ai())
		{
			ai->internal_change_room(nullptr);
		}
	}

	// inform observers (use copy as we may alter array)
	observersLock.acquire(TXT("ModulePresence::add_to_room"));
	ARRAY_STACK(IPresenceObserver*, observersCopy, observers.get_size());
	observersCopy = observers;
	observersLock.release();
	for_every_ptr(observer, observersCopy)
	{
		Concurrency::ScopedSpinLock lock(observer->access_presence_observer_lock());
		observer->on_presence_added_to_room(this, _room, _enteredThroughDoor);
	}
}

void ModulePresence::remove_from_room(Name const & _reason)
{
	Room* wasInRoom = inRoom;

	ai_create_message_leaves_room(nullptr, nullptr, _reason);
	internal_remove_from_room();

	// inform other modules
	if (IModulesOwner * owner = get_owner())
	{
		if (ModuleAI* ai = owner->get_ai())
		{
			ai->internal_change_room(nullptr);
		}
	}

	// inform observers (use copy as we may alter array)
	observersLock.acquire(TXT("ModulePresence::remove_from_room"));
	ARRAY_STACK(IPresenceObserver*, observersCopy, observers.get_size());
	observersCopy = observers;
	observersLock.release();
	for_every_ptr(observer, observersCopy)
	{
		Concurrency::ScopedSpinLock lock(observer->access_presence_observer_lock());
		observer->on_presence_removed_from_room(this, wasInRoom);
	}
}

void ModulePresence::internal_add_to_room(Room* _room)
{
	scoped_call_stack_info(TXT("internal add to room"));
	an_assert_immediate(_room != nullptr);
	internal_remove_from_room();
    inRoom = _room;
	useRoomVelocityTime = 0.0f;
	if (inRoom)
	{
		inRoom->add_object(get_owner());
		get_owner()->set_room_distance_to_recently_seen_by_player(AVOID_CALLING_ACK_ inRoom->get_distance_to_recently_seen_by_player()); // will be updated by room
	}
}

void ModulePresence::internal_remove_from_room()
{
    if (inRoom)
    {
		scoped_call_stack_info_str_printf(TXT("internal remove from room:%p"), inRoom);
		inRoom->remove_object(get_owner());
    }
    inRoom = nullptr;
	useRoomVelocityTime = 0.0f;
	
	an_assert(!basedOn.get_presence() || !basedOn.get_presence()->is_locked_as_base(this) ||
		!get_in_world() || get_in_world()->is_being_destroyed() || basedOn.is_based_through_doors(), TXT("dropping base at this moment may lead to issues, maybe there should be no such door"));
	if (!basedOn.is_based_through_doors())
	{
		basedOn.clear(); // based on works only within room
		basedOnDoneAtLoc.clear();
	}
	if (!is_base_through_doors())
	{
		drop_based(); // as above
	}
}

void ModulePresence::quick_pre_update_presence()
{
	scoped_call_stack_ptr(get_owner());
	collides = false;
	if (ModuleCollision const* collision = get_owner()->get_collision())
	{
		collides = !collision->get_gradient().is_zero();
	}

#ifdef WITH_PRESENCE_INDICATOR
	if (presenceIndicatorUpdatedFor ^ ModulePresence::showPresenceIndicator)
	{
		presenceIndicatorUpdatedFor = ModulePresence::showPresenceIndicator;
		if (auto* a = get_owner()->get_appearance_named(NAME(presenceIndicator)))
		{
			if (a->get_name() == NAME(presenceIndicator))
			{
				a->be_visible(presenceIndicatorUpdatedFor);
				presenceIndicatorVisible = presenceIndicatorUpdatedFor;
				update_presence_indicator();
			}
		}
	}
#endif
}

void ModulePresence::advance__update_presence(IMMEDIATE_JOB_PARAMS)
{
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		MEASURE_PERFORMANCE(updatePresence);
		if (ModulePresence* self = modulesOwner->get_presence())
		{
			self->quick_pre_update_presence();

			// update gravity
			if (self->get_in_room())
			{
				self->update_gravity();
				self->update_based_on();
			}
		}
		else
		{
			an_assert(false, TXT("shouldn't be called"));
		}
	}
}

void ModulePresence::force_base_on(IModulesOwner* _onto)
{
#ifdef DEBUG_PLAYER_BASE
	if (GameUtils::is_player(get_owner()))
	{
		output(TXT("[PLAYER'S PRESENCE] force base on <%S>"), _onto? _onto->ai_get_name().to_char() : TXT(">none<"));
	}
#endif

#ifdef DEBUG_MOVEMENT_FOR_VR_ELEVATOR_ISSUES
	if (keepWithinSafeVolumeOnLockedBase)
	{
		output(TXT("[base_on] forced based \"%S\" on \"%S\""), get_owner()->ai_get_name().to_char(), _onto? _onto->ai_get_name().to_char() : TXT("--"));
	}
#endif

	basedOn.set(_onto, get_placement());
	forcedBasedOn = _onto != nullptr;
}

void ModulePresence::update_based_on()
{
	if ((!can_be_based_on() && !can_be_based_on_volumetric ()) || forcedBasedOn)
	{
		return;
	}

	if (is_attached())
	{
		basedOn.clear();
		basedOnDoneAtLoc.clear();
		return;
	}

	// disallow changing base if our base is in locked state
	if (basedOn.is_set() && basedOn.get_presence()->is_locked_as_base(this))
	{
#ifdef DEBUG_PLAYER_BASE_DETAILED
		if (GameUtils::is_player(get_owner()))
		{
			output(TXT("[PLAYER'S PRESENCE] on a locked base"));
		}
#endif
		return;
	}

	if (basedOn.is_set() && basedOnDoneAtLoc.is_set() &&
		!GameUtils::is_controlled_by_player(get_owner()))
	{
		float distSq = (basedOnDoneAtLoc.get() - placement.get_translation()).length_squared();
		// check only every very short distance - when they're standing, do not check at all
		if (distSq < sqr(0.02f) && basedOnDoneAtTime.get_time_since() < 0.5f)
		{
			// use previous as we didn't move and not too much time has passed since last check
			return;
		}
	}

	basedOnDoneAtTime.reset();
	basedOnDoneAtLoc = placement.get_translation();

	MEASURE_PERFORMANCE_COLOURED(presence_udpateBasedOn, Colour::zxGreen);

	IModulesOwner* newBase = basedOn.get(); // by default keep current one

	if (can_be_based_on())
	{
		// store flags for later use
		Collision::Flags presenceTraceFlags = Collision::DefinedFlags::get_presence_trace();
		if (auto const * mc = get_owner()->get_collision())
		{
			presenceTraceFlags = mc->get_presence_trace_flags();
		}

		CheckCollisionCache cache;
		cache.build_from_presence_links(get_in_room(), get_placement(), modulePresenceData->basedOnPresenceTraces);
		an_assert(modulePresenceData->basedOnPresenceTraces.get_size() == 1, TXT("handle multiple traces"));
		for_every(basedOnPresenceTrace, modulePresenceData->basedOnPresenceTraces)
		{
			CheckSegmentResult result;
			CheckCollisionContext context;
			context.collision_info_needed();
			context.use_cache(&cache);
			context.use_collision_flags(presenceTraceFlags);
			context.use_check_object([this](Collision::ICollidableObject const * _object)
			{
				if (auto * imo = fast_cast<IModulesOwner>(_object))
				{
					if (auto * presence = imo->get_presence())
					{
#ifdef DEBUG_PLAYER_BASE_DETAILED
						if (GameUtils::is_player(get_owner()))
						{
							output(TXT("[PLAYER'S PRESENCE] trace hit: <%S>"), imo? imo->ai_get_name().to_char() : TXT(">none<"));
						}
#endif
						// only if we can base on that object and it is not in locked state
						if (presence->canBeBase && !presence->is_locked_as_base(this))
						{
							if (!presence->canBeBaseForPlayer)
							{
								if (GameUtils::is_controlled_by_player(get_owner()))
								{
#ifdef DEBUG_PLAYER_BASE_DETAILED
									output(TXT("[PLAYER'S PRESENCE]    not for a player"));
#endif
									return false;
								}
							}
#ifdef DEBUG_PLAYER_BASE_DETAILED
							if (GameUtils::is_player(get_owner()))
							{
								output(TXT("[PLAYER'S PRESENCE]    ok!"));
							}
#endif
							return true;
						}
#ifdef DEBUG_PLAYER_BASE_DETAILED
						if (GameUtils::is_player(get_owner()))
						{
							if (!presence->canBeBase)
							{
								output(TXT("[PLAYER'S PRESENCE]    not a base"));
							}
							else if (presence->is_locked_as_base(this))
							{
								output(TXT("[PLAYER'S PRESENCE]    locked as a base for us"));
							}
							else
							{
								output(TXT("[PLAYER'S PRESENCE]    other reason?"));
							}
						}
#endif
						return false;
					}
				}
				return true;
			});
			if (ModuleCollision const * collision = get_owner()->get_collision())
			{
				todo_future(TXT("separate flags for gravity presence traces? now instead of presence trace we use \"collides with\" which makes presence traces useless?"));
				//context.use_collision_flags(collision->get_collides_with_flags()); // for time being, keep to presence traces
			}
			if (trace_collision(AgainstCollision::Movement, *basedOnPresenceTrace, REF_ result, CollisionTraceFlag::ResultInPresenceRoom, context))
			{
				newBase = fast_cast<IModulesOwner>(cast_to_nonconst(result.object));
#ifdef DEBUG_PLAYER_BASE_DETAILED
				if (GameUtils::is_player(get_owner()))
				{
					output(TXT("[PLAYER'S PRESENCE] found possible base <%S>"), newBase ? newBase->ai_get_name().to_char() : TXT(">none<"));
				}
#endif
				if (newBase &&
					newBase->get_presence()->get_in_room() != get_in_room())
				{
#ifdef DEBUG_PLAYER_BASE_DETAILED
					if (GameUtils::is_player(get_owner()))
					{
						output(TXT("[PLAYER'S PRESENCE] base disallowed, different room <%S>"), newBase? newBase->ai_get_name().to_char() : TXT(">none<"));
					}
#endif
					// drop this new base as it is in different room
					newBase = nullptr;
				}

#ifdef DEBUG_PLAYER_BASE
#ifndef DEBUG_PLAYER_BASE_DETAILED
				if (newBase != basedOn.get())
#endif
				{
					if (GameUtils::is_player(get_owner()))
					{
						if (basedOnPresenceTrace->get_locations_in() == CollisionTraceInSpace::OS)
						{
							output(TXT("[PLAYER'S PRESENCE] trace used for base check (OS) %i %S to %S"),
								basedOnPresenceTrace->get_locations().get_size(),
								placement.location_to_world(basedOnPresenceTrace->get_locations().get_first()).to_string().to_char(),
								placement.location_to_world(basedOnPresenceTrace->get_locations().get_last()).to_string().to_char());
						}
						else
						{
							output(TXT("[PLAYER'S PRESENCE] trace used for base check (n/OS) %i %S to %S"),
								basedOnPresenceTrace->get_locations().get_size(),
								(basedOnPresenceTrace->get_locations().get_first()).to_string().to_char(),
								(basedOnPresenceTrace->get_locations().get_last()).to_string().to_char());
						}
					}
				}
#endif
			}
		}
	}

	if (can_be_based_on_volumetric())
	{
		Vector3 centreWS = get_centre_of_presence_WS();
		// check if current base is volumetric and we're still inside, if so, stay there, if not, find new one
		bool stillInCurrentBase = false;
		if (basedOn.is_set())
		{
			if (auto* objPresence = basedOn.get()->get_presence())
			{
				if (objPresence->canBeVolumetricBase)
				{
					an_assert(objPresence->volumetricBase.is_set());
					Vector3 centreInObjSpace = objPresence->get_placement().location_to_local(centreWS);
					if (objPresence->volumetricBase.get().does_contain(centreInObjSpace))
					{
						stillInCurrentBase = true;
						newBase = basedOn.get();
					}
					else
					{
						// leave this base!
						newBase = nullptr;
					}
				}
			}
		}

		if (!stillInCurrentBase)
		{
			for_every_ptr(obj, get_in_room()->get_objects())
			{
				if (auto* objPresence = obj->get_presence())
				{
					if (objPresence->canBeVolumetricBase)
					{
						an_assert(objPresence->volumetricBase.is_set());
						Vector3 centreInObjSpace = objPresence->get_placement().location_to_local(centreWS);
						if (objPresence->volumetricBase.get().does_contain(centreInObjSpace))
						{
							newBase = obj;
						}
					}
				}
			}
		}

#ifdef DEBUG_PLAYER_BASE
#ifndef DEBUG_PLAYER_BASE_DETAILED
		if (newBase != basedOn.get())
#endif
		{
			if (GameUtils::is_player(get_owner()))
			{
				output(TXT("[PLAYER'S PRESENCE] changed due to volume"));
			}
		}
#endif
	}

#ifdef DEBUG_MOVEMENT_FOR_VR_ELEVATOR_ISSUES
	if (keepWithinSafeVolumeOnLockedBase)
	{
		output(TXT("[base_on] be based \"%S\" on \"%S\""), get_owner()->ai_get_name().to_char(), newBase ? newBase->ai_get_name().to_char() : TXT("--"));
	}
#endif

	basedOn.set(newBase, get_placement());
}

void ModulePresence::update_gravity()
{
	MEASURE_PERFORMANCE_COLOURED(presence_udpateGravity, Colour::zxBlue);
	bool useDefault = true;
	gravityPresenceTracesTouchSurroundings = false;
	/**
	 *	Here's whats happening
	 *	1. we try to provide gravity by checking multiple traces (presence traces)
	 *		a. if hit checks provide gravity, we use it
	 *		b. if they provide no gravity, we check room to see if maybe room provides gravity
	 *		c. if they there is no gravity - we assume no gravity and we will be checking normal/up dir from hit location
	 *		d. hit results might be ignored if surface is not aligned with our up dir (although in certain cases it might be taken into consideration)
	 *	2. if there are no presence traces, we just check gravity from room
	 */
#ifdef AN_DEVELOPMENT
	debugPresenceTraceHits.clear();
#endif
	if (modulePresenceData && !modulePresenceData->gravityPresenceTraces.is_empty())
	{
		IModulesOwner const * modulesOwner = get_owner();
		ModuleMovement const * moduleMovement = modulesOwner->get_movement();
		MEASURE_PERFORMANCE_COLOURED(checkGravityPresenceTraces, Colour::zxBlue);
	
		Collision::Flags presenceTraceFlags = Collision::DefinedFlags::get_presence_trace();
		Collision::Flags presenceTraceRejectFlags = Collision::DefinedFlags::get_presence_trace_reject();
		if (auto const * mc = modulesOwner->get_collision())
		{
			presenceTraceFlags = mc->get_presence_trace_flags();
			presenceTraceRejectFlags = mc->get_presence_trace_reject_flags();
		}

		useDefault = false;
		Vector3 newGravityDir = Vector3::zero;
		Vector3 upDir = placement.get_orientation().get_z_axis();
		float newGravityForce = 0.0f;
		float newGravityWeight = 0.0f;
		float tempFloorLevelOffsetAlongUpDirSum = 0.0f;
		float tempFloorLevelOffsetAlongUpDirWeight = 0.0f;
		struct GravityPresenceTraceHitLocation
		{
			float weight;
			Vector3 location;
			float gravityWeight; // not weight!
			Vector3 gravityForce;

			GravityPresenceTraceHitLocation(){}
			GravityPresenceTraceHitLocation(float _weight, Vector3 const & _location, float _gravityWeight, Vector3 const & _gravityForce) : weight(_weight), location(_location), gravityWeight(_gravityWeight), gravityForce(_gravityForce) {}
		};
		CheckCollisionCache cache;
		cache.build_from_presence_links(get_in_room(), get_placement(), modulePresenceData->gravityPresenceTraces);
		ARRAY_STACK(GravityPresenceTraceHitLocation, gravityPresenceTraceHitLocations, modulePresenceData->gravityPresenceTraces.get_size());
		int currentGravityPresenceTracePriority = INF_INT;
		while (true)
		{
			{
				int nextGravityPresenceTracePriority = NONE;
				bool found = false;
				for_every(gravityPresenceTrace, modulePresenceData->gravityPresenceTraces)
				{
					int prio = gravityPresenceTrace->get_priority();
					if (prio < currentGravityPresenceTracePriority)
					{
						nextGravityPresenceTracePriority = !found ? prio : max(nextGravityPresenceTracePriority, prio);
						found = true;
					}
				}
				if (!found)
				{
					break;
				}
				currentGravityPresenceTracePriority = nextGravityPresenceTracePriority;
			}
			for_every(gravityPresenceTrace, modulePresenceData->gravityPresenceTraces)
			{
				if (gravityPresenceTrace->get_priority() != currentGravityPresenceTracePriority)
				{
					continue;
				}
				CheckSegmentResult result;
				CheckCollisionContext context;
				context.collision_info_needed();
				context.use_cache(&cache);
				// we no longer ignore objects as we want trace using "presence trace" flags and we want to land on platform actors too
				context.use_collision_flags(presenceTraceFlags);
				if (ModuleCollision const * collision = get_owner()->get_collision())
				{
					todo_future(TXT("separate flags for gravity presence traces? now instead of presence trace we use \"collides with\" which makes presence traces useless?"));
					//context.use_collision_flags(collision->get_collides_with_flags()); // for time being, keep to presence traces
				}
				// gravityPresenceTrace->debug_draw(placement); // !@#
				if (trace_collision(AgainstCollision::Movement, *gravityPresenceTrace, REF_ result, CollisionTraceFlag::ResultInPresenceRoom, context))
				{
					if (!result.has_collision_flags(presenceTraceRejectFlags, false))
					{
						float useGravityFromTraceDotLimit = 0.6f;
						float floorLevelOffset = 0.0f;
						bool allowUsingGravityFromOutboundTraces = false;
						if (moduleMovement)
						{
							useGravityFromTraceDotLimit = moduleMovement->find_use_gravity_from_trace_dot_limit_of_current_gait();
							floorLevelOffset = moduleMovement->find_floor_level_offset_of_current_gait();
							allowUsingGravityFromOutboundTraces = moduleMovement->find_does_allow_using_gravity_from_outbound_traces_of_current_gait();
						}
						bool useTraceForGravity = Vector3::dot(result.hitNormal, upDir) > useGravityFromTraceDotLimit; // is floor - on option can be ignored or not, actually it should be never ignored but maybe adjusted?
						if (!useTraceForGravity)
						{
							Vector3 const relativeHitLocation = (result.hitLocation - placement.get_translation()).drop_using_normalised(upDir);
							Vector3 const hitNormal = result.hitNormal.drop_using_normalised(upDir);
							if (Vector3::dot(hitNormal, relativeHitLocation) > 0.0f)
							{
								useTraceForGravity = true;
							}
						}
						if (useTraceForGravity &&
							result.has_collision_flags(presenceTraceFlags, true))
						{
							// use only result that is valid for presence traces (blocks and does not reject)
							float const weight = 1.0f;
							Vector3 gravityForceToProvide = Vector3::zero;
							Vector3 gravityDirToProvide = Vector3::zero;
							if (!result.gravityDir.is_set() ||
								!result.gravityForce.is_set())
							{
								// if no gravity provided, use one from room
								// if room provides no gravity then we will be using hit locations anyway
								get_in_room()->update_gravity(REF_ result.gravityDir, REF_ result.gravityForce);
							}
							if (result.gravityDir.is_set() &&
								result.gravityForce.is_set())
							{
								gravityForceToProvide = result.gravityDir.get() * result.gravityForce.get();
								gravityDirToProvide = result.gravityDir.get();
								newGravityDir += weight * result.gravityDir.get();
								newGravityForce += weight * result.gravityForce.get();
								newGravityWeight += weight;
							}
							gravityPresenceTraceHitLocations.push_back(GravityPresenceTraceHitLocation(weight, result.hitLocation, weight, gravityForceToProvide));
							{
								// weight floor level offset
								// if gravity is present (non zero) we want to align only if our hit is aligned with gravity (reversed)
								// if there's no gravity, any hit location is good
								float weight = gravityDirToProvide.is_zero() ? 1.0f : clamp(Vector3::dot(-gravityDirToProvide, result.hitNormal), 0.0f, 1.0f);
								float offset = Vector3::dot(upDir, (result.hitLocation - placement.get_translation())) - floorLevel + floorLevelOffset;
								tempFloorLevelOffsetAlongUpDirSum += offset * weight;
								tempFloorLevelOffsetAlongUpDirWeight += weight;
							}
#ifdef AN_DEVELOPMENT
							debugPresenceTraceHits.push_back(DebugPresenceTraceHit(result.hitLocation, result.hitNormal));
#endif
						}
					}
				}
			}
			if (newGravityWeight != 0.0f)
			{
				break;
			}
		}
		if (newGravityWeight != 0.0f)
		{
			gravityDir = newGravityDir.normal(); // normalise it! don't rely just on weights
			gravity = gravityDir * newGravityForce / newGravityWeight; // we divide each one by gravity weight)
		}
		if (tempFloorLevelOffsetAlongUpDirWeight != 0.0f)
		{
			floorLevelOffsetAlongUpDir = tempFloorLevelOffsetAlongUpDirSum / tempFloorLevelOffsetAlongUpDirWeight;
		}
		else
		{
			floorLevelOffsetAlongUpDir = 0.0f;
		}
		if (!gravityPresenceTraceHitLocations.is_empty())
		{
			//debug_context(inRoom);
			Vector3 centre = Vector3::zero;
			{
				float weightSum = 0.0f;
				for_every(hitLocation, gravityPresenceTraceHitLocations)
				{
					centre += hitLocation->location;
					weightSum += 1.0f;
				}
				centre /= weightSum;
			}
			lastGravityPresenceTracesTouchOS = placement.location_to_local(centre);
			// check hit locations against centre and upDir to determine what each location prefers as up dir
			// or just use gravity if provided
			Vector3 preferredGravityDir = Vector3::zero;
			{
				float weightSum = 0.0f;
				for_every(hitLocation, gravityPresenceTraceHitLocations)
				{
					//debug_draw_line(true, Colour::red, centre, hitLocation->location);

					todo_note(TXT("TN#6 for characters that ignore gravity there should be option"));
					if (!hitLocation->gravityForce.is_zero())
					{
						preferredGravityDir += hitLocation->gravityForce * hitLocation->gravityWeight;
						weightSum += hitLocation->gravityWeight;
					}
					else
					{
						Vector3 offset = hitLocation->location - centre;
						if (offset.length() > 0.1f)
						{
							//debug_draw_line(true, Colour::cyan, centre, centre + offset);
							Vector3 const right = Vector3::cross(offset, upDir);
							Vector3 const newGravityDir = Vector3::cross(offset, right).normal();
							preferredGravityDir += newGravityDir;
							weightSum += 1.0f;
						}
					}
				}
				preferredGravityDir = (preferredGravityDir / weightSum).normal();
			}
			//debug_draw_line(true, Colour::blue, centre, centre + upDir * floorLevelOffsetAlongUpDir);
			//debug_draw_line(true, Colour::green, centre, centre + preferredGravityDir);
			gravityDirFromHitLocations = preferredGravityDir;
			gravityPresenceTracesTouchSurroundings = true;
			//debug_no_context();
		}
		else
		{
			useDefault = true;
		}
	}
	if (useDefault)
	{
		Optional<Vector3> newGravityDir;
		Optional<float> newGravityForce;
		get_in_room()->update_gravity(REF_ newGravityDir, REF_ newGravityForce);
		if (newGravityDir.is_set() && newGravityForce.is_set())
		{
			gravity = newGravityDir.get() * newGravityForce.get();
			an_assert_immediate(newGravityDir.get().is_normalised());
			gravityDir = newGravityDir.get();
			gravityDirFromHitLocations = newGravityDir.get();
		}
		floorLevelOffsetAlongUpDir = 0.0f;
		gravityPresenceTracesTouchSurroundings = false;
	}
}

bool ModulePresence::trace_collision(AgainstCollision::Type _againstCollision, CollisionTrace const & _collisionTrace, REF_ CheckSegmentResult & _result, CollisionTraceFlag::Flags _flags, CheckCollisionContext & _context, RelativeToPresencePlacement * _fillRelativeToPresencePlacement) const
{
	if (_collisionTrace.get_locations().get_size() < 2)
	{
		an_assert(_collisionTrace.get_locations().get_size() >= 2, TXT("what are you trying to check?"));
		return false;
	}

	if (! inRoom)
	{
		warn(TXT("not in a room %S"), get_owner()->ai_get_name().to_char());
		return false;
	}

	// this is for _fillRelativeToPresencePlacement to update on hit and store all doors on the way
	allocate_stack_var(Framework::DoorInRoom const*, allThroughDoors, PathSettings::MaxDepth);
	DoorInRoomArrayPtr allThroughDoorsArrayPtr(allThroughDoors, PathSettings::MaxDepth);

	MEASURE_PERFORMANCE_COLOURED(presence_traceCollision, Colour::c64Red);
	CheckCollisionCache cache;
	bool ownCache = false;
	if (!_context.get_cache())
	{
		// build own cache
		ownCache = true;
		cache.build_from_presence_links(get_in_room(), get_placement(), _collisionTrace);
		_context.use_cache(&cache);
	}
	// we will fill hit point (or lack of it, ie. end point) on the way (in Room::check_segment_against through door will be filled)
	Vector3 prevInCheckRoomLocation = placement.get_translation();
	Vector3 blockingNormal = Vector3::zero;
	Room const * checkRoom = inRoom;
	Transform intoCheckRoomTransform = Transform::identity; // transform that takes us from inRoom to room that we're currently checking (checkRoom)
	bool traceThroughDoorsOnly = true;
	int traceIdx = -1; // first is either empty or initial doors from starting point
	// first trace is auto, from our current location to just go through doors
	bool result = false;
	for_every(location, _collisionTrace.get_locations())
	{
		bool const initialTrace = traceIdx < 0;
		Vector3 inStartingRoomLocation;
		if (is_flag_set(_flags, CollisionTraceFlag::StartInProvidedRoom))
		{
			an_assert_immediate(_collisionTrace.get_locations_in() == CollisionTraceInSpace::WS);
			inStartingRoomLocation = _collisionTrace.get_into_start_room_transform().location_to_local(*location);
		}
		else
		{
			inStartingRoomLocation = _collisionTrace.get_locations_in() == CollisionTraceInSpace::OS ? placement.location_to_world(*location) : *location;
		}
		Vector3 inCheckRoomLocation = intoCheckRoomTransform.location_to_world(inStartingRoomLocation);
		if (!blockingNormal.is_zero())
		{
			// slide along the wall
			float intoBlocking = Vector3::dot(-blockingNormal, inCheckRoomLocation - prevInCheckRoomLocation);
			if (intoBlocking >= 0.0f)
			{
				inCheckRoomLocation += intoBlocking * blockingNormal;
			}
		}
		if (initialTrace && is_flag_set(_flags, CollisionTraceFlag::StartInProvidedRoom))
		{
			checkRoom = _collisionTrace.get_start_in_room();
			intoCheckRoomTransform = intoCheckRoomTransform.to_world(_collisionTrace.get_into_start_room_transform());
			// don't do anything
		}
		else
		{
			Room const * endsInRoom = checkRoom;
			Transform intoEndRoomTransform = Transform::identity;
			Segment inCheckRoomSegment(prevInCheckRoomLocation, inCheckRoomLocation);
			bool hit = false;
			if (prevInCheckRoomLocation != inCheckRoomLocation) // if same may result hitting doors due to minInFront
			{
				int const throughDoorsLimit = 32;
				allocate_stack_var(Framework::DoorInRoom const*, throughDoors, throughDoorsLimit);
				DoorInRoomArrayPtr throughDoorsArrayPtr(throughDoors, throughDoorsLimit);
				if (traceThroughDoorsOnly)
				{
					// first trace is from current placement to first line in trace - and it checks just doors to know in which space should we start checking
					CheckCollisionContext doorsOnlyContext = _context;
					doorsOnlyContext.doors_only();
					hit = checkRoom->check_segment_against(_againstCollision, REF_ inCheckRoomSegment, _result, doorsOnlyContext, endsInRoom, intoEndRoomTransform, &throughDoorsArrayPtr);
					an_assert(!hit, TXT("there should be no hit when checking doors on first trace"));
				}
				else
				{
					hit = checkRoom->check_segment_against(_againstCollision, REF_ inCheckRoomSegment, _result, _context, endsInRoom, intoEndRoomTransform, &throughDoorsArrayPtr);
				}

				if (_context.get_non_aligned_gravity_slide_dot_threshold().is_set() && hit && !get_gravity_dir().is_zero())
				{
					float alignWithGravity = Vector3::dot(-get_gravity_dir(), _result.hitNormal);
					if (alignWithGravity < _context.get_non_aligned_gravity_slide_dot_threshold().get())
					{
						hit = false;
						_result.hitLocation = _result.hitLocation + _result.hitNormal * 0.001f;
						blockingNormal = _result.hitNormal;
					}
				}

				checkRoom = endsInRoom;
				inCheckRoomLocation = intoEndRoomTransform.location_to_world(inCheckRoomLocation);
				intoCheckRoomTransform = intoCheckRoomTransform.to_world(intoEndRoomTransform);
				blockingNormal = blockingNormal.is_zero()? blockingNormal : intoEndRoomTransform.vector_to_world(blockingNormal);

				if (_fillRelativeToPresencePlacement)
				{
					// store in case we would hit, we do not want to modify existing relativetopresenceplacement, we want only to update on hit
					DoorInRoom const ** throughDoor = throughDoors;
					for (int i = 0; i < throughDoorsArrayPtr.get_number_of_doors(); ++i, ++throughDoor)
					{
						allThroughDoorsArrayPtr.push_door(*throughDoor);
					}
				}
				if (hit && _fillRelativeToPresencePlacement)
				{
					_fillRelativeToPresencePlacement->clear_target();
					// use results to add "through doors" that are on the way and add hit location (or just end point)
					DoorInRoom const ** throughDoor = allThroughDoors;
					for (int i = 0; i < allThroughDoorsArrayPtr.get_number_of_doors(); ++i, ++throughDoor)
					{
						_fillRelativeToPresencePlacement->push_through_door(*throughDoor);
					}
					Vector3 hitLocation = _result.hitLocation;
					if (_result.presenceLink)
					{
						an_assert(_result.presenceLink->owner);
						// go up towards owner to know exact location, move hit location there
						PresenceLink const * link = _result.presenceLink;
						while (link->throughDoor)
						{
							an_assert(link->linkTowardsOwner);
							hitLocation = link->throughDoor->get_other_room_transform().location_to_local(hitLocation);
							_fillRelativeToPresencePlacement->push_through_door(link->throughDoor);
							link = link->linkTowardsOwner;
						}
					}
					_fillRelativeToPresencePlacement->set_placement_in_final_room(Transform(hitLocation, Quat::identity), _result.presenceLink ? _result.presenceLink->ownerPresence : nullptr);
				}
			}

			// update result
			_result.changedRoom = checkRoom != inRoom;
			if (_flags & CollisionTraceFlag::ResultInPresenceRoom)
			{
				_result.to_local_of(intoCheckRoomTransform);
				_result.inRoom = inRoom;
				_result.intoInRoomTransform = Transform::identity;
			}
			else if (_flags & CollisionTraceFlag::ResultInFinalRoom)
			{
				_result.intoInRoomTransform = intoCheckRoomTransform;
			}
			else if (_flags & CollisionTraceFlag::ResultInObjectSpace)
			{
				_result.to_local_of(intoCheckRoomTransform);
				_result.to_local_of(placement);
				_result.inRoom = inRoom;
				_result.intoInRoomTransform = Transform::identity;
			}
			else
			{
				an_assert(_flags & CollisionTraceFlag::ResultNotRequired, TXT("not defined where result should be, assuming final room"));
				_result.intoInRoomTransform = intoCheckRoomTransform;
			}

			if (hit)
			{
				// we want to have traces in space of inRoom - where we currently are
				result = true;
				break;
			}
			else if (_flags & CollisionTraceFlag::ProvideNonHitResult)
			{
				// update empty trace
				_result.hit = false;
				_result.inRoom = checkRoom;
				_result.intoInRoomTransform = intoCheckRoomTransform;
				_result.hitLocation = inCheckRoomLocation;
			}
		}

		++traceIdx; // not that we start at -1
		if (traceIdx > 0 || !is_flag_set(_flags, CollisionTraceFlag::FirstTraceThroughDoorsOnly))
		{
			traceThroughDoorsOnly = false;
		}
		prevInCheckRoomLocation = inCheckRoomLocation;
	}
	if (ownCache)
	{
		_context.use_cache(nullptr);
	}
	return result;
}

void ModulePresence::advance__prepare_move(IMMEDIATE_JOB_PARAMS)
{
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		MEASURE_PERFORMANCE(prepareMove);
		if (ModulePresence* self = modulesOwner->get_presence())
		{
			an_assert(modulesOwner->does_require_move_advance());
			self->preMovePlacement = self->placement;
			self->prepare_move(modulesOwner, modulesOwner->get_move_advance_delta_time());
			self->preparedMovePlacement = self->nextPlacement;
		}
		else
		{
			an_assert(false, TXT("shouldn't be called"));
		}
	}
}

void ModulePresence::prepare_move(IModulesOwner* modulesOwner, float deltaTime)
{
	MEASURE_PERFORMANCE(prepareMove);

	vrAnchorLastDisplacement = Transform::identity;

	if (useInitialVelocityLinearOS.is_set())
	{
		set_velocity_linear(placement.vector_to_world(useInitialVelocityLinearOS.get()));
		an_assert(!useInitialVelocityLinearOS.is_set(), TXT("should be cleared by set velocity linear"));
	}

	if (!velocityImpulses.is_empty())
	{
		MEASURE_PERFORMANCE(velocityImpulses);
		Concurrency::ScopedSpinLock lock(velocityImpulsesLock);

		if (!ignoreImpulses)
		{
			for_every(velocityImpulse, velocityImpulses)
			{
				nextVelocityLinear += *velocityImpulse;
				an_assert_immediate(nextVelocityLinear.is_ok());
			}
		}

		velocityImpulses.clear();
	}

	an_assert_immediate(nextVelocityRotation.is_ok());

	if (!orientationImpulses.is_empty())
	{
		MEASURE_PERFORMANCE(orientationImpulses);
		Concurrency::ScopedSpinLock lock(orientationImpulsesLock);

		if (!ignoreImpulses)
		{
			for_every(orientationImpulse, orientationImpulses)
			{
				nextVelocityRotation += *orientationImpulse;
			}
		}

		an_assert_immediate(nextVelocityRotation.is_ok());
		
		orientationImpulses.clear();
	}

	if (!is_attached())
	{
		store_velocity_if_required();
	}

	if (get_in_room())
	{
		// apply collision now, as before we don't want to affect "logic speed" - where character would like to go to
		// and as collisions are really limiting us, we want to do collision manipulation last
		Vector3 locationDisplacement = nextVelocityLinear * deltaTime + nextLocationDisplacement;
		an_assert_immediate(locationDisplacement.is_ok());
		Quat orientationDisplacement = (nextVelocityRotation * deltaTime).to_quat().rotate_by(nextOrientationDisplacement);
		an_assert(abs(orientationDisplacement.length() - 1.0f) < 0.1f);
		bool updateVelocities = false; // attached objects (like hands) may have prepare_move called to update look controls (for turrets) but in general they should not have velocities updated
		// apply final changes to velocity and displacement
		if (ModuleMovement* movement = modulesOwner->get_movement())
		{
			MEASURE_PERFORMANCE(fromMovement);
			ModuleMovement::VelocityAndDisplacementContext context(nextVelocityLinear, locationDisplacement, nextVelocityRotation, orientationDisplacement);
			an_assert(abs(orientationDisplacement.length() - 1.0f) < 0.1f);
			{
				// for vr movement if not using sliding locomotion - ignore collision
				bool ignoreCollision = (is_vr_movement() && !Game::is_using_sliding_locomotion());
#ifdef TEST_NOT_SLIDING_LOCOMOTION
				if (beAlwaysAtVRAnchorLevel) // a way to determine it's the player
				{
					ignoreCollision = true;
				}
#endif
				context.apply_collision(! ignoreCollision);
			}
			movement->apply_changes_to_velocity_and_displacement(deltaTime, context);
			nextVelocityLinear = context.velocityLinear;
			an_assert_immediate(nextVelocityLinear.is_ok());
			locationDisplacement = context.currentDisplacementLinear;
			an_assert_immediate(locationDisplacement.is_ok());
			if (beAlwaysAtVRAnchorLevel)
			{
				// for vr movement, disallow any vertical displacement from vr anchor
				Vector3 up = vrAnchor.get_axis(Axis::Up);
				locationDisplacement = locationDisplacement.drop_using(up);
				an_assert_immediate(locationDisplacement.is_ok());
			}
			nextVelocityRotation = context.velocityRotation;
			an_assert_immediate(nextVelocityRotation.is_ok());
			orientationDisplacement = context.currentDisplacementRotation;
			an_assert(abs(orientationDisplacement.length() - 1.0f) < 0.1f);
			if (!movement->is_rotation_allowed())
			{
				nextVelocityRotation = Rotator3::zero;
				orientationDisplacement = Quat::identity;
			}
			updateVelocities = true;
		}
		else if (!is_attached())
		{
			locationDisplacement = Vector3::zero;
			orientationDisplacement = Quat::identity;
			nextVelocityLinear = Vector3::zero;
			nextVelocityRotation = Rotator3::zero;
			updateVelocities = true;
		}

		if (useRoomVelocity > 0.0f && inRoom)
		{
			Vector3 roomVelocity = inRoom->get_room_velocity();
			Vector3 roomVelocityDisplacement = -roomVelocity * deltaTime;
			useRoomVelocityTime += deltaTime;
			locationDisplacement += roomVelocityDisplacement * useRoomVelocity * (useRoomVelocityInTime <= 0.0f ? 1.0f : clamp(useRoomVelocityTime / useRoomVelocityInTime, 0.0f, 1.0f));
			an_assert_immediate(locationDisplacement.is_ok());
		}

		if (noVerticalMovement)
		{
			Vector3 up = placement.get_axis(Axis::Up).normal();
			locationDisplacement = locationDisplacement.drop_using_normalised(up);
			nextVelocityLinear = nextVelocityLinear.drop_using_normalised(up);
			an_assert_immediate(locationDisplacement.is_ok());
			an_assert_immediate(nextVelocityLinear.is_ok());
		}

		{
			MEASURE_PERFORMANCE(prepareMove_nextPlacement);

			an_assert(abs(placement.get_orientation().length() - 1.0f) < 0.1f);
			an_assert(abs(orientationDisplacement.length() - 1.0f) < 0.1f);
			nextPlacementDeltaTime = deltaTime;
			nextPlacement.set_translation(placement.get_translation() + locationDisplacement);
			nextPlacement.set_scale(placement.get_scale());
			nextPlacement.set_orientation(placement.get_orientation().rotate_by(orientationDisplacement));
			an_assert_immediate(nextPlacement.is_ok());
			an_assert_immediate(nextPlacement.get_scale() != 0.0f);
			// we should rotate around centre of mass, to achieve that, move our origin accordingly
			if (!centreOfMass.is_zero())
			{
				// add translation (negative of movement of centre of mass if it would be rotated)
				Vector3 const comAtPlacement = placement.get_orientation().to_world(centreOfMass);
				Vector3 const comAtNextPlacement = nextPlacement.get_orientation().to_world(centreOfMass);
				Vector3 const comDisplacement = comAtNextPlacement - comAtPlacement;
				nextPlacement.set_translation(nextPlacement.get_translation() - comDisplacement);
			}
			an_assert_immediate(nextPlacement.is_ok());
			an_assert_immediate(nextPlacement.get_scale() != 0.0f);
		}

		/*
		{
			MEASURE_PERFORMANCE(prepareMove_maintainRelativeOrientation);
			// maintain relative orientation (rotates yaw)
			Quat requestedLookRelativeOrientation = nextPlacement.get_orientation().to_local((placement.get_orientation().to_world(requestedRelativeLook.get_orientation())));
			an_assert(abs(requestedRelativeLook.get_orientation().length() - 1.0f) < 0.1f);
			an_assert(abs(placement.get_orientation().length() - 1.0f) < 0.1f);
			an_assert(abs(nextPlacement.get_orientation().length() - 1.0f) < 0.1f);
			an_assert(abs(requestedLookRelativeOrientation.length() - 1.0f) < 0.1f, TXT("requestedLookRelativeOrientation is %S"), requestedLookRelativeOrientation.to_string().to_char());
			requestedLookRelativeOrientation.normalise();
			requestedRelativeLook.set_orientation(requestedLookRelativeOrientation);
		}
		*/

		// ...but overwrite if requested looking dir is set
		if (ModuleController* controller = modulesOwner->get_controller())
		{
			MEASURE_PERFORMANCE(prepareMove_updateController);
			if (controller->is_requested_look_set())
			{
				requestedRelativeLook = controller->get_requested_relative_look();
				an_assert(abs(requestedRelativeLook.get_orientation().length() - 1.0f) < 0.1f);

				bool maintainRelativeRequestedLookOrientation = controller->get_maintain_relative_requested_look_orientation().get(false);
				if (maintainRelativeRequestedLookOrientation)
				{
					// it's ok
				}
				else
				{
					requestedRelativeLook.set_orientation(nextPlacement.get_orientation().to_local(controller->get_requested_look_orientation())); // requested not relative! it is done using current placement
				}
				an_assert(requestedRelativeLook.get_orientation().is_normalised());
			}
			lookOrientationAdditionalOffset = controller->get_look_orientation_additional_offset();

			for_count(int, h, 2)
			{
				if (controller->is_requested_hand_set(h))
				{
					requestedRelativeHands[h] = controller->get_requested_hand(h);
					requestedRelativeHandsAccurate[h] = controller->is_requested_hand_accurate(h);
				}
			}
		}

		// next becomes current and clear displacements
		if (updateVelocities)
		{
			velocityLinear = nextVelocityLinear;
			velocityRotation = nextVelocityRotation;
			nextLocationDisplacement = Vector3::zero;
			nextOrientationDisplacement = Quat::identity;
		}
	}

	{
		disallowFallingTimeLeft = max(0.0f, disallowFallingTimeLeft - deltaTime);
	}
}

void ModulePresence::advance_vr_important__looks(IModulesOwner* _modulesOwner)
{
	if (ModulePresence* self = _modulesOwner->get_presence())
	{
		// ...but overwrite if requested looking dir is set
		if (ModuleController * controller = _modulesOwner->get_controller())
		{
			if (controller->is_requested_look_set())
			{
				self->requestedRelativeLook = controller->get_requested_relative_look();
			}
			self->lookOrientationAdditionalOffset = controller->get_look_orientation_additional_offset();
			for_count(int, h, 2)
			{
				if (controller->is_requested_hand_set(h))
				{
					self->requestedRelativeHands[h] = controller->get_requested_hand(h);
					self->requestedRelativeHandsAccurate[h] = controller->is_requested_hand_accurate(h);
				}
			}
		}
	}
	else
	{
		an_assert(false, TXT("shouldn't be called"));
	}
}

void ModulePresence::advance__do_move(IMMEDIATE_JOB_PARAMS)
{
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		MEASURE_PERFORMANCE(doMove);
		if (ModulePresence* self = modulesOwner->get_presence())
		{
			scoped_call_stack_info(modulesOwner->ai_get_name().to_char());
			an_assert(modulesOwner->does_require_move_advance());
			if (!self->basedOn.get() ||
				! self->basedOn.get()->get_presence()->is_base_through_doors())
			{
				self->do_move(modulesOwner);
			}
		}
		else
		{
			an_assert(false, TXT("shouldn't be called"));
		}
	}
}

bool ModulePresence::does_require_do_move() const
{
	if (attachmentRequest.type != AttachmentRequest::None ||
		teleportRequested || teleported ||
		is_vr_movement())
	{
		return true;
	}

	if (is_base_through_doors() && !based.is_empty())
	{
		return true;
	}

	if (attachment.attachedTo)
	{
		return false;
	}

	if (moveUsingEyes || moveUsingEyesZOnly)
	{
		// should be ok for time being
		return true;
	}

	if (basedOn.is_set())
	{
		return true;
	}

	if (auto* m = get_owner()->get_movement())
	{
		if (m->does_require_continuous_movement())
		{
			return true;
		}
	}

	return ! Transform::same(nextPlacement, placement, 0.01f * nextPlacementDeltaTime, 0.01f * nextPlacementDeltaTime, 0.0f);
}

Transform ModulePresence::calculate_move_centre_offset_os() const
{
	Transform moveCentreOffsetOS = presenceLinkCentreOS;
	if (moveUsingEyesZOnly)
	{
		Vector3 loc = moveCentreOffsetOS.get_translation();
		loc.z = get_eyes_relative_look().get_translation().z;
		moveCentreOffsetOS.set_translation(loc);
	}
	else if (moveUsingEyes)
	{
		moveCentreOffsetOS.set_translation(get_eyes_relative_look().get_translation());
	}
	return moveCentreOffsetOS;
}

void ModulePresence::do_move(IModulesOwner * modulesOwner)
{
	if (attachmentRequest.type != AttachmentRequest::None)
	{
		// do requested attachments now
		if (attachmentRequest.type == AttachmentRequest::AttachToUsingInRoomPlacement)
		{
			if (attachmentRequest.toObject.get())
			{
				if (attachmentRequest.toBone.is_valid())
				{
					attach_to_bone_using_in_room_placement(attachmentRequest.toObject.get(), attachmentRequest.toBone, attachmentRequest.following, attachmentRequest.placement, attachmentRequest.doAttachmentMovement);
				}
				else if (attachmentRequest.toSocket.is_valid())
				{
					attach_to_socket_using_in_room_placement(attachmentRequest.toObject.get(), attachmentRequest.toSocket, attachmentRequest.following, attachmentRequest.placement, attachmentRequest.doAttachmentMovement);
				}
				else
				{
					attach_to_using_in_room_placement(attachmentRequest.toObject.get(), attachmentRequest.following, attachmentRequest.placement, attachmentRequest.doAttachmentMovement);
				}
			}
			else
			{
				warn(TXT("tried to attach \"%S\" to something, but the target has disappeared"), get_owner()->ai_get_name().to_char());
			}
		}
		if (attachmentRequest.type == AttachmentRequest::AttachToUsingRelativePlacement)
		{
			if (attachmentRequest.toObject.get())
			{
				if (attachmentRequest.toBone.is_valid())
				{
					attach_to_bone(attachmentRequest.toObject.get(), attachmentRequest.toBone, attachmentRequest.following, attachmentRequest.placement, attachmentRequest.doAttachmentMovement);
				}
				else if (attachmentRequest.toSocket.is_valid())
				{
					attach_to_socket(attachmentRequest.toObject.get(), attachmentRequest.toSocket, attachmentRequest.following, attachmentRequest.placement, attachmentRequest.doAttachmentMovement);
				}
				else
				{
					attach_to(attachmentRequest.toObject.get(), attachmentRequest.following, attachmentRequest.placement, attachmentRequest.doAttachmentMovement);
				}
			}
			else
			{
				warn(TXT("tried to attach \"%S\" to something, but the target has disappeared"), get_owner()->ai_get_name().to_char());
			}
		}
		attachmentRequest.type = AttachmentRequest::None;
		attachmentRequest.toObject.clear();
		attachmentRequest.toBone = Name::invalid();
		attachmentRequest.toSocket = Name::invalid();
	}
	
	if (attachment.attachedTo)
	{
		// will update in post move
		return;
	}

	Vector3 prevMoveCentreOffsetOS = moveCentreOffsetOS.is_set()? moveCentreOffsetOS.get().get_translation() : presenceLinkCentreOS.get_translation();
	moveCentreOffsetOS = calculate_move_centre_offset_os();

	baseVelocityLinear = Vector3::zero;
	baseVelocityRotation = Rotator3::zero;
	teleported = false;

#ifdef AN_ALLOW_BULLSHOTS
	if (bullshotPlacement.is_set())
	{
		place_within_room(bullshotPlacement.get());
	}
	else
#endif
	if (teleportRequested)
	{
		MEASURE_PERFORMANCE(teleport);
		scoped_call_stack_info(TXT("handle teleport"));
		teleported = true;
		teleportRequested = false;

		if (teleportBeVisible.is_set())
		{
			if (auto* a = get_owner()->get_appearance())
			{
				a->be_visible(teleportBeVisible.get());
			}
		}

		Transform placementInVR = get_placement_in_vr_space();
		Optional<Vector3> keepVelocityOS;
		if (teleportKeepVelocityOS.is_set())
		{
			keepVelocityOS = get_placement().vector_to_local(get_velocity_linear());
		}
		if (inRoom != teleportIntoRoom || !silentTeleport)
		{
			remove_from_room(NAME(teleports));
			place_in_room(teleportIntoRoom, teleportToPlacement, NAME(teleports));
		}
		else
		{
			place_within_room(teleportToPlacement);
		}
		if (keepVelocityOS.is_set())
		{
			set_velocity_linear(teleportToPlacement.vector_to_world(keepVelocityOS.get()));
		}
		if (teleportWithVelocity.is_set())
		{
			set_velocity_linear(teleportWithVelocity.get());
		}
		if (Game::is_using_sliding_locomotion())
		{
			// start at where we are
			zero_vr_anchor();
		}
		if (teleportFindVRAnchor)
		{
			vrAnchor = get_in_room()->get_vr_anchor(get_placement(), placementInVR);
		}
	}
	else if (inRoom)
	{
		struct BeAtVRAnchorLevel
		{
			static void update(Transform const& vrAnchor, REF_ Transform& placement)
			{
				Vector3 up = vrAnchor.get_axis(Axis::Up);
				{
					Vector3 verticalDisplacement = up * Vector3::dot(up, vrAnchor.get_translation() - placement.get_translation());
					an_assert_immediate(verticalDisplacement.is_ok());
					placement.set_translation(placement.get_translation() + verticalDisplacement);
				}
			}
		};

		Transform prevVRAnchor = vrAnchor;

		// use presence centre offset as move centre offset to make move happen in specific point of presence instead of border (that could be slightly outside the door)
		// moving through doors doesn't affect placements (just changes relative transform for them)
		// if we would use collision centre, we could end up in situation where presence centre is behind door (when affecting character's pitch or roll)
		// in most cases presenceLinkCentreOS is enough
		// important thing to remember is that we should use previous and current centre as this could move us through door as well
		Transform forcedNextPlacement = placement; // for based on, in case we have collision check and we wouldn't be able to move through a collision (if we're embedded in door!)
		Quat baseRotation = Quat::identity;
		if (basedOn.is_set())
		{
			// move by "based on"'s movement (between prev and next placements)
			if (auto const* basePresence = basedOn.get_presence())
			{
				if (basePresence->get_in_room() == get_in_room() || basedOn.is_based_through_doors()) // we depend on base being in the same room
				{
					// do not update placement, we don't do collisions here, and if nothing breaks, we will be at nextPlacemenet
					// update forced next placement
					Transform newNextPlacement = nextPlacement;
					Transform baseSpaceInOwnerSpace = Transform::identity;
					if (basedOn.is_based_through_doors())
					{
						baseSpaceInOwnerSpace = basedOn.transform_from_base_to_owner(Transform::identity);
						forcedNextPlacement = baseSpaceInOwnerSpace.to_local(forcedNextPlacement);
						newNextPlacement = baseSpaceInOwnerSpace.to_local(newNextPlacement);
						vrAnchor = baseSpaceInOwnerSpace.to_local(vrAnchor);
					}
					forcedNextPlacement = basePresence->transform_based_movement_placement(forcedNextPlacement);
					newNextPlacement = basePresence->transform_based_movement_placement(newNextPlacement);
					vrAnchor = basePresence->transform_based_movement_placement(vrAnchor);
					if (basedOn.is_based_through_doors())
					{
						forcedNextPlacement = baseSpaceInOwnerSpace.to_world(forcedNextPlacement);
						newNextPlacement = baseSpaceInOwnerSpace.to_world(newNextPlacement);
						vrAnchor = baseSpaceInOwnerSpace.to_world(vrAnchor);
					}

					baseRotation = nextPlacement.get_orientation().to_local(newNextPlacement.get_orientation());
					{
						float deltaTime = Game::get()->get_delta_time();
						float invDeltaTime = 1.0f / max(0.001f, deltaTime);
						baseVelocityRotation = baseRotation.to_rotator() * invDeltaTime;
						baseVelocityLinear = nextPlacement.location_to_local(newNextPlacement.get_translation()) * invDeltaTime;
					}

					nextPlacement = newNextPlacement;
					an_assert_immediate(nextPlacement.is_ok());
					an_assert_immediate(nextPlacement.get_scale() != 0.0f);

					vrAnchor.normalise_orientation(); // we don't want to accumulate errors
				}
				else
				{
					basedOn.clear();
					basedOnDoneAtLoc.clear();
				}
			}
		}

		if (beAlwaysAtVRAnchorLevel)
		{
			// do "keep at vr anchor level" here as if the base moved vr anchor, we're already moved it
			// we need to have all relevant placements at the right level, this means:
			// - placement - where we are right now
			// - nextPlacement - where we are going to be
			// - forcedNextPlacement - the safe place where we can move back in case something gets wrong (in most cases this is just placement, differs for moving base, where it considers that it moves)
			todo_note(TXT("adjusting placement like this, is a bit unsafe if we would have elevators going up or down and moving through door. but if we don't do this, we might be hitting walls incorrectly"));

			Vector3 prevnp = nextPlacement.get_translation();

			BeAtVRAnchorLevel::update(vrAnchor, REF_ placement);
			BeAtVRAnchorLevel::update(vrAnchor, REF_ nextPlacement);
			BeAtVRAnchorLevel::update(vrAnchor, REF_ forcedNextPlacement);
		}

		if (forceLocationDisplacementVRAnchor.is_set() ||
			forceOrientationDisplacementVRAnchor.is_set())
		{
			Transform forcedDisplacement(forceLocationDisplacementVRAnchor.get(Vector3::zero), forceOrientationDisplacementVRAnchor.get(Quat::identity));

			Transform prevForcedNextPlacement = forcedNextPlacement;

			forcedNextPlacement.set_translation(forcedNextPlacement.get_translation() + forcedDisplacement.get_translation());
			forcedNextPlacement.set_orientation(forcedNextPlacement.get_orientation().to_world(forcedDisplacement.get_orientation()));
			nextPlacement.set_translation(nextPlacement.get_translation() + forcedDisplacement.get_translation());
			nextPlacement.set_orientation(nextPlacement.get_orientation().to_world(forcedDisplacement.get_orientation()));

			{
				Transform vrAnchorRelative = prevForcedNextPlacement.to_local(vrAnchor);
				vrAnchor = forcedNextPlacement.to_world(vrAnchorRelative);
			}

			if (forceOrientationDisplacementVRAnchor.is_set())
			{
				if (ModuleController* controller = modulesOwner->get_controller())
				{
					if (controller->is_requested_look_orientation_set())
					{
						if (controller->get_maintain_relative_requested_look_orientation().get(false))
						{
							// leave as it is
						}
						else
						{
							Quat look = controller->get_requested_look_orientation();
							Quat lookRelative = prevForcedNextPlacement.get_orientation().to_local(look);
							look = forcedNextPlacement.get_orientation().to_world(lookRelative);
							controller->set_requested_look_orientation(look);
						}
					}
				}
			}

			forceLocationDisplacementVRAnchor.clear();
			forceOrientationDisplacementVRAnchor.clear();
		}

		vrAnchorLastDisplacement = prevVRAnchor.to_local(vrAnchor);

		// we prefer to use both at the same time as otherwise, if switching between we could lose some precision
		// if you update one, update the other
		Transform movePlacement = placement;
		Transform moveNextPlacement = nextPlacement;
		Vector3 movePlacementOffset = movePlacement.vector_to_world(prevMoveCentreOffsetOS);
		Vector3 moveNextPlacementOffset = moveNextPlacement.vector_to_world(moveCentreOffsetOS.get().get_translation());
		movePlacement.set_translation(movePlacement.get_translation() + movePlacementOffset);
		moveNextPlacement.set_translation(moveNextPlacement.get_translation() + moveNextPlacementOffset);

		an_assert_immediate(moveNextPlacement.is_ok());
		an_assert_immediate(moveNextPlacement.get_scale() != 0.0f);

		bool allowCheckGoingThroughCollision = true;
		todo_note(TXT("same problem as with adjusting placement above, based on could be moved already"));
		if (keepWithinSafeVolumeOnLockedBase)
		{
			if (basedOn.is_set())
			{
				// move by "based on"'s movement (between prev and next placements)
				if (auto const* basePresence = basedOn.get_presence())
				{
					if (basePresence->is_locked_as_base(this))
					{
						MEASURE_PERFORMANCE(keepWithinSafeVolumeOnLockedBase);

#ifdef DEBUG_MOVEMENT_FOR_VR_ELEVATOR_ISSUES
						output(TXT("[vr_el_is] keepWithinSafeVolumeOnLockedBase moveNextPlacement = %S"), moveNextPlacement.get_translation().to_string().to_char());
#endif
						if (basePresence->get_in_room() == get_in_room()) // we depend on base being in the same room
						{
							auto& sv = basePresence->get_safe_volume();
							if (sv.is_set())
							{
								allowCheckGoingThroughCollision = false; // no need to do this
								Vector3 locWS = moveNextPlacement.get_translation();
								Transform bsPlacement = basePresence->placement;
								Vector3 locBS = bsPlacement.location_to_local(locWS);
								Vector3 locBSClamped = sv.get().clamp(locBS);
								locWS = bsPlacement.location_to_world(locBSClamped);
#ifdef DEBUG_MOVEMENT_FOR_VR_ELEVATOR_ISSUES
								if (locBS != locBSClamped)
								{
									output(TXT("[vr_el_is] CLAMPED LOC TO SAFE VOLUME %S -> %S"), locBS.to_string().to_char(), locBSClamped.to_string().to_char());
									output(TXT("[vr_el_is] safe volume %S"), sv.get().to_string().to_char());
									output(TXT("[vr_el_is] locws clamped %S"), locWS.to_string().to_char());
								}
#endif
								moveNextPlacement.set_translation(locWS);
								nextPlacement.set_translation(locWS - moveNextPlacementOffset);
							}
						}
					}
				}
			}
		}

		// this bit is not safe as while we move here, other things move too, it is possible that the base moves not in sync with use and we may move through a wall
		if (allowCheckGoingThroughCollision)
		{
			if (auto* collision = get_owner()->get_collision())
			{
				// alright, this is mostly for player, I could setup checkGoingThroughCollision for all on locked base but most of the things have actual collisions
				// player in VR doesn't have, that's why we need explicit shouldAvoidGoingThroughCollisionForVROnLockedBase
				// checking against collision prevents us going into regions of the world that should not be accessible to us
				// but it is still possible to go through
				// also, due to this for VR we have a mechanism to keep within base's safe volume
				bool checkGoingThroughCollision = shouldAvoidGoingThroughCollision;
				if (shouldAvoidGoingThroughCollisionForSlidingLocomotion)
				{
					if (Game::is_using_sliding_locomotion())
					{
						checkGoingThroughCollision = true;
#ifdef DEBUG_MOVEMENT_FOR_VR_ELEVATOR_ISSUES
						output(TXT("[vr_el_is] shouldAvoidGoingThroughCollisionForSlidingLocomotion"));
#endif
					}
				}
				if (shouldAvoidGoingThroughCollisionForVROnLockedBase
#ifndef TEST_NOT_SLIDING_LOCOMOTION
					&& is_vr_movement()
#endif
					)
				{
					// use avoidance for going through collision only if locked as base (elevators!)
					// this is because the system is not bulletproof and sometimes the character could be locked beyond collision
					todo_future(TXT("have a more proof way to solve this problem (shouldAvoidGoingThroughCollisionForVROnLockedBase)"));
					if (auto* bop = basedOn.get_presence())
					{
						if (bop->is_locked_as_base(this))
						{
							checkGoingThroughCollision = true;
#ifdef DEBUG_MOVEMENT_FOR_VR_ELEVATOR_ISSUES
							output(TXT("[vr_el_is] shouldAvoidGoingThroughCollisionForVROnLockedBase"));
#endif
						}
					}
				}
				bool preventPushingThroughCollision = false;
				if (!checkGoingThroughCollision)
				{
					if (collision->should_prevent_pushing_through_collision())
					{
						preventPushingThroughCollision = true;
						checkGoingThroughCollision = true;
					}
				}
				if (checkGoingThroughCollision)
				{
					MEASURE_PERFORMANCE(checkGoingThroughCollision);

					bool wouldGoThroughCollision = false;

					// start at where we would be relatively to the base, this is to avoid collisions with stuff on the base
					// if base is not moving, this should be the same

					// calculate offsets as we will be interpolating lots of things
					Vector3 startOffsetWS = movePlacementOffset;
					Vector3 endOffsetWS = moveNextPlacementOffset;

					Vector3 startLocWS = forcedNextPlacement.get_translation() + startOffsetWS;
					Vector3 endLocWS = moveNextPlacement.get_translation();

					Vector3 safeLocWS = endLocWS;
					Vector3 safeOffsetWS = endOffsetWS;

					float const keepOffWallDistance = 0.01f;

					int triesLeft = 2; // allow stop and single slide
					while (triesLeft > 0)
					{
						MEASURE_PERFORMANCE(traceCollisions);
						--triesLeft;
						CollisionTrace collisionTrace(CollisionTraceInSpace::WS);
						collisionTrace.add_location(startLocWS);
						collisionTrace.add_location(endLocWS);

						int collisionTraceFlags = CollisionTraceFlag::ResultNotRequired;
						CheckCollisionContext checkCollisionContext;
						checkCollisionContext.ignore_temporary_objects();
						checkCollisionContext.use_collision_flags(collision->get_collides_with_flags());
						checkCollisionContext.avoid(get_owner());
						collision->add_not_colliding_with_to(checkCollisionContext);

						CheckSegmentResult result;

						if (get_owner()->get_presence()->trace_collision(AgainstCollision::Movement, collisionTrace, result, collisionTraceFlags, checkCollisionContext))
						{
							// for some reason we might be embedded in the collision, ignore such a case
							if (result.hitLocation != startLocWS)
							{
								wouldGoThroughCollision = true;

								float totalDistanceAlongNormal = Vector3::dot(result.hitNormal, startLocWS - endLocWS);
								if (totalDistanceAlongNormal >= 0.0f) // we could have negative values if we're inside
								{
									float hitDistanceAlongNormal = Vector3::dot(result.hitNormal, startLocWS - result.hitLocation);
									float safeHitDistanceAlongNormal = max(0.0f, hitDistanceAlongNormal - keepOffWallDistance);
									float beAtT = totalDistanceAlongNormal != 0.0f ? safeHitDistanceAlongNormal / totalDistanceAlongNormal : 0.0f;

									// safe loc should be where we hit in a safe distance
									safeLocWS = lerp(beAtT, startLocWS, endLocWS);
									safeOffsetWS = lerp(beAtT, startOffsetWS, endOffsetWS);
									startLocWS = safeLocWS;
									startOffsetWS = safeOffsetWS;
									endLocWS = startLocWS + (endLocWS - startLocWS).drop_using_normalised(result.hitNormal);

									continue;
								}

								// else we break and end up with new start loc
							}
						}
						else if (wouldGoThroughCollision)
						{
							// we didn't hit anything but we detected we could go through collision already
							// we should update safe loc then
							safeLocWS = endLocWS;
							safeOffsetWS = endOffsetWS;
						}

						// we're good
						break;
					}

					if (wouldGoThroughCollision)
					{
						if (preventPushingThroughCollision)
						{
							collision->prevent_future_current_pushing_through_collision();
						}
						nextPlacement = forcedNextPlacement; // restore to forcedNextPlacement which is placement with "based on" movement
						// but use safeLocWS
						moveNextPlacementOffset = safeOffsetWS;
						nextPlacement.set_translation(safeLocWS - moveNextPlacementOffset);
						moveNextPlacement = nextPlacement;
						moveNextPlacement.set_translation(safeLocWS);
						an_assert_immediate(nextPlacement.is_ok());
						an_assert_immediate(nextPlacement.get_scale() != 0.0f);
#ifdef DEBUG_MOVEMENT_FOR_VR_ELEVATOR_ISSUES
						output(TXT("[vr_el_is] would go through collision, move back to %S"), nextPlacement.get_translation().to_string().to_char());
						output(TXT("[vr_el_is] moveNextPlacement = %S"), moveNextPlacement.get_translation().to_string().to_char());
#endif
					}
				}
			}
		}

		if (beAlwaysAtVRAnchorLevel)
		{
			Vector3 prevnp = nextPlacement.get_translation();
			// make sure we land at the right place
			BeAtVRAnchorLevel::update(vrAnchor, REF_ nextPlacement);

			moveNextPlacement = nextPlacement;
			moveNextPlacement.set_translation(moveNextPlacement.get_translation() + moveNextPlacementOffset);
		}

#ifdef DEBUG_MOVEMENT_FOR_VR_ELEVATOR_ISSUES
		if (keepWithinSafeVolumeOnLockedBase)
		{
			output(TXT("[vr_el_is] vr anchor at %S"), vrAnchor.get_translation().to_string().to_char());
		}
#endif

		// move through doors
		if (movePlacement.get_translation() != moveNextPlacement.get_translation())
		{
			MEASURE_PERFORMANCE(checkMovingThroughDoors);

#ifdef DEBUG_MOVEMENT_FOR_VR_ELEVATOR_ISSUES
			if (keepWithinSafeVolumeOnLockedBase)
			{
				output(TXT("[vr_el_is] check moving through doors, from %S to %S"), movePlacement.get_translation().to_string().to_char(), moveNextPlacement.get_translation().to_string().to_char());
			}
#endif
			Room* intoRoom;
			Transform intoRoomTransform;
			DoorInRoom* exitThrough;
			DoorInRoom* enterThrough;
			int const throughDoorsLimit = 32;
			allocate_stack_var(Framework::DoorInRoom const*, throughDoors, throughDoorsLimit);
			Framework::DoorInRoomArrayPtr throughDoorsArrayPtr(throughDoors, throughDoorsLimit);
			if (can_change_rooms() && inRoom->move_through_doors(movePlacement, moveNextPlacement, OUT_ intoRoom, OUT_ intoRoomTransform, OUT_ & exitThrough, OUT_ & enterThrough, OUT_ & throughDoorsArrayPtr))
			{
				if (basedOn.get_presence() && basedOn.get_presence()->is_locked_as_base(this) && !basedOn.get_presence()->is_base_through_doors())
				{
					// if we're on a locked base, we should not move through doors (unless we allow that, but it has to be explicit)
					// if this happens, try to move back into the room and within the door's hole
					// we should never get behind the door as either collisions or explicit "stay" should kick in
					if (exitThrough)
					{
						Plane doorPlane = exitThrough->get_plane(); // normal is inbound
						//float placementInFront = doorPlane.get_in_front(placement.get_translation());
						float forcedNextPlacementInFront = doorPlane.get_in_front(forcedNextPlacement.get_translation());
						float minInFront = 0.01f;
						if (forcedNextPlacementInFront < minInFront)
						{
							// remain in front of door and within hole (to avoid going through nearby collision )
							Vector3 loc = forcedNextPlacement.get_translation();
							loc = exitThrough->find_inside_hole(loc, 0.05f);
							forcedNextPlacement.set_translation(loc + doorPlane.get_normal() * minInFront);
						}
					}
#ifdef DEBUG_MOVEMENT_FOR_VR_ELEVATOR_ISSUES
					output(TXT("[vr_el_is] would change doors, move back to %S"), forcedNextPlacement.get_translation().to_string().to_char());
#endif
					nextPlacement = forcedNextPlacement;
					moveNextPlacement = nextPlacement;
					moveNextPlacement.set_translation(moveNextPlacement.get_translation() + moveNextPlacementOffset);
					an_assert_immediate(nextPlacement.is_ok());
					an_assert_immediate(nextPlacement.get_scale() != 0.0f);
				}
				else
				{
					change_room(intoRoom, intoRoomTransform, exitThrough, enterThrough, &throughDoorsArrayPtr, NAME(moves));
				}
			}
		}

		// so we will always have orientation normalised
		nextPlacement.normalise_orientation();

		// store offset to previous placement
		prevPlacementOffset = nextPlacement.to_local(placement);
		prevPlacementOffsetDeltaTime = Game::get()->get_delta_time();

#ifdef DEBUG_MOVEMENT_FOR_VR_ELEVATOR_ISSUES
		if (keepWithinSafeVolumeOnLockedBase)
		{
			output(TXT("[vr_el_is] moved from %S to %S"), placement.get_translation().to_string().to_char(), nextPlacement.get_translation().to_string().to_char());
		}
#endif
		
		// update placement to next
		placement = nextPlacement;
		an_assert_immediate(placement.is_ok());
		an_assert_immediate(placement.get_scale() != 0.0f);

		if (ModuleController * controller = modulesOwner->get_controller())
		{
			if (controller->is_requested_look_orientation_set())
			{
				if (controller->get_maintain_relative_requested_look_orientation().get(false))
				{
					// restore relative requested look orientation if new placement
					controller->set_relative_requested_look_orientation(requestedRelativeLook.get_orientation());
				}
				else if (!Quat::same_orientation(baseRotation, Quat::identity, 0.0f))
				{
					Quat relativeLook = placement.get_orientation().to_local(controller->get_requested_look_orientation());
					relativeLook = baseRotation.to_world(relativeLook);
					controller->set_requested_look_orientation(placement.get_orientation().to_world(relativeLook));
				}
			}
		}

		post_placement_change();
	}

#ifdef BUILD_NON_RELEASE
	if (auto* o = get_owner()->get_as_object())
	{
		DEFINE_STATIC_NAME(logPlacement);
		if (o->get_tags().get_tag(NAME(logPlacement)))
		{
			output(TXT("[presence] %S is at %S, velocity %S"), get_owner()->ai_get_name().to_char(), placement.get_translation().to_string().to_char(), velocityLinear.to_string().to_char());
		}
	}
#endif

#ifdef WITH_PRESENCE_INDICATOR
	update_presence_indicator();
#endif

	if (provideLinearSpeedVar.is_valid())
	{
		provideLinearSpeedVar.access<float>() = velocityLinear.length();
	}

	if (is_base_through_doors())
	{
		Concurrency::ScopedSpinLock lock(basedLock, true);

		for_every_ptr(b, based)
		{
			b->get_presence()->do_move(b);
		}
	}
}

Transform ModulePresence::transform_based_movement_placement(Transform const & _basedNextPlacement) const
{
	Transform const localBNP = preMovePlacement.to_local(_basedNextPlacement);
	Transform thisPreparedMovePlacement = preparedMovePlacement;
	if (auto const * basePresence = basedOn.get_presence())
	{
		an_assert(basePresence->get_in_room() == get_in_room(), TXT("we depend on base being in the same room"));
		basePresence->transform_based_movement_placement(REF_ thisPreparedMovePlacement);
	}
	return thisPreparedMovePlacement.to_world(localBNP);
}

void ModulePresence::advance__post_move(IMMEDIATE_JOB_PARAMS)
{
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		MEASURE_PERFORMANCE(postMove);
		if (ModulePresence* self = modulesOwner->get_presence())
		{
			self->post_move(modulesOwner);
		}
		else
		{
			an_assert(false, TXT("shouldn't be called"));
		}
	}
}

void ModulePresence::store_velocity_if_required()
{
	if (s_ignoreAutoVelocities)
	{
		// to get rid of old velocities
		prevVelocities.pop_back();
		return;
	}
	if (storeVelocities > 0)
	{
		if (prevVelocities.get_size() < storeVelocities)
		{
			prevVelocities.push_front(velocityLinear);
		}
		else
		{
			for (int i = storeVelocities - 1; i > 0; --i)
			{
				prevVelocities[i] = prevVelocities[i - 1];
			}
			prevVelocities[0] = velocityLinear;
		}
	}
	else
	{
		prevVelocities.set_size(storeVelocities);
	}
}

bool ModulePresence::does_require_forced_prepare_move() const
{
	return requiresForcedUpdateLookViaController;
}

bool ModulePresence::does_require_post_move() const
{
	if (!ceaseToExistWhenNotAttached && immuneToKillGravity)
	{
		return false;
	}
	--postMoveFrameInterval;
	if (postMoveFrameInterval < 0)
	{
		// can be that sparse as it is only for killZ
		postMoveFrameInterval = Random::get_int(4);
		float vl = velocityLinear.length();
		if (vl > 2.0f)
		{
			postMoveFrameInterval = 0;
		}
		else if (vl > 1.0f)
		{
			postMoveFrameInterval = Random::get_int(2);
		}
	}

	if (get_owner() && ceaseToExistWhenNotAttached && !is_attached())
	{
		return true; // require post move to cease to exist
	}
	if (postMoveFrameInterval == 0)
	{
		if (get_in_room() && get_owner() && get_owner()->is_autonomous() &&
			fast_cast<TemporaryObject>(get_owner()) == nullptr &&
			!immuneToKillGravity)
		{
			if (cachedKillGravityForRoom != get_in_room())
			{
				cachedKillGravityForRoom = get_in_room();
				get_in_room()->update_kill_gravity_distance(cachedKillGravityDir, cachedKillGravityDistance);
			}
			if (cachedKillGravityDir.is_set() && cachedKillGravityDistance.is_set())
			{
				return true;
			}
		}
	}
	return false; 
}

void ModulePresence::post_move(IModulesOwner * modulesOwner)
{
	if (get_owner() && ceaseToExistWhenNotAttached && !is_attached())
	{
#ifdef DEBUG_WORLD_LIFETIME
		output(TXT("ceaseToExistWhenNotAttached o%p"), get_owner());
#endif
		get_owner()->cease_to_exist(true);
		return;
	}

	// if attached, should wait till the owner detaches it and only then it should die
	if (! is_attached() &&
		get_in_room() && get_owner() && get_owner()->is_autonomous() &&
		fast_cast<TemporaryObject>(get_owner()) == nullptr &&
		! immuneToKillGravity)
	{
		if (cachedKillGravityForRoom != get_in_room())
		{
			cachedKillGravityForRoom = get_in_room();
			get_in_room()->update_kill_gravity_distance(cachedKillGravityDir, cachedKillGravityDistance);
		}
		if (cachedKillGravityDir.is_set() && cachedKillGravityDistance.is_set())
		{
			an_assert(cachedKillGravityDir.get().is_normalised());
			float alongDir = Vector3::dot(get_placement().get_translation(), cachedKillGravityDir.get());
			if (alongDir > cachedKillGravityDistance.get())
			{
#ifdef DEBUG_WORLD_LIFETIME
				output(TXT("killed by gravity o%p alongDir = %.3f > %.3f cachedKillGravityDistance.get() %S"), get_owner(), alongDir, cachedKillGravityDistance.get(),
					get_placement().get_translation().to_string().to_char());
#endif
				if (auto* g = Game::get())
				{
					if (g->kill_by_gravity(get_owner()))
					{
						return;
					}
				}
				get_owner()->cease_to_exist(true);
				return;
			}
		}
	}
}

void ModulePresence::calculate_final_pose_and_attachments_1(float _deltaTime, PoseAndAttachmentsReason::Type _reason)
{
	MEASURE_PERFORMANCE(calculate_final_pose_and_attachments_1);
	if (has_attachments())
	{
		MEASURE_PERFORMANCE(attachements);
		Concurrency::ScopedSpinLock lock(attachedLock);

		scoped_call_stack_info(TXT("attachements"));
		for_every_ptr(at, attached)
		{
			scoped_call_stack_info(at->ai_get_name().to_char());
			if (auto* atP = at->get_presence())
			{
				scoped_call_stack_info(TXT("attached with presence"));
				atP->do_attachment_movement(_deltaTime, _reason);
				if (at->should_do_calculate_final_pose_attachment_with(get_owner()))
				{
					ModuleAppearance::calculate_final_pose_and_attachments_0(at, _deltaTime, _reason);
				}
				if (atP->is_attached_via_socket())
				{
					atP->do_attachment_movement(_deltaTime, PoseAndAttachmentsReason::SecondMovementForActiveSocket);
				}
			}
		}
	}
	Optional<Vector3> proposedPresenceLinkCentreOS;
	Optional<float> proposedPresenceLinkRadius;
	Optional<Vector3> proposedPresenceLinkCentreDistance;
	if (!updatePresenceLinkSetupContinuously)
	{
		updatePresenceLinkSetup = false;
	}
	if (updatePresenceLinkSetupFromMovementCollisionPrimitive || updatePresenceLinkSetupFromMovementCollisionPrimitiveAsSphere)
	{
		if (auto* mc = get_owner()->get_collision())
		{
			proposedPresenceLinkCentreOS = mc->get_collision_primitive_centre_offset();
			proposedPresenceLinkRadius = mc->get_collision_primitive_radius();
			proposedPresenceLinkCentreDistance = mc->get_collision_primitive_centre_distance();

			if (updatePresenceLinkSetupFromMovementCollisionPrimitiveAsSphere && proposedPresenceLinkCentreDistance.is_set())
			{
				proposedPresenceLinkRadius = proposedPresenceLinkRadius.get() + proposedPresenceLinkCentreDistance.get().length() * 0.5f;
				proposedPresenceLinkCentreDistance.clear();
			}
		}
	}
	if (!proposedPresenceLinkCentreOS.is_set())
	{
		Optional<Range3> newBBox;
		if (updatePresenceLinkSetupFromMovementCollisionBoundingBox)
		{
			scoped_call_stack_info(TXT("updatePresenceLinkSetupFromMovementCollisionBoundingBox"));
			if (auto* collision = get_owner()->get_collision())
			{
				newBBox = collision->get_movement_collision().get_bounding_box();
			}
		}
		else if (updatePresenceLinkSetupFromPreciseCollisionBoundingBox)
		{
			scoped_call_stack_info(TXT("updatePresenceLinkSetupFromPreciseCollisionBoundingBox"));
			if (auto* collision = get_owner()->get_collision())
			{
				newBBox = collision->get_precise_collision().get_bounding_box();
			}
		}
		else if (updatePresenceLinkSetupFromAppearancesBonesBoundingBox)
		{
			updatePresenceLinkSetup = true;
			scoped_call_stack_info(TXT("updatePresenceLinkSetupFromAppearancesBonesBoundingBox"));
			if (auto* appearance = get_owner()->get_appearance())
			{
				newBBox = appearance->get_bones_bounding_box();
			}
		}
		if (newBBox.is_set())
		{
			proposedPresenceLinkCentreOS = newBBox.get().centre();
			proposedPresenceLinkRadius = newBBox.get().length().length() * 0.5f;
		}
	}
	if (proposedPresenceLinkCentreOS.is_set() &&
		proposedPresenceLinkRadius.is_set())
	{
		Transform newPresenceLinkCentreOS = Transform(proposedPresenceLinkCentreOS.get(), Quat::identity);
		if (!presenceInSingleRoom &&
			newPresenceLinkCentreOS.get_translation() != presenceLinkCentreOS.get_translation())
		{
			scoped_call_stack_info(TXT("move through doors"));
			Room* intoRoom;
			Transform intoRoomPlacement;
			DoorInRoom* exitThroughDoor;
			DoorInRoom* enterThroughDoor;
			int const throughDoorsLimit = 32;
			allocate_stack_var(Framework::DoorInRoom const*, throughDoors, throughDoorsLimit);
			Framework::DoorInRoomArrayPtr throughDoorsArrayPtr(throughDoors, throughDoorsLimit);
			if (inRoom && inRoom->move_through_doors(placement.to_world(presenceLinkCentreOS), placement.to_world(newPresenceLinkCentreOS), OUT_ intoRoom, OUT_ intoRoomPlacement, &exitThroughDoor, &enterThroughDoor, &throughDoorsArrayPtr))
			{
				change_room(intoRoom, intoRoomPlacement, exitThroughDoor, enterThroughDoor, &throughDoorsArrayPtr);
			}
		}
		presenceLinkCentreOS = newPresenceLinkCentreOS;
		presenceLinkCentreDistance = proposedPresenceLinkCentreDistance.get(Vector3::zero);
		presenceLinkRadius = proposedPresenceLinkRadius.get() + updatePresenceLinkSetupMakeBigger;
	}
}

bool ModulePresence::is_attached_at_all_to(IModulesOwner const * _to) const
{
	ModulePresence const * check = this;
	while (check)
	{
		if (check->attachment.attachedTo == _to)
		{
			return true;
		}
		check = check->attachment.attachedTo? check->attachment.attachedTo->get_presence() : nullptr;
	}
	return false;
}

void ModulePresence::detach()
{
	Concurrency::ScopedSpinLock lock(attachmentLock, true, TXT("detaching"));
	if (attachment.attachedTo)
	{
#ifdef INSPECT_ATTACHMENTS
		if (!fast_cast<TemporaryObject>(get_owner()))
		{
			output(TXT("[ATTACH] detach o%p from o%p"), get_owner(), attachment.attachedTo);
		}
#endif
		Concurrency::ScopedSpinLock lock(attachment.attachedTo->get_presence()->attachedLock);
#ifdef INSPECT_ATTACHMENTS
		if (!fast_cast<TemporaryObject>(get_owner()))
		{
			output(TXT("[ATTACH] list of attached to o%p (pre detach):"), attachment.attachedTo);
			for_every_ptr(at, attachment.attachedTo->get_presence()->attached)
			{
				output(TXT("[ATTACH]   o%p:"), at);
			}
		}
#endif
		attachment.attachedTo->get_presence()->attached.remove(get_owner());
#ifdef INSPECT_ATTACHMENTS
		if (!fast_cast<TemporaryObject>(get_owner()))
		{
			output(TXT("[ATTACH] list of attached to o%p (post detach):"), attachment.attachedTo);
			for_every_ptr(at, attachment.attachedTo->get_presence()->attached)
			{
				output(TXT("[ATTACH]   o%p:"), at);
			}
		}
#endif
	}
	attachment = Attachment();
}

void ModulePresence::issue_reattach()
{
	if (attachment.attachedTo)
	{
		attachment.pathToAttachedTo.reset();
	}
}

void ModulePresence::attach_to(IModulesOwner* _owner, bool _following, Transform const & _relativePlacement, bool _doAttachmentMovement)
{
	assert_we_may_move();

	detach();

	Concurrency::ScopedSpinLock lock(attachmentLock, true, TXT("attaching"));

	an_assert(_owner);
	an_assert(!canBeBase, TXT("not supported"));
	an_assert(!canBeVolumetricBase, TXT("not supported"));
	an_assert(_owner->does_allow_to_attach(get_owner()), TXT("can't attach like that"));

#ifdef INSPECT_ATTACHMENTS
	if (!fast_cast<TemporaryObject>(get_owner()))
	{
		output(TXT("[ATTACH] attach o%p to o%p"), get_owner(), _owner);
	}
#endif

	{
		Concurrency::ScopedSpinLock lock(_owner->get_presence()->attachedLock);
#ifdef INSPECT_ATTACHMENTS
		if (!fast_cast<TemporaryObject>(get_owner()))
		{
			output(TXT("[ATTACH] list of attached to o%p (pre attach):"), _owner);
			for_every_ptr(at, _owner->get_presence()->attached)
			{
				output(TXT("[ATTACH]   o%p:"), at);
			}
		}
#endif
		_owner->get_presence()->attached.push_back(get_owner());
#ifdef INSPECT_ATTACHMENTS
		if (!fast_cast<TemporaryObject>(get_owner()))
		{
			output(TXT("[ATTACH] list of attached to o%p (post attach):"), _owner);
			for_every_ptr(at, _owner->get_presence()->attached)
			{
				output(TXT("[ATTACH]   o%p:"), at);
			}
		}
#endif
	}
	cease_to_exist_when_not_attached(_owner->get_presence()->should_cease_to_exist_when_not_attached(), CeaseToExistWhenNotAttachedFlag::Auto);

	attachment = Attachment();
	attachment.attachedTo = _owner;
	attachment.isFollowing = _following;
	attachment.relativePlacement = _relativePlacement;
	an_assert_immediate(attachment.relativePlacement.is_ok());

	if (_doAttachmentMovement)
	{
		do_post_attach_to_movement();
	}
}

void ModulePresence::request_attach(IModulesOwner* _object, bool _following, Transform const& _relativePlacement, bool _doAttachmentMovement)
{
	Concurrency::ScopedSpinLock lock(attachmentRequestLock);
	attachmentRequest.type = AttachmentRequest::AttachToUsingRelativePlacement;
	attachmentRequest.toObject = _object;
	attachmentRequest.toBone = Name::invalid();
	attachmentRequest.toSocket = Name::invalid();
	attachmentRequest.following = _following;
	attachmentRequest.placement = _relativePlacement;
	attachmentRequest.doAttachmentMovement = _doAttachmentMovement;
}

void ModulePresence::request_attach_to_using_in_room_placement(IModulesOwner* _object, bool _following, Transform const & _inRoomPlacement, bool _doAttachmentMovement)
{
	Concurrency::ScopedSpinLock lock(attachmentRequestLock);
	attachmentRequest.type = AttachmentRequest::AttachToUsingInRoomPlacement;
	attachmentRequest.toObject = _object;
	attachmentRequest.toBone = Name::invalid();
	attachmentRequest.toSocket = Name::invalid();
	attachmentRequest.following = _following;
	attachmentRequest.placement = _inRoomPlacement;
	attachmentRequest.doAttachmentMovement = _doAttachmentMovement;
}

void ModulePresence::request_attach_to_bone_using_in_room_placement(IModulesOwner* _object, Name const & _bone, bool _following, Transform const & _inRoomPlacement, bool _doAttachmentMovement)
{
	Concurrency::ScopedSpinLock lock(attachmentRequestLock);
	attachmentRequest.type = AttachmentRequest::AttachToUsingInRoomPlacement;
	attachmentRequest.toObject = _object;
	attachmentRequest.toBone = _bone;
	attachmentRequest.toSocket = Name::invalid();
	attachmentRequest.following = _following;
	attachmentRequest.placement = _inRoomPlacement;
	attachmentRequest.doAttachmentMovement = _doAttachmentMovement;
}

void ModulePresence::request_attach_to_socket_using_in_room_placement(IModulesOwner* _object, Name const & _socket, bool _following, Transform const & _inRoomPlacement, bool _doAttachmentMovement)
{
	Concurrency::ScopedSpinLock lock(attachmentRequestLock);
	attachmentRequest.type = AttachmentRequest::AttachToUsingInRoomPlacement;
	attachmentRequest.toObject = _object;
	attachmentRequest.toBone = Name::invalid();
	attachmentRequest.toSocket = _socket;
	attachmentRequest.following = _following;
	attachmentRequest.placement = _inRoomPlacement;
	attachmentRequest.doAttachmentMovement = _doAttachmentMovement;
}

void ModulePresence::request_attach_to_socket(IModulesOwner* _object, Name const& _socket, bool _following, Transform const& _relativePlacement, bool _doAttachmentMovement)
{
	Concurrency::ScopedSpinLock lock(attachmentRequestLock);
	attachmentRequest.type = AttachmentRequest::AttachToUsingRelativePlacement;
	attachmentRequest.toObject = _object;
	attachmentRequest.toBone = Name::invalid();
	attachmentRequest.toSocket = _socket;
	attachmentRequest.following = _following;
	attachmentRequest.placement = _relativePlacement;
	attachmentRequest.doAttachmentMovement = _doAttachmentMovement;
}

void ModulePresence::attach_to_using_in_room_placement(IModulesOwner* _object, bool _following, Transform const & _inRoomPlacement, bool _doAttachmentMovement)
{
	assert_we_may_move();
	an_assert(_object);
	attach_to(_object, _following, _object->get_presence()->get_placement().to_local(_inRoomPlacement), _doAttachmentMovement);
}

void ModulePresence::attach_to_bone(IModulesOwner* _object, Name const & _bone, bool _following, Transform const & _relativePlacement, bool _doAttachmentMovement)
{
	assert_we_may_move();
	attach_to(_object, _following, _relativePlacement);
	attachment.toBone = _bone;
	if (auto * appearance = _object->get_appearance())
	{
		attachment.toBoneIdx = NONE;
		if (auto* fs = appearance->get_skeleton())
		{
			attachment.toBoneIdx = fs->get_skeleton()->find_bone_index(_bone);
		}
	}

	if (_doAttachmentMovement)
	{
		do_post_attach_to_movement();
	}
}

void ModulePresence::attach_to_bone_using_in_room_placement(IModulesOwner* _object, Name const & _bone, bool _following, Transform const & _inRoomPlacement, bool _doAttachmentMovement)
{
	if (!_bone.is_valid())
	{
		attach_to_using_in_room_placement(_object, _following, _inRoomPlacement, _doAttachmentMovement);
		return;
	}
	assert_we_may_move();
	an_assert(_object);
	if (auto* appearance = _object->get_appearance())
	{
		if (auto* fs = appearance->get_skeleton())
		{
			int boneIdx = fs->get_skeleton()->find_bone_index(_bone);

			Transform boneInRoomPlacement = appearance->get_ms_to_ws_transform().to_world(appearance->get_final_pose_MS()->get_bone(boneIdx));
			attach_to_bone_index(_object, boneIdx, _following, boneInRoomPlacement.to_local(_inRoomPlacement), _doAttachmentMovement);
			return;
		}
	}
	an_assert(false, TXT("missing bone or appearance?"));
	attach_to_using_in_room_placement(_object, _following, _inRoomPlacement, _doAttachmentMovement);
}

void ModulePresence::attach_to_bone_index(IModulesOwner* _object, int _boneIdx, bool _following, Transform const & _relativePlacement, bool _doAttachmentMovement)
{
	assert_we_may_move();
	attach_to(_object, _following, _relativePlacement, false);
	attachment.toBoneIdx = _boneIdx;

	if (_doAttachmentMovement)
	{
		do_post_attach_to_movement();
	}
}

void ModulePresence::attach_to_socket(IModulesOwner* _object, Name const & _socket, bool _following, Transform const & _relativePlacement, bool _doAttachmentMovement)
{
	assert_we_may_move();
	if (auto const * appearance = _object->get_appearance())
	{
		if (auto const * mesh = appearance->get_mesh())
		{
			int socketIdx = mesh->find_socket_index(_socket);
			if (socketIdx != NONE)
			{
				int boneIdx;
				Transform relativePlacement;
				if (mesh->get_socket_info(socketIdx, boneIdx, relativePlacement))
				{
					relativePlacement = relativePlacement.to_world(_relativePlacement);
					attach_to_bone_index(_object, boneIdx, _following, relativePlacement, false);
					// fill socket info
					attachment.toSocket = SocketID(_socket, mesh);
					attachment.relativePlacement = _relativePlacement;
					an_assert_immediate(attachment.relativePlacement.is_ok());
					if (_doAttachmentMovement)
					{
						do_post_attach_to_movement();
					}
					return;
				}
			}
			else
			{
				error(TXT("could not find socket \"%S\""), _socket.to_char());
			}
		}
	}
	error(TXT("error attaching to socket \"%S\""), _socket.to_char());
	attach_to(_object, _following, Transform::identity, _doAttachmentMovement);
}

void ModulePresence::attach_to_socket_using_in_room_placement(IModulesOwner* _object, Name const & _socket, bool _following, Transform const & _inRoomPlacement, bool _doAttachmentMovement)
{
	assert_we_may_move();
	if (auto const * appearance = _object->get_appearance())
	{
		if (auto const * mesh = appearance->get_mesh())
		{
			int socketIdx = mesh->find_socket_index(_socket);
			if (socketIdx != NONE)
			{
				Transform socketInRoomPlacement = appearance->get_os_to_ws_transform().to_world(appearance->calculate_socket_os(socketIdx));
				Transform relativeToSocketPlacement = socketInRoomPlacement.to_local(_inRoomPlacement);
				attach_to_socket(_object, _socket, _following, relativeToSocketPlacement, _doAttachmentMovement);
				return;
			}
			else
			{
				error(TXT("could not find socket \"%S\""), _socket.to_char());
			}
		}
	}
	error(TXT("error attaching to socket \"%S\""), _socket.to_char());
	attach_to_using_in_room_placement(_object, _following, _inRoomPlacement, _doAttachmentMovement);
}

void ModulePresence::do_post_attach_to_movement()
{
	do_attachment_movement(0.0f, PoseAndAttachmentsReason::Normal);
}

void ModulePresence::attach_via_socket(IModulesOwner* _object, Name const & _viaSocket, Name const & _toSocket, bool _following, Transform const & _relativePlacement, bool _doAttachmentMovement)
{
	assert_we_may_move();
	auto const * appearance = _object->get_appearance();
	auto const * ourAppearance = get_owner()->get_appearance();
	if (appearance && ourAppearance)
	{
		auto const * mesh = appearance->get_mesh();
		auto const * ourMesh = ourAppearance->get_mesh();
		if (mesh && ourMesh)
		{
			int toSocketIdx = mesh->find_socket_index(_toSocket);
			int viaSocketIdx = ourMesh->find_socket_index(_viaSocket);
			if (toSocketIdx != NONE && viaSocketIdx != NONE)
			{
				int boneIdx;
				Transform relativePlacement;
				if (mesh->get_socket_info(toSocketIdx, boneIdx, relativePlacement))
				{
					relativePlacement = relativePlacement.to_world(_relativePlacement);
					attach_to_bone_index(_object, boneIdx, _following, relativePlacement, false);
					// fill socket info
					attachment.toSocket = SocketID(_toSocket, mesh);
					attachment.viaSocket = SocketID(_viaSocket, ourMesh);
					attachment.relativePlacement = _relativePlacement;
					an_assert_immediate(attachment.relativePlacement.is_ok());
					if (_doAttachmentMovement)
					{
						do_post_attach_to_movement();
					}
					return;
				}
			}
			else
			{
				if (toSocketIdx == NONE)
				{
					error(TXT("could not find socket \"%S\""), _toSocket.to_char());
				}
				if (viaSocketIdx == NONE)
				{
					error(TXT("could not find socket \"%S\""), _viaSocket.to_char());
				}
			}
		}
	}
	error(TXT("error attaching to socket \"%S\" via socket \"%S\""), _toSocket.to_char(), _viaSocket.to_char());
	attach_to_socket(_object, _toSocket, _following, Transform::identity, _doAttachmentMovement);
}

void ModulePresence::get_attached_placement_for_top(OUT_ Room* & _inRoom, OUT_ Transform & _placementWS, IModulesOwner* _stopAt, OUT_ IModulesOwner** _top) const
{
	Room* r = inRoom;
	Transform pWS = placement;
	assign_optional_out_param(_top, nullptr);

	auto* goUp = get_owner();
	while (goUp && goUp->get_presence()->is_attached())
	{
		REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(goUp ? goUp->ai_get_name().to_char() : TXT("--"));
		if (auto * p = goUp->get_presence()->get_path_to_attached_to())
		{
			if (p->is_active())
			{
				r = p->get_in_final_room();
				pWS = p->from_owner_to_target(pWS);
				auto* prevGoUp = goUp;
				goUp = goUp->get_presence()->get_attached_to();
				assign_optional_out_param(_top, goUp);
				if (goUp == _stopAt ||
					goUp == prevGoUp) // we got stuck
				{
					break;
				}
			}
			else
			{
				// not active? invalid? just break and hope we get back to the proper one
				break;
			}
		}
		else
		{
			break;
		}
	}

	_inRoom = r;
	_placementWS = pWS;
}

void ModulePresence::update_attachment_relative_placement(Transform const & _relativePlacement)
{
	attachment.relativePlacement = _relativePlacement;
	an_assert_immediate(attachment.relativePlacement.is_ok());
}

void ModulePresence::do_attachment_movement(float _deltaTime, PoseAndAttachmentsReason::Type _reason)
{
	an_assert_immediate(attachment.attachedTo != nullptr);

	scoped_call_stack_info(TXT("get initial info"));
	todo_note(TXT("most simple way for now!"));
	auto const* attachedTo = attachment.attachedTo->get_presence();
	Room* beInRoom = attachedTo->inRoom;
	if (!beInRoom)
	{
		if (inRoom)
		{
#ifdef INSPECT_ATTACHMENTS
			if (!fast_cast<TemporaryObject>(get_owner()))
			{
				output(TXT("[ATTACH] remove o%p from room as attached-to removed too"), get_owner());
			}
#endif
			scoped_call_stack_info(TXT("remove from room"));
			remove_from_room();
			return;
		}
	}

	scoped_call_stack_info(TXT("calculate placement of attach"));
	Transform newPlacementInAttachedToRoom = attachment.calculate_placement_of_attach(get_owner());
	an_assert_immediate(newPlacementInAttachedToRoom.is_ok());
	float invDeltaTime = _deltaTime == 0.0f ? 0.0f : 1.0f / _deltaTime;

	if (!inRoom)
	{
		scoped_call_stack_info(TXT("reset attach"));
		// remove any info about attached to
		attachment.pathToAttachedTo.reset();
	}

	scoped_call_stack_info(TXT("calculate centre"));
	Transform attachedToMovementCentreOffsetOS = attachedTo->moveCentreOffsetOS.get(attachedTo->presenceLinkCentreOS);
	Transform movementCentreOffsetOS = moveCentreOffsetOS.get(presenceLinkCentreOS);

	Transform placementChange = Transform::identity;

	bool updateVelocity = false;
	if (!s_ignoreAutoVelocities)
	{
#ifndef AN_ADVANCE_VR_IMPORTANT_ONCE
		bool isVRAttachmentNow = is_vr_attachment();
#endif
		if (_reason != PoseAndAttachmentsReason::SecondMovementForActiveSocket
#ifndef AN_ADVANCE_VR_IMPORTANT_ONCE
			&&	((!isVRAttachmentNow && _reason != PoseAndAttachmentsReason::VRImportant) ||
			     (isVRAttachmentNow && _reason == PoseAndAttachmentsReason::VRImportant))
#endif
			)
		{
			updateVelocity = true;
		}
	}

	if (updateVelocity)
	{
		scoped_call_stack_info(TXT("store velocity before it gets updated"));
		store_velocity_if_required();
	}

	// check if doors are not shut, so we could loose attachments
	if (!attachment.pathToAttachedTo.is_empty())
	{
		bool doorsClosed = false;
		for_every_ref(door, attachment.pathToAttachedTo.get_through_doors())
		{
			if (!door || !door->get_door() || door->get_door()->is_closed())
			{
				doorsClosed = true;
				break;
			}
		}
		if (doorsClosed)
		{
			attachment.pathToAttachedTo.clear_target();
		}
	}

	{	// always move as following, because we want to trace doors we moved through
		scoped_call_stack_info(TXT("do following (always)"));
		Transform startTracePlacementOffset = placement.to_world(movementCentreOffsetOS); // use current placement and our centre offset
		if (!attachment.pathToAttachedTo.is_active() ||
			!attachment.pathToAttachedTo.is_path_valid())
		{
#ifdef INSPECT_ATTACHMENTS
			if (!fast_cast<TemporaryObject>(get_owner()))
			{
				/*spam*/ //output(TXT("[ATTACH] attachment path for o%p is invalid (maybe was not set at all? could happen right after attachment)"), get_owner());
			}
#endif
			if (inRoom != beInRoom)
			{
				scoped_call_stack_info(TXT("change room"));
				if (inRoom)
				{
					scoped_call_stack_info(TXT("remove from room"));
					remove_from_room();
				}
				{
					scoped_call_stack_info(TXT("add to room"));
					add_to_room(beInRoom);
				}
			}
			scoped_call_stack_info(TXT("update attachement"));
			placement = attachedTo->placement;
			an_assert_immediate(placement.is_ok());
			an_assert_immediate(placement.get_scale() != 0.0f);
			startTracePlacementOffset = attachedTo->placement.to_world(attachedToMovementCentreOffsetOS); // use attached to placement and offset
			nextPlacement = placement;
			an_assert_immediate(nextPlacement.is_ok());
			an_assert_immediate(nextPlacement.get_scale() != 0.0f);
			attachment.pathToAttachedTo.reset();
			attachment.pathToAttachedTo.set_owner_presence(this);
			attachment.pathToAttachedTo.set_target(attachment.attachedTo);
		}
		scoped_call_stack_info(TXT("new placement"));
		Transform newPlacementInCurrentRoom = attachment.pathToAttachedTo.from_target_to_owner(newPlacementInAttachedToRoom);
		Room* intoRoom;
		Transform intoRoomPlacement;
		DoorInRoom* exitThroughDoor;
		DoorInRoom* enterThroughDoor;
		// use this set of variables as we will be placed properly if we're not going through doors or we're get updated in change_room, if we're going through doors
		Transform currPlacement = placement;
		placement = newPlacementInCurrentRoom;
		an_assert_immediate(placement.is_ok());
		an_assert_immediate(placement.get_scale() != 0.0f);
		nextPlacement = placement;
		an_assert_immediate(nextPlacement.is_ok());
		an_assert_immediate(nextPlacement.get_scale() != 0.0f);
		if (currPlacement.get_scale() == 0.0f)
		{
			warn(TXT("%S had zero scale"), get_owner()->ai_get_name().to_char());
			currPlacement.set_scale(nextPlacement.get_scale());
		}
		placementChange = currPlacement.to_local(nextPlacement);
		if (updateVelocity)
		{
			prevPlacementOffset = nextPlacement.to_local(currPlacement);
			prevPlacementOffsetDeltaTime = _deltaTime;
			set_velocity_linear((nextPlacement.get_translation() - currPlacement.get_translation()) * invDeltaTime); // better than just attachedTo->velocityLinear
			set_velocity_rotation(currPlacement.to_local(nextPlacement).get_orientation().to_rotator() * invDeltaTime);
		}
		{
			scoped_call_stack_info(TXT("move through doors"));
			int const throughDoorsLimit = 32;
			allocate_stack_var(Framework::DoorInRoom const*, throughDoors, throughDoorsLimit);
			Framework::DoorInRoomArrayPtr throughDoorsArrayPtr(throughDoors, throughDoorsLimit);
			// use offset for move through doors
			Transform placementOffset = placement.to_world(movementCentreOffsetOS);
			scoped_call_stack_info(TXT("move through doors - now"));
			if (inRoom && inRoom->move_through_doors(startTracePlacementOffset, placementOffset, OUT_ intoRoom, OUT_ intoRoomPlacement, &exitThroughDoor, &enterThroughDoor, &throughDoorsArrayPtr))
			{
				scoped_call_stack_info(TXT("moved through doors - change room"));
				change_room(intoRoom, intoRoomPlacement, exitThroughDoor, enterThroughDoor, &throughDoorsArrayPtr);
			}
		}
	}
	if (!attachment.isFollowing ||
		attachment.pathToAttachedTo.is_empty()) // this condition may cover case when we're in a room, we move our hand beyond door but we're standing in front of the door, this should move attached thing through the door
	{
		scoped_call_stack_info(TXT("not following"));
		// if not following, start from startTracePlacementOffset (beInRoom)
		// and check where we end - if we end up in different room than we really are, switch immediately there without changing doors
		Room* intoRoom = beInRoom;
		Transform intoRoomPlacement = Transform::identity;

		int const throughDoorsLimit = 32;
		allocate_stack_var(Framework::DoorInRoom const*, throughDoors, throughDoorsLimit);
		Framework::DoorInRoomArrayPtr throughDoorsArrayPtr(throughDoors, throughDoorsLimit); // this will store from where we are attached to, to us (for presence path towards "attached to", this means it is in reverse)

		// use offset for move through doors
		{
			scoped_call_stack_info(TXT("move through doors"));
			Transform startTracePlacementOffset = attachedTo->placement.to_world(attachedToMovementCentreOffsetOS);
			Transform newPlacementInAttachedToRoomOffset = newPlacementInAttachedToRoom.to_world(movementCentreOffsetOS);
			scoped_call_stack_info(TXT("move through doors - now"));
			if (beInRoom && beInRoom->move_through_doors(startTracePlacementOffset, newPlacementInAttachedToRoomOffset, OUT_ intoRoom, OUT_ intoRoomPlacement, nullptr, nullptr, &throughDoorsArrayPtr))
			{
				scoped_call_stack_info(TXT("move through doors - update placement"));
				beInRoom = intoRoom;
				newPlacementInAttachedToRoom = intoRoomPlacement.to_local(newPlacementInAttachedToRoom);
			}
		}

		if (inRoom != beInRoom)
		{
#ifdef INSPECT_ATTACHMENTS
			if (!fast_cast<TemporaryObject>(get_owner()))
			{
				output(TXT("[ATTACH] attachment path for o%p is not valid, we need to find a new one, we're in different rooms"), get_owner());
			}
#endif
			scoped_call_stack_info(TXT("change room"));
			if (inRoom)
			{
				scoped_call_stack_info(TXT("remove from room"));
				remove_from_room();
			}
			scoped_call_stack_info(TXT("place in new room"));
			// reset here to avoid add_to_room do stuff
			attachment.pathToAttachedTo.reset();
			placement = newPlacementInAttachedToRoom;
			an_assert_immediate(placement.is_ok());
			an_assert_immediate(placement.get_scale() != 0.0f);
			copy_placement_for_nav();
			nextPlacement = placement;
			an_assert_immediate(nextPlacement.is_ok());
			an_assert_immediate(nextPlacement.get_scale() != 0.0f);
			prevPlacementOffset = Transform::identity;
			prevPlacementOffsetDeltaTime = _deltaTime;
			vrAnchor = attachedTo->vrAnchor;
			gravity = attachedTo->gravity;
			an_assert(attachedTo->gravityDir.is_normalised() || attachedTo->gravityDir.is_zero()); // allow zero gravity - it should happen only for a short moment while other object gets updated
			gravityDir = attachedTo->gravityDir;
			velocityLinear = attachedTo->velocityLinear;
			velocityRotation = attachedTo->velocityRotation;
			internal_update_into_room(intoRoomPlacement);
			add_to_room(beInRoom);
			// setup path here, we did check that there are no doors (we were in the same room) so our through doors array is correct
			// and we do this after add_to_room as that call clears target!
			{
				scoped_call_stack_info(TXT("setup path to attached to"));
				attachment.pathToAttachedTo.set_owner_presence(this);
				attachment.pathToAttachedTo.set_target(attachment.attachedTo);
				attachment.pathToAttachedTo.push_through_doors_in_reverse(throughDoorsArrayPtr);
				an_assert(attachment.pathToAttachedTo.debug_check_if_path_valid());
			}
		}
		else if (!attachment.pathToAttachedTo.is_active())
		{
#ifdef INSPECT_ATTACHMENTS
			if (!fast_cast<TemporaryObject>(get_owner()))
			{
				output(TXT("[ATTACH] attachment path for o%p is not active, we need to activate it - we're in the same room"), get_owner());
			}
#endif
			scoped_call_stack_info(TXT("room matches but we're not active, activate"));
			// be active
			attachment.pathToAttachedTo.reset();
			attachment.pathToAttachedTo.set_owner_presence(this);
			attachment.pathToAttachedTo.set_target(attachment.attachedTo);
			an_assert(attachment.pathToAttachedTo.debug_check_if_path_valid());
		}
	}
	{
		scoped_call_stack_info(TXT("post placement change"));
		post_placement_change();
	}

	// copy vr anchor as attached objects may rely on that
	{
		scoped_call_stack_info(TXT("vr anchor"));
		if (attachment.attachedTo->get_presence()->get_in_room() == get_in_room())
		{
			vrAnchor = attachment.attachedTo->get_presence()->get_vr_anchor();
		}
		else if (!attachment.pathToAttachedTo.is_empty())
		{
			vrAnchor = attachment.pathToAttachedTo.from_target_to_owner(attachment.attachedTo->get_presence()->get_vr_anchor());
		}
	}

	if (_reason == PoseAndAttachmentsReason::VRImportant)
	{
		scoped_call_stack_info(TXT("vr important update"));
		update_placements_for_rendering_in_presence_links(placementChange); // just update them as we moved during vr important
	}

	an_assert(attachment.pathToAttachedTo.is_active(), TXT("at this point we should have path set this way or another"));

#ifdef WITH_PRESENCE_INDICATOR
	update_presence_indicator();
#endif
}

void ModulePresence::calculate_distances_to_attached_to(OUT_ float & _relativeDistance, OUT_ float & _stringPulledDistance, Transform const & _attachedToRelativeOffset, Transform const & _relativeOffset) const
{
	if (!attachment.attachedTo)
	{
		_relativeDistance = 0.0f;
		_stringPulledDistance = 0.0f;
		return;
	}

	if (! attachment.pathToAttachedTo.is_active())
	{
		// fallback if we do not have path, use just expected placement
		Transform expectedPlacementInAttachedToRoom = attachment.calculate_placement_of_attach(get_owner());
		_relativeDistance = (attachment.attachedTo->get_presence()->get_placement().to_world(_attachedToRelativeOffset).get_translation() - expectedPlacementInAttachedToRoom.to_world(_relativeOffset).get_translation()).length();
		_stringPulledDistance = _relativeDistance;
		return;
	}

	Transform attachedToPlacementInCurrentRoom = attachment.pathToAttachedTo.from_target_to_owner(attachment.attachedTo->get_presence()->get_placement().to_world(_attachedToRelativeOffset));
	Transform offsetPlacement = placement.to_world(_relativeOffset);
	_relativeDistance = (attachedToPlacementInCurrentRoom.get_translation() - offsetPlacement.get_translation()).length();
	_stringPulledDistance = attachment.pathToAttachedTo.calculate_string_pulled_distance(_relativeOffset, _attachedToRelativeOffset);
}

void ModulePresence::internal_update_into_room(Transform const & _intoRoomTransform)
{
	scoped_call_stack_info(TXT("internal update into room"));
	// place in new room
	placement = _intoRoomTransform.to_local(placement);
	an_assert_immediate(placement.is_ok());
	an_assert_immediate(placement.get_scale() != 0.0f);
	// do not update preMovePlacement and preparedMovePlacement
	preMovePlacement = _intoRoomTransform.to_local(preMovePlacement);
	preparedMovePlacement = _intoRoomTransform.to_local(preparedMovePlacement);
	nextPlacement = _intoRoomTransform.to_local(nextPlacement);
	an_assert_immediate(nextPlacement.is_ok());
	an_assert_immediate(nextPlacement.get_scale() != 0.0f);
	velocityLinear = _intoRoomTransform.vector_to_local(velocityLinear);
	nextVelocityLinear = _intoRoomTransform.vector_to_local(nextVelocityLinear);
	an_assert_immediate(nextVelocityLinear.is_ok());
	for_every(v, prevVelocities)
	{
		*v = _intoRoomTransform.vector_to_local(*v);
	}
	// TODO velocityRotation = doorTransform.to_local(velocityRotation); // is it needed? it is relative to object
	gravity = _intoRoomTransform.vector_to_local(gravity);
	gravityDir = _intoRoomTransform.vector_to_local(gravityDir);
	an_assert(gravityDir.is_zero() || gravityDir.is_normalised());
//#ifdef AN_RELEASE
//	if (is_vr_movement())
//#endif
	{
		vrAnchor = _intoRoomTransform.to_local(vrAnchor);
		vrAnchor.normalise_orientation(); // we don't want to accumulate errors
	}
}

void ModulePresence::change_room(Room * _intoRoom, Transform const & _intoRoomTransform, DoorInRoom* _exitThrough, DoorInRoom* _enterThrough, DoorInRoomArrayPtr const * _throughDoors, Name const & _reason)
{
	scoped_call_stack_info(TXT("change room"));
	ai_create_message_leaves_room(_intoRoom, _exitThrough, _reason);
	internal_change_room(_intoRoom, _intoRoomTransform, _exitThrough, _enterThrough, _throughDoors);
	ai_create_message_enters_room(_enterThrough, _reason);
}

void ModulePresence::internal_change_room(Room * _intoRoom, Transform const & _intoRoomTransform, DoorInRoom* _exitThrough, DoorInRoom* _enterThrough, DoorInRoomArrayPtr const * _throughDoors)
{
	an_assert_immediate(inRoom != nullptr);
	an_assert_immediate(_intoRoom != nullptr);

	an_assert(inRoom == _exitThrough->get_in_room());
	an_assert(_intoRoom == _enterThrough->get_in_room());

	// change room
	internal_add_to_room(_intoRoom);

	// place in new room
	internal_update_into_room(_intoRoomTransform);

	// inform other modules
	if (IModulesOwner * owner = get_owner())
	{
		if (ModuleAI* ai = owner->get_ai())
		{
			ai->internal_change_room(_enterThrough);
		}
		if (ModuleController* controller = owner->get_controller())
		{
			controller->internal_change_room(_intoRoomTransform);
		}
	}

	// inform observers (use copy as we may alter array)
	observersLock.acquire(TXT("ModulePresence::internal_change_room"));
	ARRAY_STACK(IPresenceObserver*, observersCopy, observers.get_size());
	observersCopy = observers;
	observersLock.release();
	for_every_ptr(observer, observersCopy)
	{
		Concurrency::ScopedSpinLock lock(observer->access_presence_observer_lock());
		observer->on_presence_changed_room(this, _intoRoom, _intoRoomTransform, _exitThrough, _enterThrough, _throughDoors);
	}

	post_placement_change();
}

void ModulePresence::post_placement_change()
{
	copy_placement_for_nav();
}

void ModulePresence::post_placed()
{
	// we may need to spawn/play as we were not placed in the world before
	if (auto* t = get_owner()->get_temporary_objects())
	{
		t->spawn_auto_spawn();
	}
	if (auto* s = get_owner()->get_sound())
	{
		s->play_auto_play();
	}
}

bool ModulePresence::is_attached_to_teleported() const
{
	if (is_attached())
	{
		auto* goUp = get_owner();
		while (goUp && goUp->get_presence()->is_attached())
		{
			if (goUp->get_presence()->has_teleported())
			{
				return true;
			}
			auto* prevGoUp = goUp;
			goUp = goUp->get_presence()->get_attached_to();
			if (goUp == prevGoUp) // we got stuck
			{
				break;
			}
		}
		return false;
	}
	else
	{
		return false;
	}
}

bool ModulePresence::does_require_lazy_update_presence_links() const
{
	return (lazyUpdatePresenceLinksTS.is_set() && lazyUpdatePresenceLinksTS.get().get_time_since() > 0.1f);
}

bool ModulePresence::does_require_building_presence_links(PresenceLinkOperation::Type _operation) const
{
	return doesRequireBuildingPresenceLinks[_operation];
}

void ModulePresence::update_does_require_building_presence_links(Concurrency::Counter* _excessPresenceLinksLeft)
{
	if (!get_owner()->is_advanceable())
	{
		// no need, should be removed
		doesRequireBuildingPresenceLinks[PresenceLinkOperation::Clearing] = false;
		doesRequireBuildingPresenceLinks[PresenceLinkOperation::Building] = false;

		return;
	}

	bool buildPresenceLinks = presenceLinks == nullptr && inRoom && inRoom->is_world_active(); // otherwise we may start to accumulate presence links before we activate and end up with multiple for the same object
	bool excessPresenceLink = true;
#ifdef VERY_DETAILED_PRESENCE_LINKS
	output(TXT("  update_does_require_building_presence_links [%S] for %p %S [r%p:%S:%S:%S]"), _operation == PresenceLinkOperation::Clearing? TXT("clearing") : TXT("building"), get_owner(), buildPresenceLinks? TXT("yes-buildPresenceLinks") : TXT("no-buildPresenceLinks"),
		inRoom,
		inRoom? inRoom->get_name().to_char() : TXT("--"),
		inRoom && inRoom ->is_world_active()? TXT("world active") : TXT("nwa"),
		inRoom && inRoom->get_presence_links()? TXT("has presence links") : TXT("no presence links"));
#endif
	if ((canChangeRoom || !presenceInSingleRoom || presenceInSingleRoomTO || get_owner()->get_movement() || is_attached() || has_teleported() || forceUpdatePresenceLinks || does_require_lazy_update_presence_links()) &&
		inRoom && inRoom->is_world_active())	// allow updating only if is in a room that has been activated
	{
		buildPresenceLinks = false;
		excessPresenceLink = false;
		if ((!get_owner()->is_advancement_suspended() || has_teleported() || is_attached_to_teleported()) && // teleportation overrides advancement suspension
			inRoom)
		{
			float collisionRadius = 0.0f;
			if (auto* c = get_owner()->get_collision())
			{
				collisionRadius = c->get_max_radius();
			}
			if (alwaysRebuildPresenceLinks ||
				presenceLinksBuiltFor.room != inRoom ||
				!Transform::same(presenceLinksBuiltFor.placement, placement, 0.00001f, 0.00001f, 0.00001f) ||
				!Transform::same(presenceLinksBuiltFor.presenceLinkCentreOS, presenceLinkCentreOS, 0.00001f, 0.00001f, 0.00001f) ||
				presenceLinksBuiltFor.presenceLinkRadius != presenceLinkRadius ||
				presenceLinksBuiltFor.presenceLinkRadiusClipLessPt != presenceLinkRadiusClipLessPt ||
				presenceLinksBuiltFor.presenceLinkRadiusClipLessRadius != presenceLinkRadiusClipLessRadius ||
				presenceLinksBuiltFor.presenceLinkCentreDistance != presenceLinkCentreDistance ||
				presenceLinksBuiltFor.collisionRadius != collisionRadius ||
				presenceLinksBuiltFor.lightSourceBasedPresenceRadius != lightSourceBasedPresenceRadius ||
				forceUpdatePresenceLinks ||
				does_require_lazy_update_presence_links()
#ifdef FORCE_ALWAYS_REBUILD_PRESENCE_LINKS
				|| true
#endif
				)
			{
#ifdef AN_DEVELOPMENT
#ifndef AN_CLANG
				bool roomDiffers = presenceLinksBuiltFor.room != inRoom;
				bool placDiffers = !Transform::same(presenceLinksBuiltFor.placement, placement, 0.00001f, 0.00001f, 0.00001f);
				bool centDiffers = !Transform::same(presenceLinksBuiltFor.presenceLinkCentreOS, presenceLinkCentreOS, 0.00001f, 0.00001f, 0.00001f);
				bool radiDiffers = presenceLinksBuiltFor.presenceLinkRadius != presenceLinkRadius;
				bool raclDiffers = presenceLinksBuiltFor.presenceLinkRadiusClipLessPt != presenceLinkRadiusClipLessPt;
				bool racrDiffers = presenceLinksBuiltFor.presenceLinkRadiusClipLessRadius != presenceLinkRadiusClipLessRadius;
				bool distDiffers = presenceLinksBuiltFor.presenceLinkCentreDistance != presenceLinkCentreDistance;
				bool mcolDiffers = presenceLinksBuiltFor.collisionRadius != collisionRadius;
				bool lsbrDiffers = presenceLinksBuiltFor.lightSourceBasedPresenceRadius != lightSourceBasedPresenceRadius;
#endif
#endif
				buildPresenceLinks = true;
				excessPresenceLink = false;
#ifdef VERY_DETAILED_PRESENCE_LINKS
				output(TXT("  update_does_require_building_presence_links moved or always rebuild for %p %S %c%c%c%c%c%c%c%c%c"), get_owner(), buildPresenceLinks ? TXT("yes-buildPresenceLinks") : TXT("no-buildPresenceLinks"),
					roomDiffers? 'r' : '-',
					placDiffers? 'p' : '-',
					centDiffers? 'c' : '-',
					radiDiffers? 'D' : '-',
					raclDiffers? 'l' : '-',
					racrDiffers? 'L' : '-',
					distDiffers? 'd' : '-',
					mcolDiffers? 'c' : '-',
					lsbrDiffers? '*' : '-'
				);
#endif
			}
		}
	}

	if (buildPresenceLinks)
	{
		if (excessPresenceLink)
		{
			doesRequireBuildingPresenceLinks[PresenceLinkOperation::Clearing] = false; // never clear excess presence links
			{
				int dist = inRoom ? AVOID_CALLING_ACK_ inRoom->get_distance_to_recently_seen_by_player() : NONE;
				if (dist >= 0 && dist <= 1)
				{
#ifdef VERY_DETAILED_PRESENCE_LINKS
					output(TXT("  update_does_require_building_presence_links for %p %S _excessPresenceLinksLeft dist = %i"), get_owner(), buildPresenceLinks ? TXT("yes-buildPresenceLinks") : TXT("no-buildPresenceLinks"), dist);
#endif
					doesRequireBuildingPresenceLinks[PresenceLinkOperation::Building] = true; // too close, force to build
				}
				else
				{
					if (_excessPresenceLinksLeft)
					{
						bool result = _excessPresenceLinksLeft->decrease() > 0;
#ifdef VERY_DETAILED_PRESENCE_LINKS
						output(TXT("  update_does_require_building_presence_links for %p %S _excessPresenceLinksLeft left? %S"), get_owner(), buildPresenceLinks ? TXT("yes-buildPresenceLinks") : TXT("no-buildPresenceLinks"), result? TXT("yes") : TXT("no"));
#endif
						doesRequireBuildingPresenceLinks[PresenceLinkOperation::Building] = result; // use excess
					}
					else
					{
						doesRequireBuildingPresenceLinks[PresenceLinkOperation::Building] = false; // not sure, don't do anything
					}
				}
			}
		}
		else
		{
#ifdef VERY_DETAILED_PRESENCE_LINKS
			output(TXT("  update_does_require_building_presence_links for %p %S not excess presence link"), get_owner(), buildPresenceLinks ? TXT("yes-buildPresenceLinks") : TXT("no-buildPresenceLinks"));
#endif
			// always clear and build if we should build presence links
			doesRequireBuildingPresenceLinks[PresenceLinkOperation::Clearing] = true;
			doesRequireBuildingPresenceLinks[PresenceLinkOperation::Building] = true;
		}
	}
	else
	{
		// no need
		doesRequireBuildingPresenceLinks[PresenceLinkOperation::Clearing] = false;
		doesRequireBuildingPresenceLinks[PresenceLinkOperation::Building] = false;
	}
}

void ModulePresence::advance__build_presence_links(IMMEDIATE_JOB_PARAMS)
{
	REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(TXT("ModulePresence::advance__build_presence_links"));
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(TXT("IMO"));
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		MEASURE_PERFORMANCE(buildPresenceLinks);
		if (ModulePresence * self = modulesOwner->get_presence())
		{
			// if is not advanceable, it should be cleared somewhere else - to remove both from room and object
			if (self->get_owner()->is_advanceable())
			{
				REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(TXT("advancable"));
				// if we're here, do not check if we're required to build, assume we should build
				// we assume that presence links were dropped during purge in rooms
				if (self->presenceLinks)
				{
					an_assert_log_always(self->presenceLinks->prevInObject == nullptr);
					self->presenceLinks->release_for_building_in_object_ignore_for_room();
					self->presenceLinks = nullptr;
				}
				self->forceUpdatePresenceLinks = false;
				self->lazyUpdatePresenceLinksTS.clear();
				PresenceLinkBuildContext buildContext;
				if (self->inRoom && ! self->inRoom->is_deletion_pending())
				{
					InternalPresenceLinkBuildContext context(self->inRoom);
					REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(TXT("BUILD!"));
					// use moveCentreOffset instead of presenceLinkOffset as it is more important, in which space did we land
					// due to that, grow presence radius. and if we use segment, move it to current centre offset (keeping axis orientation) and grow radius to accomodate for the movement
					// also, start with the room the presence centre is, if moving using eyes is set
					Transform startWithPlacement = self->placement;
					Room* startInRoom = self->inRoom;
					//
					if (!self->presenceInSingleRoom &&
						(self->moveUsingEyes || self->moveUsingEyesZOnly))
					{
						// in such a case we should start in the room the centre is, we just cast ray through doors
						if (Room* inRoom = startInRoom)
						{
							Transform moveCentreOffsetOS = self->calculate_move_centre_offset_os();
							Transform placement = startWithPlacement;
							Transform intoRoomTransform;
							if (inRoom->move_through_doors(placement.to_world(moveCentreOffsetOS), placement.to_world(self->presenceLinkCentreOS), REF_ inRoom, REF_ intoRoomTransform))
							{
								placement = intoRoomTransform.to_local(placement);
								// make it start in another room
								startInRoom = inRoom;
								startWithPlacement = placement;
								// alter context as well
								context.inRoom = inRoom;
								context.currentTransformFromOrigin = intoRoomTransform.to_world(context.currentTransformFromOrigin);
							}
						}
					}
					context.currentCentre = startWithPlacement.location_to_world(self->moveCentreOffsetOS.get(self->presenceLinkCentreOS).get_translation());
					auto* movement = modulesOwner->get_movement();
					float addDistance = 0.0f;
					if (movement && movement->does_require_continuous_presence())
					{
						float deltaTime = Game::get()->get_delta_time();
						an_assert(self->presenceLinkCentreDistance.is_zero());
						context.useSegment = true;
						Vector3 offset = self->get_velocity_linear() * deltaTime;
						context.currentSegment = SegmentSimple(context.currentCentre,
															   context.currentCentre + offset);
						context.currentCentre = context.currentSegment.get_half();
						addDistance = offset.length() * 0.5f;
					}
					else if (! self->presenceLinkCentreDistance.is_zero())
					{
						context.useSegment = true;
						Vector3 halfCentreDistance = startWithPlacement.to_world(self->presenceLinkCentreOS).vector_to_world(0.5f * self->presenceLinkCentreDistance);
						Vector3 properCentre = startWithPlacement.location_to_world(self->presenceLinkCentreOS.get_translation());
						Vector3 a = properCentre - halfCentreDistance;
						Vector3 b = properCentre + halfCentreDistance;
						context.currentSegment = SegmentSimple(context.currentCentre + (a - context.currentCentre).along(halfCentreDistance),
															   context.currentCentre + (b - context.currentCentre).along(halfCentreDistance));
						// use radius just as far as it is from presence link capsule
						addDistance = (context.currentCentre - b).drop_using(a - b).length();
					}
					else
					{
						addDistance = (self->moveCentreOffsetOS.get(self->presenceLinkCentreOS).get_translation() - self->presenceLinkCentreOS.get_translation()).length();
					}

					float collisionRadius = 0.0f;
					if (auto* c = modulesOwner->get_collision())
					{
						collisionRadius = c->get_max_radius(); // collision, gradient, detection, name it
					}
					float scale = startWithPlacement.get_scale();
					context.actualPresenceRadiusLeft = self->presenceLinkRadius * scale + addDistance;
					context.collisionRadiusLeft = collisionRadius * scale + addDistance;
					if (modulesOwner->get_custom<CustomModules::LightSources>())
					{
						context.lightSourceRadiusLeft = self->lightSourceBasedPresenceRadius.get(0.0f) * scale + addDistance;
					}
					else
					{
						context.lightSourceRadiusLeft = 0.0f;
					}
					context.radiusLeft = max(context.lightSourceRadiusLeft, max(context.collisionRadiusLeft, context.actualPresenceRadiusLeft));

#ifdef DEBUG_BUILD_PRESENCE_LINKS
#ifdef AN_DEBUG_RENDERER
					context.prevCentre = context.currentCentre;
					context.depth = 0;
#endif
#endif
					/*
					// ok, I commented it out instead of deleting as this might be something useless now as it competes with proper movement (movement using movement offset)
					if (!self->presenceInSingleRoom)
					{
						if (!self->presenceLinkCentreOS.get_translation().is_zero())
						{
							Transform nextPlacement(context.currentCentre, self->placement.get_orientation());
							Room* intoRoom;
							Transform intoRoomPlacement(context.currentCentre, self->placement.get_orientation());
							if (self->inRoom->move_through_doors(self->placement, nextPlacement, OUT_ intoRoom, OUT_ intoRoomPlacement))
							{
								context.inRoom = intoRoom;
								context.currentCentre = intoRoomPlacement.location_to_local(context.currentCentre);
								if (context.useSegment)
								{
									context.currentSegment = SegmentSimple(intoRoomPlacement.location_to_local(context.currentSegment.get_start()),
										intoRoomPlacement.location_to_local(context.currentSegment.get_end()));
								}
								context.currentTransformFromOrigin = intoRoomPlacement.to_world(context.currentTransformFromOrigin);
							}
						}
					}
					*/
					REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(TXT("build presence link"));
					self->build_presence_link(buildContext, context, nullptr, 6); // this will be our first link
					// store information about for what conditions did we built to avoid rebuilding
					self->presenceLinksBuiltFor.room = startInRoom;
					self->presenceLinksBuiltFor.placement = startWithPlacement;
					self->presenceLinksBuiltFor.presenceLinkCentreOS = self->presenceLinkCentreOS;
					self->presenceLinksBuiltFor.presenceLinkRadius = self->presenceLinkRadius;
					self->presenceLinksBuiltFor.presenceLinkRadiusClipLessPt = self->presenceLinkRadiusClipLessPt;
					self->presenceLinksBuiltFor.presenceLinkRadiusClipLessRadius = self->presenceLinkRadiusClipLessRadius;
					self->presenceLinksBuiltFor.presenceLinkCentreDistance = self->presenceLinkCentreDistance;
					self->presenceLinksBuiltFor.collisionRadius = collisionRadius;
					self->presenceLinksBuiltFor.lightSourceBasedPresenceRadius = self->lightSourceBasedPresenceRadius;

					// no longer required to clear nor build (this is really only for advanced once who without updating these will be cleared)
					self->doesRequireBuildingPresenceLinks[PresenceLinkOperation::Clearing] = false;
					self->doesRequireBuildingPresenceLinks[PresenceLinkOperation::Building] = false;
				}
			}
		}
		else
		{
			an_assert(false, TXT("shouldn't be called"));
		}
	}
}

bool ModulePresence::should_clip_less(float _radiusLeft) const
{
	if (presenceLinkClipLess)
	{
		return true;
	}
	if ((presenceLinkRadiusClipLessPt >= 0.0f && (_radiusLeft >= presenceLinkRadius * presenceLinkRadiusClipLessPt))
		|| (presenceLinkRadiusClipLessRadius >= 0.0f && (_radiusLeft >= presenceLinkRadiusClipLessRadius)))
	{
		return true;
	}
	return false;
}

void ModulePresence::build_presence_link(PresenceLinkBuildContext const & _buildContext, InternalPresenceLinkBuildContext const & _context, PresenceLink* _linkTowardsOwner, int _depthLeft)
{
	REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(TXT("ModulePresence::build_presence_link"));
	if (!_depthLeft ||
		_context.inRoom->is_deletion_pending())
	{
		return;
	}

	bool useCollision = _context.collisionRadiusLeft > 0.0f || _context.actualPresenceRadiusLeft > 0.0f; // using presence as we may have some collisions without radius

	float lightSourceRadiusLeft = _context.lightSourceRadiusLeft;
	if (lightSourceRadiusLeft > 0.0f && _linkTowardsOwner && _context.throughDoor)
	{
		// check if any door on the way is backwards, if so, reduce light source greatly
		Vector3 doorDir = _context.throughDoor->get_placement().get_axis(Axis::Forward);
		auto const * l = _linkTowardsOwner;
		while (l)
		{
			if (l->throughDoor)
			{
				Vector3 ld = l->throughDoor->get_placement().get_axis(Axis::Forward);
				float dot = Vector3::dot(doorDir, ld);
				lightSourceRadiusLeft *= clamp((dot + 0.1f) * 1.1f, 0.0f, 1.0f);
			}
			l = l->linkTowardsOwner;
		}
	}

	PresenceLink* link = PresenceLink::create(_buildContext, this, _linkTowardsOwner, _context.inRoom, _context.throughDoor, _context.currentTransformFromOrigin, _context.actualPresenceRadiusLeft > 0.0f, useCollision && _context.validForCollision, useCollision && _context.validForCollisionGradient, useCollision && _context.validForCollisionDetection, lightSourceRadiusLeft > 0.0f);

#ifdef DEBUG_BUILD_PRESENCE_LINKS
	debug_context(_context.inRoom);
	debug_filter(buildPresenceLinks);
	debug_draw_line(true, Colour::grey, _context.currentCentre, _context.prevCentre);
	if (_context.useSegment)
	{
		debug_draw_capsule(true, false, Colour::red.with_alpha(0.5f), 0.2f, Capsule(_context.currentSegment.get_start(), _context.currentSegment.get_end(), _context.radiusLeft));
	}
	else
	{
		debug_draw_sphere(true, false, Colour::red.with_alpha(0.5f), 0.2f, Sphere(_context.currentCentre, _context.radiusLeft));
	}
	debug_draw_line(true, Colour::purple, _context.currentCentre, link->get_placement_in_room().location_to_world(presenceLinkCentreOS.get_translation()));
	debug_draw_line(true, Colour::zxYellow, Vector3::zero, _context.currentTransformFromOrigin.location_to_local(get_placement().get_translation()));
#ifdef AN_DEBUG_RENDERER
	for (int i = 1; i <= _context.depth; ++i)
	{
		// show depth
		debug_draw_sphere(true, false, Colour::blue.with_alpha(0.5f), 0.5f, Sphere(_context.currentCentre + Vector3(0.0f, 0.0f, 0.025f) * (float)i, 0.005f));
	}
	if (_context.throughDoor)
	{
		// to centre of door, so we know through which one we came
		debug_draw_line(true, Colour::blue, _context.currentCentre, _context.throughDoor->get_placement().get_translation());
	}
#endif
	debug_draw_sphere(true, false, Colour::red.with_alpha(0.5f), 0.5f, Sphere(_context.currentCentre, 0.005f));
	debug_draw_sphere(true, false, Colour::purple.with_alpha(0.5f), 0.5f, Sphere(link->get_placement_in_room().location_to_world(presenceLinkCentreOS.get_translation()), 0.01f));
	debug_no_filter();
	debug_no_context();
#endif

	if (! link)
	{
		return;
	}

	if (presenceInSingleRoom)
	{
		return;
	}

	if (_context.throughDoor)
	{
		an_assert(!_context.throughDoor->get_plane().get_normal().is_zero());
		if (presenceLinkAccumulateDoorClipPlanes)
		{
			REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(TXT("presenceLinkAccumulateDoorClipPlanes"));
			link->clipPlanesForCollision.add(_context.throughDoorPlanes);
			link->clipPlanes.add(_context.throughDoorPlanes);
			//
			link->clipPlanesForCollision.add_smartly_more_relevant(_context.throughDoor->get_plane());
			if (!presenceLinkDoNotClip)
			{
				link->clipPlanes.add_smartly_more_relevant(_context.throughDoor->get_plane());
			}
		}
		else if (presenceLinkAlwaysClipAgainstThroughDoor)
		{
			REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(TXT("presenceLinkAlwaysClipAgainstThroughDoor"));
			link->clipPlanesForCollision.add_smartly_more_relevant(_context.throughDoor->get_plane());
			if (!presenceLinkDoNotClip)
			{
				link->clipPlanes.add_smartly_more_relevant(_context.throughDoor->get_plane());
			}
		}
		else
		{
			REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(TXT("clipPlanes usual"));
			bool clipLessNow = should_clip_less(_context.radiusLeft);
			if (!presenceLinkDoNotClip && !clipLessNow )
			{
				link->clipPlanes.add_smartly_more_relevant(_context.throughDoor->get_plane());
			}
			REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info_str_printf(TXT("through door dr'%p in room r%p"), _context.throughDoor, _context.inRoom);
			if ((_context.useSegment && _context.throughDoor->is_inside_hole(_context.currentSegment)) ||
				(! _context.useSegment && _context.throughDoor->is_inside_hole(_context.currentCentre)))
			{
				link->clipPlanesForCollision.add_smartly_more_relevant(_context.throughDoor->get_plane());
				if (!presenceLinkDoNotClip && clipLessNow)
				{
					link->clipPlanes.add_smartly_more_relevant(_context.throughDoor->get_plane());
				}
			}
		}
	}

	// go through all doors
	for_every_ptr(door, _context.inRoom->get_doors())
	{
		REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(TXT("door"));
		if (door->is_visible() && door != _context.throughDoor && door->has_world_active_room_on_other_side() &&
			(door->get_door()->can_see_through_when_closed() || ! door->get_door()->is_closed()))
		{
			// check if it is in front
			Vector3 onDoorHole;
			SegmentSimple onDoorHoleSegment;
			float radiusLeft;
			float const offHoleDistMultiplier = 3.0f;	// to prefer distances within hole - if object is outside hole, it's distance on door plane will be extended
														// this helps to disallow objects going to another room when they should stay within room
			if ((_context.useSegment && door->is_in_radius(true, _context.currentSegment, _context.radiusLeft, OUT_ onDoorHoleSegment, OUT_ radiusLeft, offHoleDistMultiplier)) ||
				(!_context.useSegment && door->is_in_radius(true, _context.currentCentre, _context.radiusLeft, OUT_ onDoorHole, OUT_ radiusLeft, offHoleDistMultiplier)))
			{
#ifdef DEBUG_BUILD_PRESENCE_LINKS
				if (_context.useSegment)
				{
					onDoorHole = onDoorHoleSegment.get_half();
				}
				debug_context(_context.inRoom);
				debug_filter(buildPresenceLinks);
				debug_draw_line(true, Colour::green, _context.currentCentre, onDoorHole);
				door->debug_draw_hole(true, Colour::green.with_alpha(0.5f));
				debug_draw_line(true, Colour::cyan, onDoorHole, door->get_placement().get_translation());
				debug_draw_sphere(true, false, Colour::green, 0.2f, Sphere(onDoorHole, 0.05f));
				debug_no_filter();
				debug_no_context();
#endif
				InternalPresenceLinkBuildContext nextContext(_context);
				++nextContext.currentDepth;
				if (presenceLinkAccumulateDoorClipPlanes)
				{
					REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(TXT("door - presenceLinkAccumulateDoorClipPlanes"));
					nextContext.throughDoorPlanes = _context.throughDoorPlanes;
					if (_context.throughDoor)
					{
						nextContext.throughDoorPlanes.add_smartly_more_relevant(_context.throughDoor->get_plane());
					}
					// transform planes to the new room
					for_every(p, nextContext.throughDoorPlanes.planes)
					{
						*p = door->get_other_room_transform().to_local(*p);
					}
				}
				nextContext.inRoom = door->get_world_active_room_on_other_side();
				nextContext.throughDoor = door->get_door_on_other_side();
				nextContext.currentTransformFromOrigin = _context.currentTransformFromOrigin.to_world(door->get_other_room_transform()); // accumulate transform from original room to current one (add door's transform to transform so-far)
				if (_context.useSegment)
				{
					nextContext.currentSegment = SegmentSimple(door->get_other_room_transform().location_to_local(onDoorHoleSegment.get_start()),
															   door->get_other_room_transform().location_to_local(onDoorHoleSegment.get_end()));
					nextContext.currentCentre = nextContext.currentSegment.get_half();
					if (nextContext.currentSegment.get_start() == nextContext.currentSegment.get_end())
					{
						nextContext.useSegment = false;
					}
				}
				else
				{
					nextContext.currentCentre = door->get_other_room_transform().location_to_local(onDoorHole);
				}
				if (nextContext.validForCollision || nextContext.validForCollisionGradient || nextContext.validForCollisionDetection)
				{
					if (IModulesOwner * owner = get_owner())
					{
						if (ModuleCollision* moduleCollision = owner->get_collision())
						{
							Transform usePlacement = link->get_placement_in_room();
							float distanceSoFar = 0.0f;
							// this doesn't work well as centre is not the same as placement and we require placement to offset collision primitives
							//placement.set_translation(_context.currentCentre);
							//distanceSoFar = presenceLinkRadius - _context.radiusLeft;
							moduleCollision->should_consider_for_collision(door, usePlacement, distanceSoFar, REF_ nextContext.validForCollision, REF_ nextContext.validForCollisionGradient, REF_ nextContext.validForCollisionDetection);
						}
					}
				}
#ifdef DEBUG_BUILD_PRESENCE_LINKS
#ifdef AN_DEBUG_RENDERER
				nextContext.prevCentre = door->get_other_room_transform().location_to_local(_context.currentCentre);
				nextContext.depth = _context.depth + 1;
#endif
#endif
#ifdef DEBUG_BUILD_PRESENCE_LINKS
				debug_context(inRoom);
				debug_filter(buildPresenceLinks);
				debug_draw_line(true, Colour::orange, _context.currentTransformFromOrigin.get_translation(), nextContext.currentTransformFromOrigin.get_translation());
#ifdef AN_DEBUG_RENDERER
				for (int i = 1; i <= nextContext.depth; ++i)
				{
					// show depth
					debug_draw_sphere(true, false, Colour::orange.with_alpha(0.5f), 0.5f, Sphere(nextContext.currentTransformFromOrigin.get_translation() + Vector3(0.0f, 0.0f, 0.025f) * (float)i, 0.005f));
				}
#endif
				debug_no_filter();
				debug_no_context();
#endif
				nextContext.radiusLeft = radiusLeft;
				nextContext.actualPresenceRadiusLeft = _context.actualPresenceRadiusLeft + (radiusLeft - _context.radiusLeft);
				nextContext.collisionRadiusLeft = _context.collisionRadiusLeft + (radiusLeft - _context.radiusLeft);
				nextContext.lightSourceRadiusLeft = lightSourceRadiusLeft + (radiusLeft - _context.radiusLeft);
				build_presence_link(_buildContext, nextContext, link, _depthLeft - 1);
				// add clip plane as we went pass that door
				bool clipLessNow = should_clip_less(nextContext.radiusLeft);
				if (!presenceLinkDoNotClip && !clipLessNow)
				{
					link->clipPlanes.add_smartly_more_relevant(door->get_plane());
				}
				if ((_context.useSegment && door->is_inside_hole(_context.currentSegment)) ||
					(!_context.useSegment && door->is_inside_hole(_context.currentCentre)))
				{
					an_assert(!door->get_plane().get_normal().is_zero());
					link->clipPlanesForCollision.add_smartly_more_relevant(door->get_plane());
					if (!presenceLinkDoNotClip && clipLessNow)
					{
						link->clipPlanes.add_smartly_more_relevant(door->get_plane());
					}
				}
			}
		}
	}
}

ModulePresence::InternalPresenceLinkBuildContext::InternalPresenceLinkBuildContext(Room* _inRoom)
: inRoom(_inRoom)
, throughDoor(nullptr)
, currentTransformFromOrigin(Transform::identity)
, currentDepth(0)
, validForCollision(true)
, validForCollisionGradient(true)
, validForCollisionDetection(true)
{}

ModulePresence::InternalPresenceLinkBuildContext::InternalPresenceLinkBuildContext(InternalPresenceLinkBuildContext const & _other)
: inRoom(_other.inRoom)
, throughDoor(_other.throughDoor)
, currentTransformFromOrigin(_other.currentTransformFromOrigin)
, currentDepth(_other.currentDepth)
, currentCentre(_other.currentCentre)
, useSegment(_other.useSegment)
, currentSegment(_other.currentSegment)
, radiusLeft(_other.radiusLeft)
, validForCollision(_other.validForCollision)
, validForCollisionGradient(_other.validForCollisionGradient)
, validForCollisionDetection(_other.validForCollisionDetection)
{}

void ModulePresence::add_presence_link(PresenceLinkBuildContext const & _context, PresenceLink* _link)
{
	// we build for module presence in a single thread, spanning across multiple rooms
	an_assert_log_always(_link->owner == get_owner());
	an_assert_log_always(_link->ownerPresence == this);
	an_assert_log_always(presenceLinks == nullptr || presenceLinks->prevInObject == nullptr);
	an_assert_log_always(_link->prevInObject == nullptr);
	an_assert_log_always(_link->nextInObject == nullptr);
	_link->nextInObject = presenceLinks;
	if (presenceLinks)
	{
		an_assert_log_always(presenceLinks->prevInObject == nullptr);
		presenceLinks->prevInObject = _link;
	}
	presenceLinks = _link;
	an_assert_log_always(presenceLinks->prevInObject == nullptr);
	an_assert_log_always(presenceLinks->ownerPresence == this);
}

void ModulePresence::sync_remove_presence_link(PresenceLink* _link)
{
	ASSERT_SYNC; // should be called only during destruction of an object! and that should happen only in sync

	auto* pl = presenceLinks;
	while (pl)
	{
		auto* next = pl->nextInObject;
		if (pl->linkTowardsOwner == _link)
		{
			pl->linkTowardsOwner = nullptr; // we don't want any other presence link to point at us
		}
		if (pl == _link)
		{
			// relink
			if (pl->prevInObject)
			{
				pl->prevInObject->nextInObject = pl->nextInObject;
			}
			if (pl->nextInObject)
			{
				pl->nextInObject->prevInObject = pl->prevInObject;
			}
			// replace first
			if (presenceLinks == pl)
			{
				an_assert_log_always(pl->prevInObject == nullptr, TXT("sync_remove_presence_link, first prev pre"));
				presenceLinks = pl->nextInObject;
				an_assert_log_always(pl->prevInObject == nullptr, TXT("sync_remove_presence_link, first prev post"));
			}
			// clear
			pl->prevInObject = nullptr;
			pl->nextInObject = nullptr;
			an_assert_log_always(presenceLinks == nullptr || presenceLinks->prevInObject == nullptr);
			an_assert_log_always(presenceLinks == nullptr || presenceLinks->ownerPresence == this || presenceLinks->ownerPresence == nullptr);
		}
		pl = next;
	}

	an_assert(!_link->owner || !_link->owner->is_advanceable(), TXT("not found?")); // not advanceable objects may clear presenceLinks when building presence links is issued for them
}

void ModulePresence::ai_create_message_leaves_room(Room* _toRoom, DoorInRoom* _throughDoor, Name const & _reason)
{
	if (createAIMessages && inRoom && get_owner()->is_available_as_safe_object())
	{
		if (World* world = inRoom->get_in_world())
		{
			if (AI::Message* message = world->create_ai_message(NAME(leavesRoom)))
			{
				message->to_room(inRoom);
				message->access_param(NAME(who)).access_as<SafePtr<IModulesOwner>>() = get_owner();
				message->access_param(NAME(reason)).access_as<Name>() = _reason;
				message->access_param(NAME(toRoom)).access_as<SafePtr<Room>>() = _toRoom;
				message->access_param(NAME(throughDoor)).access_as<SafePtr<DoorInRoom>>() = _throughDoor;
			}
		}
	}
}

void ModulePresence::ai_create_message_enters_room(DoorInRoom * _throughDoor, Name const & _reason)
{
	if (createAIMessages && inRoom && get_owner()->is_available_as_safe_object())
	{
		if (World* world = inRoom->get_in_world())
		{
			if (AI::Message* message = world->create_ai_message(NAME(entersRoom)))
			{
				message->to_room(inRoom);
				message->access_param(NAME(who)).access_as<SafePtr<IModulesOwner>>() = get_owner();
				message->access_param(NAME(throughDoor)).access_as<SafePtr<DoorInRoom>>() = _throughDoor;
			}
		}
	}
}

bool ModulePresence::does_require_update_eye_look_relative() const
{
	todo_note(TXT("we rely on fact that eyesLookRelativeAppearance is set"));
	return eyesLookRelativeSocket.is_name_valid() || eyesLookRelativeBone.is_name_valid() || eyesLookRelativeAppearance.is_valid();
}

void ModulePresence::update_eye_look_relative()
{
	bool eyeProvided = false;
	ModuleAppearance const* appearance = nullptr;
	if (eyesLookRelativeAppearance.is_valid())
	{
		appearance = get_owner()->get_appearance_named(eyesLookRelativeAppearance);
	}
	else
	{
		appearance = get_owner()->get_appearance();
	}

	if (appearance)
	{
		if (Skeleton const * skeleton = appearance->get_skeleton())
		{
			if (! eyeProvided &&
				eyesLookRelativeSocket.is_name_valid())
			{
				if (eyesLookRelativeSocket.get_mesh() != appearance->get_mesh())
				{
					eyesLookRelativeSocket.look_up(appearance->get_mesh());
				}
				if (eyesLookRelativeSocket.is_valid())
				{
					if (Meshes::Pose const* finalPoseLS = appearance->get_final_pose_LS())
					{
						Transform eyesLookRelativeTransformOS = appearance->calculate_socket_os(eyesLookRelativeSocket.get_index(), finalPoseLS);
						eyesRelativeLook = eyesLookRelativeTransformOS;
						eyeProvided = true;
					}
				}
			}
			if (!eyeProvided &&
				eyesLookRelativeBone.is_name_valid())
			{
				if (eyesLookRelativeBone.get_skeleton() != skeleton->get_skeleton())
				{
					eyesLookRelativeBone.look_up(skeleton->get_skeleton());
				}
				if (eyesLookRelativeBone.is_valid())
				{
					if (Meshes::Pose const * finalPoseLS = appearance->get_final_pose_LS())
					{
						Transform eyesLookRelativeTransformMS = finalPoseLS->get_bone_ms_from_ls(eyesLookRelativeBone.get_index());
						Transform eyesLookRelativeTransformOS = appearance->get_ms_to_os_transform().to_world(eyesLookRelativeTransformMS);
						eyesRelativeLook = eyesLookRelativeTransformOS;
						eyeProvided = true;
					}
				}
			}
		}
	}

	if (!eyeProvided)
	{
		// fallback, just use straight requested look relative values
		eyesRelativeLook = requestedRelativeLook;
		eyesRelativeLook.set_orientation(eyesRelativeLook.get_orientation().to_world(lookOrientationAdditionalOffset));
		eyesRelativeLook.set_translation(eyesRelativeLook.get_translation() + Vector3(0.0f, 0.0f, verticalLookOffset));
	}
}

bool ModulePresence::get_request_teleport_details(OPTIONAL_ OUT_ Room** _intoRoom, OPTIONAL_ OUT_ Transform* _placement, OPTIONAL_ OUT_ int* _teleportPriority) const
{
	if (teleportRequested)
	{
		assign_optional_out_param(_intoRoom, teleportIntoRoom);
		assign_optional_out_param(_placement, teleportToPlacement);
		assign_optional_out_param(_teleportPriority, teleportRequestedPriority);
	}
	return teleportRequested;
}

void ModulePresence::request_teleport(Room * _intoRoom, Transform const & _placement, Optional<TeleportRequestParams> const& _params)
{
	int teleportPriority = 0;
	if (_params.is_set())
	{
		teleportPriority = _params.get().teleportPriority.get(teleportPriority);
	}
	if (!teleportRequested || teleportRequestedPriority < teleportPriority)
	{
		teleportRequested = true;
		teleportRequestedPriority = teleportPriority;
		teleportIntoRoom = _intoRoom;
		teleportToPlacement = _placement;
		if (_params.is_set())
		{
			teleportWithVelocity = _params.get().velocityLinearWS;
			silentTeleport = _params.get().silentTeleport.get(false);
			teleportBeVisible = _params.get().beVisible;
			teleportKeepVelocityOS = _params.get().keepVelocityOS.get(false);
			teleportFindVRAnchor = _params.get().findVRAnchor;
		}
		else
		{
			teleportWithVelocity.clear();
			silentTeleport = false;
			teleportBeVisible.clear();
			teleportKeepVelocityOS = false;
			teleportFindVRAnchor = false;
		}
	}
}

void ModulePresence::request_teleport_within_room(Transform const & _placement, Optional<TeleportRequestParams> const& _params)
{
	request_teleport(inRoom, _placement, _params);
}

void ModulePresence::activate()
{
	base::activate();

	if (Room* room = inRoom)
	{
		// remove from allObjects and add again
		inRoom->remove_object(get_owner());
		inRoom->add_object(get_owner());
	}

	memory_zero(doesRequireBuildingPresenceLinks, sizeof(bool) * PresenceLinkOperation::COUNT);
}

Transform ModulePresence::get_requested_relative_look(bool _withAdditionalOffset) const
{
	if (!_withAdditionalOffset)
	{
		return requestedRelativeLook;
	}
	else
	{
		Transform result = requestedRelativeLook;
		result.set_orientation(result.get_orientation().to_world(lookOrientationAdditionalOffset));
		return result;
	}
}

Transform ModulePresence::get_requested_relative_look_for_vr(bool _withAdditionalOffset) const
{
	Transform result = get_requested_relative_look(_withAdditionalOffset);
	Vector3 translation = result.get_translation();
	translation.z += floorLevel;
	result.set_translation(translation);
	return result;
}

Transform ModulePresence::get_requested_relative_hand(int _handIdx) const
{
	return requestedRelativeHands[_handIdx];
}

bool ModulePresence::is_requested_relative_hand_accurate(int _handIdx) const
{
	return requestedRelativeHandsAccurate[_handIdx];
}

Transform ModulePresence::get_requested_relative_hand_for_vr(int _handIdx) const
{
	Transform result = get_requested_relative_hand(_handIdx);
	Vector3 translation = result.get_translation();
	translation.z += floorLevel;
	result.set_translation(translation);
	return result;
}

Transform ModulePresence::get_placement_in_vr_space() const
{
	return vrAnchor.to_local(placement);
}

void ModulePresence::set_vr_anchor(Transform const& _vrAnchor)
{
	vrAnchor = _vrAnchor;
}

void ModulePresence::zero_vr_anchor(bool _continuous)
{
	if (auto* vr = VR::IVR::get())
	{
		if (vr->is_controls_view_valid())
		{
			if (_continuous)
			{
				Transform movementCentre = vr->get_controls_movement_centre();
				Vector3 locationInVR = movementCentre.get_translation() * Vector3::xy;
				Vector3 shouldBeAtLocationInVR = vrAnchor.location_to_local(placement.get_translation());
				Vector3 diffInVR = shouldBeAtLocationInVR - locationInVR;
				float const moveLimit = 0.01f;
				if (diffInVR.length() > moveLimit)
				{
					// pull towards where should we be
					diffInVR = diffInVR.normal() * (diffInVR.length() - moveLimit);
					Vector3 diffInWS = vrAnchor.vector_to_world(diffInVR);
					vrAnchor.set_translation(vrAnchor.get_translation() + diffInWS);
				}
			}
			else
			{
				Transform movementCentre = vr->get_controls_movement_centre();
				Transform placementInVR = look_matrix(movementCentre.get_translation() * Vector3::xy, calculate_flat_forward_from(movementCentre), Vector3::zAxis).to_transform();
				//Transform shouldBeAtPlacementInVR = vrAnchor.to_local(placement);
				//Transform diffInVR = placementInVR.to_local(shouldBeAtPlacementInVR);
				//vrAnchor = vrAnchor.to_world(diffInVR);
				// reverse placementInVR = vrAnchor.to_local(placement);
				vrAnchor = placement.to_world(placementInVR.inverted());
			}
			return;
		}
	}

	vrAnchor = placement;
}

void ModulePresence::transform_vr_anchor(Transform const& _localToVRAnchorTransform)
{
	vrAnchor = vrAnchor.to_world(_localToVRAnchorTransform);
	vrAnchor.normalise_orientation();
}

void ModulePresence::turn_vr_anchor_by_180()
{
	vrAnchor = vrAnchor.to_world(Transform(Vector3::zero, Rotator3(0.0f, 180.0f, 0.0f).to_quat()));
}

void ModulePresence::update_vr_anchor_from_room()
{
	vrAnchor = inRoom->get_vr_anchor(placement);
}

void ModulePresence::add_based(IModulesOwner* _basedOnThisOne)
{
	scoped_call_stack_info(TXT("add based"));
	scoped_call_stack_info(get_owner()->ai_get_name().to_char());
	Concurrency::ScopedSpinLock lock(basedLock, true);
	an_assert(!based.does_contain(_basedOnThisOne));
	based.push_back(_basedOnThisOne);
}

void ModulePresence::remove_based(IModulesOwner* _basedOnThisOne)
{
	scoped_call_stack_info(TXT("remove based"));
	scoped_call_stack_info(get_owner()->ai_get_name().to_char());
	Concurrency::ScopedSpinLock lock(basedLock, true);
	an_assert(based.does_contain(_basedOnThisOne));
	based.remove(_basedOnThisOne);
}

void ModulePresence::drop_based()
{
	while (true)
	{
		IModulesOwner* imo = nullptr;
		{
			Concurrency::ScopedSpinLock lock(basedLock, true);
			if (based.is_empty())
			{
				break;
			}
			imo = based.get_last();
		}
		if (imo)
		{
			if (auto* p = imo->get_presence())
			{
				an_assert_immediate(p->basedOn.get_presence() == this);
				p->basedOn.clear();
			}
		}
	}
}

void ModulePresence::add_presence_observer(IPresenceObserver* _observer)
{
	Concurrency::ScopedSpinLock lock(observersLock, TXT("ModulePresence::add_presence_observer"));
	//an_assert(!observers.does_contain(_observer));
	observers.push_back(_observer);
}

void ModulePresence::remove_presence_observer(IPresenceObserver* _observer)
{
	Concurrency::ScopedSpinLock lock(observersLock, TXT("ModulePresence::remove_presence_observer"));
	//an_assert(observers.does_contain(_observer));
	observers.remove_fast(_observer);
}

void ModulePresence::debug_draw(Colour const & _colour, float _alphaFill, bool _justPresence) const
{
#ifdef AN_DEBUG_RENDERER
	debug_context(get_owner()->get_presence()->get_in_room());
	debug_subject(get_owner());
	auto* appearance = get_owner()->get_appearance();
	bool usePresencePrimitive = true;
	if (appearance)
	{
		if (appearance->does_use_movement_collision_bounding_box_for_frustum_check())
		{
			int id = appearance->get_movement_collision_id();
			if (id != NONE)
			{
				if (auto const * collision = get_owner()->get_collision())
				{
					if (auto const * instance = collision->get_movement_collision().get_instance_by_id(id))
					{
						Box box;
						box.setup_allow_zero(instance->get_bounding_box());
						debug_push_transform(get_owner()->get_presence()->get_placement());
						box.debug_draw(true, false, _colour, _alphaFill);
						debug_pop_transform();
						usePresencePrimitive = false;
					}
				}
			}
		}
		if (appearance->does_use_precise_collision_bounding_box_for_frustum_check())
		{
			int id = appearance->get_precise_collision_id();
			if (id != NONE)
			{
				if (auto const * collision = get_owner()->get_collision())
				{
					if (auto const * instance = collision->get_precise_collision().get_instance_by_id(id))
					{
						Box box;
						box.setup_allow_zero(instance->get_bounding_box());
						debug_push_transform(get_owner()->get_presence()->get_placement());
						box.debug_draw(true, false, _colour, _alphaFill);
						debug_pop_transform();
						usePresencePrimitive = false;
					}
				}
			}
		}
		if (usePresencePrimitive && appearance->does_use_bones_bounding_box_for_frustum_check())
		{
			Box box;
			box.setup_allow_zero(appearance->get_bones_bounding_box());
			debug_push_transform(get_owner()->get_presence()->get_placement());
			box.debug_draw(true, false, _colour, _alphaFill);
			debug_pop_transform();
			usePresencePrimitive = false;
		}
	}
	if (usePresencePrimitive)
	{
		Vector3 presLoc = placement.location_to_world(presenceLinkCentreOS.get_translation());
		Vector3 presHCD = placement.to_world(presenceLinkCentreOS).vector_to_world(presenceLinkCentreDistance) * 0.5f;
		debug_draw_capsule(true, false, _colour, _alphaFill, Capsule(presLoc - presHCD, presLoc + presHCD, presenceLinkRadius));
	}
	if (!_justPresence)
	{
		debug_push_transform(get_owner()->get_presence()->get_placement());
		debug_draw_sphere(true, true, Colour::red, 0.2f, Sphere(centreOfMass, 0.01f));
		float height = 0.2f;
		float radius = 0.1f;
		debug_draw_line(true, Colour::blue.with_alpha(0.5f), Vector3::zero, Vector3(radius, 0.0f, height));
		debug_draw_line(true, Colour::blue.with_alpha(0.5f), Vector3::zero, Vector3(-radius, 0.0f, height));
		debug_draw_line(true, Colour::blue.with_alpha(0.5f), Vector3::zero, Vector3(0.0f, radius, height));
		debug_draw_line(true, Colour::blue.with_alpha(0.5f), Vector3::zero, Vector3(0.0f, -radius, height));
		debug_draw_line(true, Colour::blue.with_alpha(0.5f), Vector3(radius, 0.0f, height), Vector3(0.0f, radius, height));
		debug_draw_line(true, Colour::blue.with_alpha(0.5f), Vector3(radius, 0.0f, height), Vector3(0.0f, -radius, height));
		debug_draw_line(true, Colour::blue.with_alpha(0.5f), Vector3(-radius, 0.0f, height), Vector3(0.0f, radius, height));
		debug_draw_line(true, Colour::blue.with_alpha(0.5f), Vector3(-radius, 0.0f, height), Vector3(0.0f, -radius, height));
		debug_pop_transform();
#ifdef AN_DEVELOPMENT
		for_every(debugPresenceTraceHit, debugPresenceTraceHits)
		{
			debug_draw_line(true, Colour::red, debugPresenceTraceHit->location, debugPresenceTraceHit->location + debugPresenceTraceHit->normal);
		}
#endif
	}
	debug_no_subject();
	debug_no_context();
#endif
}

void ModulePresence::debug_draw_base_info() const
{
#ifdef AN_DEBUG_RENDERER
	debug_context(get_owner()->get_presence()->get_in_room());
	debug_subject(get_owner());
	Vector3 at = get_centre_of_presence_WS();
	float scale = 0.1f;
	Vector3 l(0.0f, 0.0f, -1.0f * scale);
	if (canBeBase)
	{
		String lockInfo(TXT("canBeBase"));
		Colour colour = Colour::green;
		if (lockAsBaseRequiresVRMovement || lockAsBaseRequiresPlayerMovement)
		{
			colour = Colour::red;
			lockInfo = String::empty();
			if (lockAsBaseRequiresVRMovement)
			{
				lockInfo += TXT("lockedBaseVR ");
			}
			if (lockAsBaseRequiresPlayerMovement)
			{
				lockInfo += TXT("lockedBasePlayer ");
			}
		}
		else if (lockAsBase)
		{
			colour = Colour::red;
			lockInfo = TXT("lockedBase");
		}
		debug_draw_text(true, colour, at, Vector2::half, true, scale, NP, lockInfo.to_char());
		at += l;
	}
	if (auto* bop = basedOn.get_presence())
	{
		Colour colour = Colour::c64Orange;
		if (bop->is_locked_as_base(this))
		{
			colour = Colour::c64Brown;
		}
		debug_draw_text(true, colour, at, Vector2::half, true, scale, NP, TXT("basedOn"));
		debug_draw_arrow(true, colour, at, bop->get_centre_of_presence_WS());
		at += l;

		if (keepWithinSafeVolumeOnLockedBase && bop->is_locked_as_base(this))
		{
			debug_push_transform(bop->get_placement());
			auto& sv = bop->get_safe_volume();
			if (sv.is_set())
			{
				Colour svc = Colour::c64Brown;
				debug_draw_line(true, svc, sv.get().get_at(Vector3(0.0f, 0.0f, 0.0f)), sv.get().get_at(Vector3(1.0f, 0.0f, 0.0f)));
				debug_draw_line(true, svc, sv.get().get_at(Vector3(0.0f, 1.0f, 0.0f)), sv.get().get_at(Vector3(1.0f, 1.0f, 0.0f)));
				debug_draw_line(true, svc, sv.get().get_at(Vector3(0.0f, 0.0f, 0.0f)), sv.get().get_at(Vector3(0.0f, 1.0f, 0.0f)));
				debug_draw_line(true, svc, sv.get().get_at(Vector3(1.0f, 0.0f, 0.0f)), sv.get().get_at(Vector3(1.0f, 1.0f, 0.0f)));
				debug_draw_line(true, svc, sv.get().get_at(Vector3(0.0f, 0.0f, 1.0f)), sv.get().get_at(Vector3(1.0f, 0.0f, 1.0f)));
				debug_draw_line(true, svc, sv.get().get_at(Vector3(0.0f, 1.0f, 1.0f)), sv.get().get_at(Vector3(1.0f, 1.0f, 1.0f)));
				debug_draw_line(true, svc, sv.get().get_at(Vector3(0.0f, 0.0f, 1.0f)), sv.get().get_at(Vector3(0.0f, 1.0f, 1.0f)));
				debug_draw_line(true, svc, sv.get().get_at(Vector3(1.0f, 0.0f, 1.0f)), sv.get().get_at(Vector3(1.0f, 1.0f, 1.0f)));
				debug_draw_line(true, svc, sv.get().get_at(Vector3(0.0f, 0.0f, 0.0f)), sv.get().get_at(Vector3(0.0f, 0.0f, 1.0f)));
				debug_draw_line(true, svc, sv.get().get_at(Vector3(0.0f, 1.0f, 0.0f)), sv.get().get_at(Vector3(0.0f, 1.0f, 1.0f)));
				debug_draw_line(true, svc, sv.get().get_at(Vector3(1.0f, 0.0f, 0.0f)), sv.get().get_at(Vector3(1.0f, 0.0f, 1.0f)));
				debug_draw_line(true, svc, sv.get().get_at(Vector3(1.0f, 1.0f, 0.0f)), sv.get().get_at(Vector3(1.0f, 1.0f, 1.0f)));
			}
			debug_pop_transform();
		}
	}
	debug_no_subject();
	debug_no_context();
#endif
}

void ModulePresence::log_base_info(LogInfoContext & _log) const
{
	Optional<Colour> defColour = _log.get_colour();
	if (canBeBase)
	{
		if (lockAsBaseRequiresVRMovement || lockAsBaseRequiresPlayerMovement)
		{
			_log.set_colour(Colour::red);
			_log.log(TXT("can be base [locked for%S%S]"), lockAsBaseRequiresVRMovement ? TXT(" VR") : TXT(""), lockAsBaseRequiresPlayerMovement ? TXT(" player") : TXT(""));
		}
		else if (lockAsBase)
		{
			_log.set_colour(Colour::red);
			_log.log(TXT("can be base [locked]"));
		}
		else
		{
			_log.log(TXT("can be base"));
		}
		_log.set_colour(defColour);
	}
	if (canBeBasedOn && !basedOn.get_presence())
	{
		_log.log(TXT("can be based on"));
	}
	if (is_attached())
	{
		_log.log(TXT("can't base on as is attached"));
	}
	if (auto * bop = basedOn.get_presence())
	{
		if (bop->is_locked_as_base(this))
		{
			_log.set_colour(Colour::c64Brown);
		}
		_log.log(TXT("based on \"%S\"%S"), bop->get_owner()->ai_get_name().to_char(), bop->is_locked_as_base(this)? TXT(" locked") : TXT(" (nl)"));
	}
	else
	{
		_log.log(TXT("not based on"));
	}
	_log.set_colour(defColour);
}

void ModulePresence::log(LogInfoContext & _context) const
{
	{
		Vector3 l = placement.get_translation();
		_context.log(TXT("loc: %12.6f %12.6f %12.6f"), l.x, l.y, l.z);
		Rotator3 r = placement.get_orientation().to_rotator();
		_context.log(TXT("rot: %12.6f %12.6f %12.6f"), r.pitch, r.yaw, r.roll);
	}
	_context.log(TXT("room: %S"), get_in_room()? get_in_room()->get_name().to_char() : TXT("--"));
	if (is_attached())
	{
		_context.log(TXT("attached to: %S"), get_attached_to() ? get_attached_to()->ai_get_name().to_char() : TXT("??"));
		LOG_INDENT(_context);
		attachment.pathToAttachedTo.log(_context);
	}
	{
		LOG_INDENT(_context);
		if (presenceLinks)
		{
			log_presence_links(_context);
		}
		else
		{
			_context.log(TXT("no presence links"));
		}
		if (updatePresenceLinkSetup)
		{
			_context.log(TXT("update presence links"));
		}
	}
}

void ModulePresence::log_presence_links(LogInfoContext& _context) const
{
	_context.log(TXT("with presence links:"));
	{
		LOG_INDENT(_context);
		auto* p = presenceLinks;
		while (p)
		{
			_context.log(TXT("+- in \"%S\" %S:%S:%S:%S"), p->get_in_room() ? p->get_in_room()->get_name().to_char() : TXT("??"),
				p->isActualPresence? TXT("actPre") : TXT("-"),
				p->validForCollision? TXT("valCol") : TXT("-"),
				p->validForCollisionGradient? TXT("valColGrad") : TXT("-"),
				p->validForCollisionDetection? TXT("valColDet") : TXT("-")
				);
			p = p->nextInObject;
		}
	}
}

void ModulePresence::invalidate_presence_links()
{
#ifdef AN_DEVELOPMENT
	auto* pl = presenceLinks;
	while (pl)
	{
		an_assert_immediate(pl->owner == nullptr /* could be already invalidated */ || pl->owner == get_owner(), TXT("presence link %p for %p has different owner, expected %p\"%S\", has %p\"%S\""),
			pl, get_owner(),
			get_owner(), get_owner()->ai_get_name().to_char(),
			pl->owner, pl->owner == nullptr? TXT("[??]") : pl->owner->ai_get_name().to_char()
			);
		pl = pl->nextInObject;
	}
#endif
	if (presenceLinks)
	{
		an_assert_log_always(presenceLinks->ownerPresence == this || presenceLinks->ownerPresence == nullptr, TXT("this object or most likely in a room to delete"));
		an_assert_log_always(presenceLinks->prevInObject == nullptr);
		presenceLinks->invalidate_object();
		presenceLinks = nullptr;
	}
}

Vector3 ModulePresence::get_random_point_within_presence_os(float _radiusCoef) const
{
	Vector3 point = presenceLinkCentreOS.get_translation();
	point += Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f)).normal() * Random::get_float(0.0f, presenceLinkRadius) * _radiusCoef;
	if (! presenceLinkCentreDistance.is_zero())
	{
		point += presenceLinkCentreDistance * Random::get_float(-0.5f, 0.5f);
	}
	return point;
}

int ModulePresence::get_three_point_of_presence_os(ArrayStatic<Vector3, 3> & _threePointOffsetsOS) const
{
	_threePointOffsetsOS.set_size(0);
	_threePointOffsetsOS.push_back(presenceLinkCentreOS.get_translation());
	if (! presenceLinkCentreDistance.is_zero())
	{
		_threePointOffsetsOS.push_back(presenceLinkCentreOS.location_to_world(presenceLinkCentreDistance * -0.5f));
		_threePointOffsetsOS.push_back(presenceLinkCentreOS.location_to_world(presenceLinkCentreDistance * 0.5f));
	}
	return _threePointOffsetsOS.get_size();
}

bool ModulePresence::get_presence_primitive_info(OUT_ Vector3 & _locA, OUT_ Vector3 & _locB, OUT_ float & _radius) const
{
	_radius = presenceLinkRadius;
	Vector3 centre = placement.location_to_world(presenceLinkCentreOS.get_translation());
	if (presenceLinkCentreDistance.is_zero())
	{
		_locA = _locB = centre;
	}
	else
	{
		Vector3 halfCentreDistance = placement.to_world(presenceLinkCentreOS).vector_to_world(0.5f * presenceLinkCentreDistance);
		_locA = centre - halfCentreDistance;
		_locB = centre + halfCentreDistance;
	}
	return true;
}

float ModulePresence::get_presence_size() const
{
	return presenceLinkRadius * 2.0f + presenceLinkCentreDistance.length(); // height
}

World* ModulePresence::get_in_world() const
{
	return inRoom ? inRoom->get_in_world() : nullptr;
}

bool ModulePresence::move_through_doors(REF_ Transform & _nextPlacement, OUT_ Room *& _intoRoom) const
{
	Transform placement = get_centre_of_presence_transform_WS();
	_intoRoom = inRoom;
	return inRoom->move_through_doors(placement, REF_ _nextPlacement, REF_ _intoRoom);
}

bool ModulePresence::is_vr_attachment() const
{
	if (isVRAttachment)
	{
		return true;
	}
	if (attachment.attachedTo &&
		attachment.attachedTo->get_presence())
	{
		return attachment.attachedTo->get_presence()->is_vr_attachment();
	}
	return false;
}

void ModulePresence::update_placements_for_rendering_in_presence_links(Transform const & _placementChange)
{
	auto* presenceLink = presenceLinks;
	while (presenceLink)
	{
		presenceLink->update_placement_for_rendering_by(_placementChange);
		presenceLink = presenceLink->nextInObject;
	}
}

void ModulePresence::set_scale(float _scale)
{
	placement.set_scale(_scale);
	an_assert_immediate(placement.is_ok());
	an_assert_immediate(placement.get_scale() != 0.0f);
}

void ModulePresence::clear_velocity_impulses()
{
	if (!get_owner()->get_movement() ||
		get_owner()->get_movement()->is_immovable() ||
		(is_vr_movement() && !Game::is_using_sliding_locomotion()) ||
		ignoreImpulses)
	{
		// no point storing as we won't process that
		return;
	}

	Concurrency::ScopedSpinLock lock(velocityImpulsesLock);

	velocityImpulses.clear();
}

void ModulePresence::add_velocity_impulse(Vector3 const & _impulse)
{
	if (!get_owner()->get_movement() ||
		get_owner()->get_movement()->is_immovable() ||
		(is_vr_movement() && ! Game::is_using_sliding_locomotion()) ||
		ignoreImpulses)
	{
		// no point storing as we won't process that
		return;
	}

	Concurrency::ScopedSpinLock lock(velocityImpulsesLock);

	velocityImpulses.push_back(_impulse);
}

void ModulePresence::add_orientation_impulse(Rotator3 const & _impulse)
{
	if (!get_owner()->get_movement() ||
		get_owner()->get_movement()->is_immovable() ||
		(is_vr_movement() && !Game::is_using_sliding_locomotion()) ||
		ignoreImpulses)
	{
		// no point storing as we won't process that
		return;
	}

	Concurrency::ScopedSpinLock lock(orientationImpulsesLock);

	orientationImpulses.push_back(_impulse);
}

Vector3 ModulePresence::calc_average_velocity() const
{
	Vector3 velocitySum = velocityLinear;
	float velocityCount = 1.0f;
	for_every(v, prevVelocities)
	{
		velocitySum += *v;
		velocityCount += 1.0f;
	}
	return velocitySum / velocityCount;
}

Vector3 ModulePresence::get_max_velocity() const
{
	Vector3 maxVelocity = velocityLinear;
	float maxVelocityLength = maxVelocity.length_squared();
	for_every(v, prevVelocities)
	{
		float vl = v->length_squared();
		if (maxVelocityLength < vl)
		{
			maxVelocityLength = vl;
			maxVelocity = *v;
		}
	}
	return maxVelocity;
}

void ModulePresence::ready_to_activate()
{
	base::ready_to_activate();
	{
		volumetricBase.clear();
		if (volumetricBaseProvided.is_set())
		{
			volumetricBase = volumetricBaseProvided;
		}
		if (volumetricBaseFromAppearancesMesh)
		{
			if (auto* appearance = get_owner()->get_appearance())
			{
				if (Meshes::Mesh3D * mesh = fast_cast<Meshes::Mesh3D>(cast_to_nonconst(appearance->get_mesh()->get_mesh())))
				{
					mesh->update_bounding_box();
					volumetricBase = mesh->get_bounding_box();
					if (volumetricBaseProvided.is_set())
					{
						volumetricBase.access().include(volumetricBaseProvided.get());
					}
				}
				else
				{
					todo_important(TXT("handle!"));
				}
			}
		}
	}
	{
		safeVolume.clear();
		if (safeVolumeProvided.is_set())
		{
			safeVolume = safeVolumeProvided;
		}
		if (safeVolumeFromMeshNodes ||
			safeVolumeFromVolumetricBase)
		{
			safeVolume = Range3::empty;
			if (safeVolumeFromMeshNodes)
			{
				if (auto* appearance = get_owner()->get_appearance())
				{
					if (auto* mesh = appearance->get_mesh())
					{
						for_every_ref(mn, mesh->get_mesh_nodes())
						{
							if (mn->name == NAME(safeVolume))
							{
								safeVolume.access().include(mn->placement.get_translation());
							}
						}
					}
					else
					{
						todo_important(TXT("handle!"));
					}
				}
			}
			if (safeVolumeFromVolumetricBase)
			{
				if (volumetricBase.is_set())
				{
					safeVolume.access().include(volumetricBase.get());
				}
				else
				{
					error(TXT("\"safeVolumeFromVolumetricBase\" set but no volumetric base provided"));
				}
			}
			if (safeVolumeProvided.is_set())
			{
				safeVolume.access().include(safeVolumeProvided.get());
			}
		}
		if (!safeVolumeExpandBy.is_zero())
		{
			if (safeVolume.is_set())
			{
				safeVolume.access().expand_by(safeVolumeExpandBy);
			}
			else
			{
				error(TXT("\"safeVolumeExpandBy\" set but no safe volume provided"));
			}
		}
	}
}

void ModulePresence::cease_to_exist_when_not_attached(bool _ceaseToExistWhenNotAttached, int _flag)
{
	if (is_flag_set(ceaseToExistWhenNotAttached, _flag) == _ceaseToExistWhenNotAttached)
	{
		// nothing to be done/changed
		return;
	}
	if (_ceaseToExistWhenNotAttached)
	{
		set_flag(ceaseToExistWhenNotAttached, _flag);

		// set to true, as any flag set to true means that auto is set to true
		{
			Concurrency::ScopedSpinLock lock(attachedLock);

			for_every_ptr(ao, attached)
			{
				ao->get_presence()->cease_to_exist_when_not_attached(true, CeaseToExistWhenNotAttachedFlag::Auto);
			}
		}
	}
	else
	{
		clear_flag(ceaseToExistWhenNotAttached, _flag);

		// clear "auto" flag only if the whole flag set is false
		if (!ceaseToExistWhenNotAttached)
		{
			Concurrency::ScopedSpinLock lock(attachedLock);
			for_every_ptr(ao, attached)
			{
				ao->get_presence()->cease_to_exist_when_not_attached(false, CeaseToExistWhenNotAttachedFlag::Auto);
			}
		}
	}
}

#ifdef AN_DEVELOPMENT
void ModulePresence::assert_we_dont_move() const
{
	an_assert_immediate((is_owner_object() && !get_owner_as_object()->is_world_active()) ||
					 (is_owner_temporary_object() && !get_owner_as_temporary_object()->is_active()) ||
					 !get_owner()->get_in_world() ||
					 get_owner()->get_in_world()->multithread_check__reading_rooms_objects_is_safe() ||
					 Game::get_as<PreviewGame>());
}

void ModulePresence::assert_we_may_move() const
{
	an_assert_immediate((is_owner_object() && !get_owner_as_object()->is_world_active()) ||
					 (is_owner_temporary_object() && !get_owner_as_temporary_object()->is_active()) ||
					 !get_owner()->get_in_world() ||
					 get_owner()->get_in_world()->multithread_check__writing_rooms_objects_is_allowed() ||
					 Game::get_as<PreviewGame>());
}
#endif

void ModulePresence::refresh_attached()
{
	Concurrency::ScopedSpinLock lock(attachedLock);
	for_every_ptr(at, attached)
	{
		if (auto * atP = at->get_presence())
		{
			atP->refresh_attachment();
		}
	}
}

void ModulePresence::refresh_attachment()
{
	if (is_attached() && attachment.attachedTo)
	{
		auto* attachedToAppearance = attachment.attachedTo->get_appearance();
		if (attachedToAppearance)
		{
			if (attachment.toBone.is_set())
			{
				if (auto* fs = attachedToAppearance->get_skeleton())
				{
					attachment.toBoneIdx = fs->get_skeleton()->find_bone_index(attachment.toBone.get());
				}
			}
			if (attachment.toSocket.is_valid())
			{
				attachment.toSocket.look_up(attachedToAppearance->get_mesh(), ShouldAllowToFail::AllowToFail);
			}
		}
		if (attachment.viaSocket.is_valid())
		{
			if (auto* ourAppearance = get_owner()->get_appearance())
			{
				attachment.viaSocket.look_up(ourAppearance->get_mesh(), ShouldAllowToFail::AllowToFail);
			}
		}
	}
	Concurrency::ScopedSpinLock lock(attachedLock);
	for_every_ptr(at, attached)
	{
		if (auto * atP = at->get_presence())
		{
			atP->refresh_attachment();
		}
	}
}

void ModulePresence::make_attachment_valid_on_zero_scaled_bones()
{
	if (is_attached() && attachment.attachedTo)
	{
		if (auto* appearance = attachment.attachedTo->get_appearance())
		{
			if (auto* fs = appearance->get_skeleton())
			{
				// skip zero scaled bones (only if attached directly to them!) and go higher
				while (attachment.toBone.is_set())
				{
					if (appearance->get_final_pose_LS()->get_bone(attachment.toBoneIdx).get_scale() == 0.0f)
					{
						Transform pretendBone = appearance->get_final_pose_LS()->get_bone(attachment.toBoneIdx);
						pretendBone.set_scale(1.0f);
						attachment.toBoneIdx = fs->get_skeleton()->get_parent_of(attachment.toBoneIdx);
						attachment.toBone = fs->get_skeleton()->get_bones()[attachment.toBoneIdx].get_name();
						attachment.relativePlacement = pretendBone.to_world(attachment.relativePlacement);
						an_assert_immediate(attachment.relativePlacement.is_ok());
						if (!attachment.toBone.get().is_valid())
						{
							attachment.toBone.clear();
						}
					}
					else
					{
						break;
					}
				}
			}
		}
	}
}

float ModulePresence::calculate_flat_distance_for_nav(Vector3 const& _point) const
{
	Transform p = get_placement_for_nav();
	return p.location_to_local(_point).length_2d();
}

float ModulePresence::calculate_flat_distance(Vector3 const& _point) const
{
	Transform p = get_placement();
	return p.location_to_local(_point).length_2d();
}

Transform ModulePresence::calculate_current_placement() const
{
	if (is_attached())
	{
		return attachment.calculate_placement_of_attach(get_owner(), true);
	}
	return placement; // based on is done while moving
}

void ModulePresence::request_on_spawn_random_dir_teleport(Optional<float> const& _maxDist, Optional<float> const & _preferVertical, Optional<float> const& _offWall)
{
	if (is_attached())
	{
		return;
	}
	if (auto* object = get_owner_as_object())
	{
		MEASURE_PERFORMANCE(request_on_spawn_random_dir_teleport);

		Framework::CheckCollisionContext checkCollisionContext;
		checkCollisionContext.avoid(object, true);
		checkCollisionContext.ignore_actors();
		checkCollisionContext.ignore_items();
		checkCollisionContext.ignore_temporary_objects();
		checkCollisionContext.collision_info_needed();
		checkCollisionContext.use_collision_flags(object->get_collides_with_flags());

		Transform placement = get_placement();

		Random::Generator traceGenerator = object->get_individual_random_generator();
		traceGenerator.advance_seed(9754, 39784);
		for_count(int, tryNo, 20)
		{
			Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::WS);
			Vector3 startAt = placement.location_to_world(get_centre_of_presence_os().get_translation());
			float preferVertical = _preferVertical.get(0.0f);
			float allowSides = 1.0f - preferVertical;
			Vector3 endAt = startAt + placement.vector_to_world(Vector3(traceGenerator.get_float(-allowSides, allowSides), traceGenerator.get_float(-allowSides, allowSides), traceGenerator.get_float(0.2f - preferVertical, 1.0f)).normal() * _maxDist.get(4.5f));
			collisionTrace.add_location(startAt);
			collisionTrace.add_location(endAt);
			Framework::CheckSegmentResult result;
			if (trace_collision(AgainstCollision::Movement, collisionTrace, result, Framework::CollisionTraceFlag::ResultInFinalRoom, checkCollisionContext))
			{
				if (result.inRoom == get_in_room() &&
					!result.hitNormal.is_zero())
				{
					result.hitLocation += (startAt - result.hitLocation).normal() * _offWall.get(0.03f);
					Vector3 at = result.hitLocation + placement.get_axis(Axis::Y);
					if (abs(Vector3::dot((at - result.hitLocation).normal(), result.hitNormal)) > 0.95f)
					{
						// just add random bit
						at.x += result.hitNormal.y * 0.1f;
						at.y += result.hitNormal.z * 0.1f;
						at.z += result.hitNormal.x * 0.1f;
					}
					placement = look_at_matrix(result.hitLocation, at, result.hitNormal).to_transform();
					// change placement
					request_teleport_within_room(placement);
					
					disallow_falling(1.0f); // for walkers and gravity dir to catch up

					break;
				}
			}
		}
	}
}

bool ModulePresence::is_locked_as_base(ModulePresence const* _for) const
{
	if (canBeBase && lockAsBase)
	{
		int req = 0;
		int ok = 0;
		if (! ok && lockAsBaseRequiresVRMovement)
		{
			++req;
			ok += _for->is_vr_movement() ? 1 : 0;
		}
		if (! ok && lockAsBaseRequiresPlayerMovement)
		{
			++req;
			ok += _for->is_player_movement() ? 1 : 0;
		}
		return req == 0 || ok;
	}
	return false;
}

void ModulePresence::make_head_bone_os_safe(REF_ Transform& _headBoneOS) const
{
	if (auto* collision = get_owner()->get_collision())
	{
		Vector3 headLocOS = _headBoneOS.get_translation();

		Vector3 startOS = get_centre_of_presence_os().get_translation();

		Collision::Flags flags = collision->get_collides_with_flags();

		// do this check only if we actually are supposed to check collisions
		if (!flags.is_empty())
		{
			float extraDist = 0.1f;
			float minHeadAltitude = 0.1f;

			if (auto* ch = fast_cast<ModuleCollisionHeaded>(collision))
			{
				extraDist = ch->get_above_head_height();
				minHeadAltitude = ch->get_min_head_altitude();
				flags = ch->get_head_collides_with_flags();
			}

			startOS.z = max(minHeadAltitude, collision->get_collision_primitive_centre_offset().z - collision->get_collision_primitive_centre_distance().z * 0.5f);

			Vector3 startToHeadOS = headLocOS - startOS;
			float startToHeadLen = startToHeadOS.length();
			if (startToHeadLen != 0.0f)
			{
				Vector3 extraHeadLocOS = startOS + startToHeadOS * ((extraDist + startToHeadLen) / startToHeadLen);

				CheckCollisionCache cache;
				cache.build_from_presence_links(get_in_room(), get_placement(), modulePresenceData->basedOnPresenceTraces);

				ArrayStatic<Vector3, 4> offsets;
				{
					float offset = extraDist * 0.3f;
					offsets.push_back(offset * Vector3(1.0f, 1.0f, 0.0f));
					offsets.push_back(offset * Vector3(1.0f, -1.0f, 0.0f));
					offsets.push_back(offset * Vector3(-1.0f, 1.0f, 0.0f));
					offsets.push_back(offset * Vector3(-1.0f, -1.0f, 0.0f));
				}

				Optional<float> lowestZ;

				for_every(offset, offsets)
				{
					Vector3 useStartOS = startOS;
					Vector3 useEndOS = extraHeadLocOS + *offset;
					CollisionTrace collisionTrace(CollisionTraceInSpace::OS);
					collisionTrace.add_location(useStartOS);
					collisionTrace.add_location(useEndOS);

					CheckCollisionContext checkCollisionContext;
					checkCollisionContext.collision_info_not_needed();
					checkCollisionContext.ignore_actors();
					checkCollisionContext.ignore_items();
					checkCollisionContext.ignore_temporary_objects();
					checkCollisionContext.use_collision_flags(flags);
					checkCollisionContext.avoid(get_owner());
					checkCollisionContext.use_cache(&cache);
					collision->add_not_colliding_with_to(checkCollisionContext);

					CheckSegmentResult result;

					if (get_owner()->get_presence()->trace_collision(AgainstCollision::Movement, collisionTrace, result, CollisionTraceFlag::ResultInObjectSpace, checkCollisionContext))
					{
						Vector3 hit = result.hitLocation;
						float startToHit = (hit.z - useStartOS.z);

						float z = useStartOS.z + max(0.0f, startToHit - extraDist);
						if (!lowestZ.is_set() || z < lowestZ.get())
						{
							lowestZ = z;
						}
					}
				}

				if (lowestZ.is_set())
				{
					headLocOS.z = lowestZ.get();
					_headBoneOS.set_translation(headLocOS);
				}
			}
		}
	}
}

Transform ModulePresence::calculate_environment_up_placement() const
{
	Vector3 upDir = get_environment_up_dir();
	return matrix_from_up_forward(get_placement().get_translation(), upDir, get_placement().get_axis(Axis::Forward)).to_transform();
}

void ModulePresence::set_light_based_presence_radius(Optional<float> const& _lightRadius)
{
	Optional<float> useLightRadius = _lightRadius;
	if (useLightRadius.is_set())
	{
		useLightRadius = ceil_to(useLightRadius.get(), 0.2f);
	}
	if (lightSourceBasedPresenceRadius != _lightRadius)
	{
		//MODULE_OWNER_LOCK(TXT("light sources based presence radius changed")); this is just a visual thing
		lightSourceBasedPresenceRadius = _lightRadius;
		force_update_presence_links();
	}
}

#ifdef WITH_PRESENCE_INDICATOR
void ModulePresence::update_presence_indicator()
{
	if (presenceIndicatorVisible)
	{
		if (auto* a = get_owner()->get_appearance_named(NAME(presenceIndicator)))
		{
			if (a->get_name() == NAME(presenceIndicator))
			{
				a->access_mesh_instance().set_rendering_order_priority(100000);

				Colour colour = Colour::green;
				if (auto* bop = basedOn.get_presence())
				{
					if (bop->is_locked_as_base(this))
					{
						colour = Colour::red;
					}
				}
				for_count(int, i, a->access_mesh_instance().get_material_instance_count())
				{
					a->set_shader_param(NAME(indicatorColour), colour.to_vector4(), i);
				}
			}
		}
	}
}
#endif

void ModulePresence::for_every_attached(std::function<void(IModulesOwner*)> _attached)
{
	Concurrency::ScopedSpinLock lock(attachedLock, true);
	for_every_ptr(imo, attached)
	{
		_attached(imo);
		imo->get_presence()->for_every_attached(_attached);
	}
}
