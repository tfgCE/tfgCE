#include "rotateTowards.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAI.h"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\moduleSound.h"
#include "..\..\presence\presencePath.h"
#include "..\..\presence\relativeToPresencePlacement.h"
#include "..\..\world\doorInRoom.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(rotateTowards);

//

REGISTER_FOR_FAST_CAST(RotateTowards);

RotateTowards::RotateTowards(RotateTowardsData const * _data)
: base(_data)
, rotateTowardsData(_data)
{
	bone = rotateTowardsData->bone.get();
	rotateYaw = rotateTowardsData->rotateYaw;
	rotatePitch = rotateTowardsData->rotatePitch;
	if (rotateTowardsData->pointBone.is_set())
	{
		pointBone = rotateTowardsData->pointBone.get();
	}
	if (rotateTowardsData->pointBonePlaneNormalBS.is_set())
	{
		pointBonePlaneNormalBS = rotateTowardsData->pointBonePlaneNormalBS.get();
		if (!pointBonePlaneNormalBS.is_zero())
		{
			pointBonePlaneNormalBS.normalise();
		}
	}
	else
	{
		pointBonePlaneNormalBS = Vector3::zero;
	}
	if (rotateTowardsData->alignYawUsingTargetPlacementsPlaneNormalLS.is_set())
	{
		alignYawUsingTargetPlacementsPlaneNormalLS = rotateTowardsData->alignYawUsingTargetPlacementsPlaneNormalLS.get();
		if (!alignYawUsingTargetPlacementsPlaneNormalLS.is_zero())
		{
			alignYawUsingTargetPlacementsPlaneNormalLS.normalise();
		}
	}
	else
	{
		alignYawUsingTargetPlacementsPlaneNormalLS = Vector3::zero;
	}
	defaultTargetPointMS = rotateTowardsData->defaultTargetPointMS.get();
	if (rotateTowardsData->defaultTargetDirMS.is_set())
	{
		defaultTargetDirMS = rotateTowardsData->defaultTargetDirMS.get();
	}
	if (rotateTowardsData->viewOffsetBS.is_set())
	{
		viewOffsetBS = rotateTowardsData->viewOffsetBS.get();
	}
	if (rotateTowardsData->targetBone.is_set())
	{
		targetBone = rotateTowardsData->targetBone.get();
	}
	if (rotateTowardsData->targetSocketVar.is_set())
	{
		targetSocketVar = rotateTowardsData->targetSocketVar.get();
	}
	if (rotateTowardsData->targetPointMSVar.is_set())
	{
		targetPointMSVar = rotateTowardsData->targetPointMSVar.get();
	}
	if (rotateTowardsData->targetPointMSPtrVar.is_set())
	{
		targetPointMSPtrVar = rotateTowardsData->targetPointMSPtrVar.get();
	}
	if (rotateTowardsData->targetPlacementMSVar.is_set())
	{
		targetPlacementMSVar = rotateTowardsData->targetPlacementMSVar.get();
	}
	if (rotateTowardsData->rotationAxisBS.is_set())
	{
		Vector3 rotationAxisBS = rotateTowardsData->rotationAxisBS.get().normal();
		Vector3 forwardDirBS = Vector3::yAxis;
		if (rotateTowardsData->forwardDirBS.is_set())
		{
			forwardDirBS = rotateTowardsData->forwardDirBS.get().normal();
		}
		if (abs(Vector3::dot(forwardDirBS, rotationAxisBS)) > 0.99f)
		{
			error(TXT("rotation axis and forward axis aligned"));
		}
		else
		{
			rotationSpaceBS = look_matrix33(forwardDirBS, rotationAxisBS).to_quat();
		}
	}
	if (rotateTowardsData->targetPresencePathVar.is_set())
	{
		targetPresencePathVar = rotateTowardsData->targetPresencePathVar.get();
	}
	if (rotateTowardsData->relativeToPresencePathVar.is_set())
	{
		relativeToPresencePathVar = rotateTowardsData->relativeToPresencePathVar.get();
	}
	if (rotateTowardsData->relativeToPresencePathOffsetOSPtrVar.is_set())
	{
		relativeToPresencePathOffsetOSPtrVar = rotateTowardsData->relativeToPresencePathOffsetOSPtrVar.get();
	}
	if (rotateTowardsData->outputRelativeTargetLocVar.is_set())
	{
		outputRelativeTargetLocVar = rotateTowardsData->outputRelativeTargetLocVar.get();
	}
	range = rotateTowardsData->range.get();
	forwardOrBackward = rotateTowardsData->forwardOrBackward.get();
	if (rotateTowardsData->rotationSpeedVar.is_set())
	{
		rotationSpeedVar = rotateTowardsData->rotationSpeedVar.get();
	}
	if (rotateTowardsData->outputAtTargetVar.is_set())
	{
		outputAtTargetVar = rotateTowardsData->outputAtTargetVar.get();
	}
	rotationSpeed = rotateTowardsData->rotationSpeed.get();
	rotationSound = rotateTowardsData->rotationSound.get(rotationSound);
	rotationThreshold = rotateTowardsData->rotationThreshold.get(0.0f);
}

RotateTowards::~RotateTowards()
{
}

void RotateTowards::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	bone.look_up(get_owner()->get_skeleton()->get_skeleton());
	pointBone.look_up(get_owner()->get_skeleton()->get_skeleton());
	targetBone.look_up(get_owner()->get_skeleton()->get_skeleton());

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	targetPresencePathVar.look_up<PresencePath*>(varStorage);
	relativeToPresencePathVar.look_up<RelativeToPresencePlacement*>(varStorage);
	relativeToPresencePathOffsetOSPtrVar.look_up<Vector3*>(varStorage);
	targetSocketVar.look_up<Name>(varStorage);
	targetPointMSVar.look_up<Vector3>(varStorage);
	targetPointMSPtrVar.look_up<Vector3*>(varStorage);
	targetPlacementMSVar.look_up<Transform>(varStorage);
	rotationSpeedVar.look_up<float>(varStorage);
	outputAtTargetVar.look_up<bool>(varStorage);
	outputRelativeTargetLocVar.look_up<Vector3>(varStorage);
}

void RotateTowards::internal_log(LogInfoContext & _log) const
{
	base::internal_log(_log);
	_log.log(TXT("bone: %S"), bone.get_name().to_char());
	if (pointBone.is_valid())
	{
		_log.log(TXT("pointBone: %S"), pointBone.get_name().to_char());
	}
	if (targetBone.is_valid())
	{
		_log.log(TXT("targetBone: %S"), targetBone.get_name().to_char());
	}
}

void RotateTowards::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void RotateTowards::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(rotateTowards_finalPose);
#endif

	debug_filter(ac_rotateTowards);
	debug_context(this->get_owner()->get_owner()->get_presence()->get_in_room());

	bool processRotation = true;
	bool playSound = false;

	if (get_active() == 0.0f)
	{
		lastNonHoldPoseLS.clear();
		processRotation = false;
	}

	Meshes::Pose * poseLS = _context.access_pose_LS();

	// get bones early
	int const boneIdx = bone.get_index();
	int const boneParentIdx = poseLS->get_skeleton()->get_parent_of(boneIdx);

	if (processRotation && should_hold() && lastNonHoldPoseLS.is_set())
	{
		poseLS->set_bone(boneIdx, lastNonHoldPoseLS.get());
		processRotation = false;
	}

	if (rotationSpeedVar.is_valid())
	{
		rotationSpeed = rotationSpeedVar.get<float>();
	}

	Transform const boneParentMS = poseLS->get_bone_ms_from_ls(boneParentIdx);
	Transform const boneLS = poseLS->get_bone(boneIdx);
	Transform const boneMS = boneParentMS.to_world(boneLS);

	// get target point
	Vector3 targetPointMS = defaultTargetPointMS;
	Vector3 targetPlaneNormalMS = Vector3::zero;
	bool targetPointProvided = false;

	if (targetBone.is_valid())
	{
		targetPointMS = poseLS->get_bone_ms_from_ls(targetBone.get_index()).get_translation();
		targetPointProvided = true;
	}
	if (targetSocketVar.is_valid())
	{
		Name targetSocketName = targetSocketVar.get<Name>();
		if (targetSocketName != targetSocket.get_name())
		{
			targetSocket.set_name(targetSocketName);
			targetSocket.look_up(get_owner()->get_mesh());
		}
		targetPointMS = get_owner()->calculate_socket_ms(targetSocket.get_index(), poseLS).get_translation();
		targetPointProvided = true;
	}

	if (targetPointMSVar.is_valid())
	{
		targetPointMS = targetPointMSVar.get<Vector3>();
		targetPointProvided = true;
	}
	if (targetPointMSPtrVar.is_valid())
	{
		auto * v3 = targetPointMSPtrVar.get<Vector3*>();
		if (v3)
		{
			targetPointMS = *v3;
		}
		targetPointProvided = true;
	}
	if (targetPlacementMSVar.is_valid())
	{
		Transform const targetPlacementMS = targetPlacementMSVar.get<Transform>();
		if (!alignYawUsingTargetPlacementsPlaneNormalLS.is_zero())
		{
			targetPlaneNormalMS = targetPlacementMS.vector_to_world(alignYawUsingTargetPlacementsPlaneNormalLS);
		}
		targetPointMS = targetPlacementMS.get_translation();
		targetPointProvided = true;
	}
	if (targetPresencePathVar.is_valid())
	{
		scoped_call_stack_info(TXT("rotate towards targetPresencePathVar"));
		scoped_call_stack_info(get_owner()->get_owner()->ai_get_name().to_char());
		PresencePath* presencePath = targetPresencePathVar.get<PresencePath*>();
		if (presencePath && presencePath->is_active() && presencePath->get_target())
		{
			scoped_call_stack_info(TXT("check path's target and if valid"));
			scoped_call_stack_info(presencePath->get_target()->ai_get_name().to_char());
			if (presencePath->is_path_valid())
			{
				Optional<Vector3> targetAtTargetsOS;
				if (auto* ai = get_owner()->get_owner()->get_ai())
				{
					targetAtTargetsOS = ai->get_target_placement_os_for(presencePath->get_target()).get_translation();
				}
				if (targetAtTargetsOS.is_set())
				{
					lastTargetLocationOS = targetAtTargetsOS;
				}
				else
				{
					targetAtTargetsOS = lastTargetLocationOS;
				}
				targetAtTargetsOS.set_if_not_set(Vector3::zero);
				Vector3 targetAtLocalWS = presencePath->location_from_target_to_owner(presencePath->get_target_presence()->get_placement().location_to_world(targetAtTargetsOS.get()));
				targetPointMS = get_owner()->get_ms_to_ws_transform().location_to_local(targetAtLocalWS);
				targetPointProvided = true;
			}
		}
	}
	if (relativeToPresencePathVar.is_valid())
	{
		RelativeToPresencePlacement* relativeToPresencePath = relativeToPresencePathVar.get<RelativeToPresencePlacement*>();
		if (relativeToPresencePath && relativeToPresencePath->is_active())
		{
			Optional<Vector3> targetAtTargetsOS;
			if (auto* ai = get_owner()->get_owner()->get_ai())
			{
				if (relativeToPresencePath->get_target())
				{
					targetAtTargetsOS = ai->get_target_placement_os_for(relativeToPresencePath->get_target()).get_translation();
				}
				else if (relativeToPresencePathOffsetOSPtrVar.is_valid())
				{
					if (Vector3* relativeToPresencePathOffsetOS = relativeToPresencePathOffsetOSPtrVar.get<Vector3*>())
					{
						targetAtTargetsOS = *relativeToPresencePathOffsetOS;
					}
				}
			}
			if (targetAtTargetsOS.is_set())
			{
				lastTargetLocationOS = targetAtTargetsOS;
			}
			else
			{
				targetAtTargetsOS = lastTargetLocationOS;
			}
			targetAtTargetsOS.set_if_not_set(Vector3::zero);
			Vector3 targetAtLocalWS = relativeToPresencePath->get_placement_in_owner_room().location_to_world(targetAtTargetsOS.get());
			debug_draw_arrow(true, Colour::orange, get_owner()->get_owner()->get_presence()->get_placement().location_to_world(boneMS.get_translation()), targetAtLocalWS);
			debug_draw_arrow(true, Colour::orange, get_owner()->get_owner()->get_presence()->get_placement().location_to_world(boneMS.get_translation()), relativeToPresencePath->get_placement_in_owner_room().get_translation());
			debug_draw_text(true, Colour::orange, targetAtLocalWS, NP, true, NP, NP, TXT("r2pp:%c d:%i"), relativeToPresencePath->get_target() ? 't' : 'p', relativeToPresencePath->get_through_doors().get_size());
			float flattenTargetPointMS = 0.0f;
			if (rotateTowardsData->lookAtDoorIfRelativeToPresencePathHasNoTarget &&
				!relativeToPresencePath->get_target() &&
				!relativeToPresencePath->get_through_doors().is_empty())
			{
				if (auto* door = relativeToPresencePath->get_through_doors().get_first().get())
				{
					// look at the centre of door if we do not see enemy directly
					targetAtLocalWS = door->get_hole_centre_WS();
					float dist = (targetAtLocalWS - get_owner()->get_owner()->get_presence()->get_placement().location_to_world(boneMS.get_translation())).length();
					float distThreshold = 2.0f;
					float useForward = clamp(1.5f * (1.0f - dist / distThreshold), 0.0f, 1.0f);
					targetAtLocalWS += door->get_outbound_matrix().vector_to_world(Vector3::yAxis * useForward * 2.0f);
					flattenTargetPointMS = useForward;
					debug_draw_arrow(true, Colour::yellow, get_owner()->get_owner()->get_presence()->get_placement().location_to_world(boneMS.get_translation()), targetAtLocalWS);
					debug_draw_text(true, Colour::yellow, targetAtLocalWS, NP, true, NP, NP, TXT("door"));
				}
			}
			targetPointMS = get_owner()->get_ms_to_ws_transform().location_to_local(targetAtLocalWS);
			targetPointProvided = true;
			if (flattenTargetPointMS > 0.0f)
			{
				// this is to look through the door
				targetPointMS.z += (boneMS.get_translation().z - targetPointMS.z) * flattenTargetPointMS;
			}
		}
	}
	if (!defaultTargetDirMS.is_zero())
	{
		targetPointMS = boneMS.get_translation() + defaultTargetDirMS;
		targetPointProvided = true;
	}

	if (processRotation)
	{
		debug_push_transform(this->get_owner()->get_owner()->get_presence()->get_placement());

		// calculate how it is currently rotated
		Rotator3 rotationRequiredRS = Rotator3::zero;

		if (targetPointProvided)
		{
			debug_draw_arrow(true, Colour::green, boneMS.get_translation(), targetPointMS);
			Vector3 targetPointBS = boneMS.location_to_local(targetPointMS) - viewOffsetBS; // apply view offset
			if (!targetPlaneNormalMS.is_zero())
			{
				an_assert(rotateYaw && !rotatePitch, TXT("not handled yet"));
				// instead of finding point, we use plane's normal
				Vector3 targetPlaneNormalBS = boneMS.vector_to_local(targetPlaneNormalMS);
				Vector3 targetPlaneNormalRS = rotationSpaceBS.to_local(targetPlaneNormalBS);
				Rotator3 targetPlaneNormalRotation = targetPlaneNormalRS.to_rotator();
				rotationRequiredRS.yaw = Rotator3::normalise_axis(targetPlaneNormalRotation.yaw - Rotator3::get_yaw(alignYawUsingTargetPlacementsPlaneNormalLS));
				rotationRequiredRS.pitch = 0.0f;
			}
			else
			{
				Vector3 targetPointRS = rotationSpaceBS.to_local(targetPointBS);
				rotationRequiredRS = targetPointRS.to_rotator();
				if (!rotateYaw)
				{
					rotationRequiredRS.yaw = 0.0f;
				}
				if (!rotatePitch)
				{
					rotationRequiredRS.pitch = 0.0f;
				}
			}
			rotationRequiredRS.roll = 0.0f;
		}

		// if we're blending out and already we've cleared target, pretend that we are still using the old one
		if (!targetPointProvided && is_deactivating())
		{
			rotationRequiredRS = lastRotationRequiredAsProvidedRS;
		}
		else
		{
			lastRotationRequiredAsProvidedRS = rotationRequiredRS;
		}

		if (pointBone.is_valid())
		{
			if (!pointBonePlaneNormalBS.is_zero())
			{
				// consider point bone placement and axis, we should have target point on point bone plane
				Transform pointBoneMS = poseLS->get_bone_ms_from_ls(pointBone.get_index());

				{	// we have to alter point bone MS to appear as it would be in required rotation, that would match target point MS
					// get to bone space (boneMS) rotate bone LS and get back to MS (boneParentMS -> rotatedBoneLS -> pointBoneInBoneLS
					Transform pointBoneInBoneLS = boneMS.to_local(pointBoneMS);
					Transform rotatedBoneLS = boneLS;
					rotatedBoneLS.set_orientation(rotatedBoneLS.get_orientation().rotate_by(rotationSpaceBS.to_world(rotationRequiredRS.to_quat())));
					pointBoneMS = boneParentMS.to_world(rotatedBoneLS).to_world(pointBoneInBoneLS);
				}

				Vector3 pointBoneToTargetPointMS = targetPointMS - pointBoneMS.get_translation();
				Vector3 alongPlaneNormalMS = pointBoneToTargetPointMS.along_normalised(pointBoneMS.vector_to_world(pointBonePlaneNormalBS));

				// we're interested in difference between how it would be non altered by using plane and how it would be altered, change by that difference
				Vector3 nonAlteredPointBoneBS = boneMS.location_to_local(pointBoneMS.get_translation());
				Vector3 alteredPointBoneBS = boneMS.location_to_local(pointBoneMS.get_translation() + alongPlaneNormalMS);
				Rotator3 alteredPointBoneRotationRS = rotationSpaceBS.to_local(alteredPointBoneBS).to_rotator();
				Rotator3 nonAlteredPointBoneRotationRS = rotationSpaceBS.to_local(nonAlteredPointBoneBS).to_rotator();
				if (rotateYaw)
				{
					rotationRequiredRS.yaw += (alteredPointBoneRotationRS.yaw - nonAlteredPointBoneRotationRS.yaw);
				}
				if (rotatePitch)
				{
					rotationRequiredRS.pitch += (alteredPointBoneRotationRS.pitch - nonAlteredPointBoneRotationRS.pitch);
				}
			}
			else
			{
				// consider point bone placement, if it is pointing
				Transform pointBoneMS = poseLS->get_bone_ms_from_ls(pointBone.get_index());

				// we do not have to use altered to non altered as current point bone we may predict difference in rotation required,
				// if it's slightly to the right, get rotation required to the left, as that offset will cover required difference
				Vector3 pointBS = boneMS.location_to_local(pointBoneMS.get_translation());
				Vector3 pointRS = rotationSpaceBS.to_local(pointBS);
				if (rotateYaw)
				{
					rotationRequiredRS.yaw -= pointRS.to_rotator().yaw;
				}
				if (rotatePitch)
				{
					rotationRequiredRS.pitch -= pointRS.to_rotator().pitch;
				}
			}
		}

		if (!range.is_empty())
		{
			float centre = range.centre();
			if (rotateYaw)
			{
				float normalisedOffset = Rotator3::normalise_axis(rotationRequiredRS.yaw - centre);
				if (forwardOrBackward)
				{
					if (abs(normalisedOffset) > 90.0f && range.length() < 180.0f)
					{
						normalisedOffset = Rotator3::normalise_axis(normalisedOffset - 180.0f);
					}
				}
				else
				{
					rotationRequiredRS.yaw = range.clamp(centre + normalisedOffset);
				}
			}
			if (rotatePitch)
			{
				float normalisedOffset = Rotator3::normalise_axis(rotationRequiredRS.pitch - centre);
				if (forwardOrBackward)
				{
					if (abs(normalisedOffset) > 90.0f && range.length() < 180.0f)
					{
						normalisedOffset = Rotator3::normalise_axis(normalisedOffset - 180.0f);
					}
				}
				else
				{
					rotationRequiredRS.pitch = range.clamp(centre + normalisedOffset);
				}
			}
		}
		todo_note(TXT("check if target point is not too close, if so, get back to default or middle point of range or maybe stop rotating"));

		// apply activation here, we will use speed to get there
		rotationRequiredRS = rotationRequiredRS * get_active();

		Rotator3 fullRotationRequiredBS = rotationRequiredRS;

		// ok, this assumes that there is no other input on bone before this kicks in, for most cases this should be fine the way it is
		if (rotationSpeed >= 0.0f)
		{
			Rotator3 requiredRotationDelta = (rotationRequiredRS - lastRotationRequiredRS).normal_axes();
			requiredRotationDelta.roll = 0.0f;
			if (!rotateYaw)
			{
				requiredRotationDelta.yaw = 0.0f;
			}
			if (!rotatePitch)
			{
				requiredRotationDelta.pitch = 0.0f;
			}
			float allowedRotation = rotationSpeed * _context.get_delta_time();
			an_assert(allowedRotation >= 0.0f);
			float requiredRotationDeltaAmount = requiredRotationDelta.length();
			if (requiredRotationDeltaAmount > allowedRotation)
			{
				requiredRotationDelta *= allowedRotation / requiredRotationDeltaAmount;
			}
			rotationRequiredRS.yaw = Rotator3::normalise_axis(lastRotationRequiredRS.yaw + requiredRotationDelta.yaw);
			rotationRequiredRS.pitch = Rotator3::normalise_axis(lastRotationRequiredRS.pitch + requiredRotationDelta.pitch);
		}

		if (outputAtTargetVar.is_valid())
		{
			outputAtTargetVar.access<bool>() = (rotationRequiredRS - fullRotationRequiredBS).length() <= 0.001f;
		}

		if (rotationThreshold > 0.0f)
		{
			if ((rotationRequiredRS - lastRotationRequiredRS).length() <= rotationThreshold)
			{
				rotationRequiredRS = lastRotationRequiredRS;
			}
		}

		// rotate towards requested point
		Quat rotationRequiredRSQuat = rotationRequiredRS.to_quat();

		// sounds
		if (rotationSound.is_valid())
		{
			bool sameRot = rotationRequiredRS == lastRotationRequiredRS;
			playSound = !sameRot;
		}

		lastRotationRequiredRS = rotationRequiredRS;

#ifdef AN_DEVELOPMENT
#ifndef AN_CLANG
		Vector3 f = rotationSpaceBS.to_matrix().vector_to_world(Vector3::yAxis);
		Vector3 r = rotationSpaceBS.to_matrix().vector_to_world(Vector3::xAxis);
		Vector3 u = rotationSpaceBS.to_matrix().vector_to_world(Vector3::zAxis);
#endif
#endif

		if (!rotateTowardsData->pureRotationSpace)
		{
			rotationRequiredRSQuat = rotationSpaceBS.to_world(rotationRequiredRSQuat);
		}

		// write back
		poseLS->access_bones()[boneIdx].set_orientation(boneLS.get_orientation().rotate_by(rotationRequiredRSQuat));
		lastNonHoldPoseLS = poseLS->get_bone(boneIdx);

		debug_pop_transform();
	}

	if (targetPointProvided && outputRelativeTargetLocVar.is_valid())
	{
		Transform boneMS = poseLS->get_bone_ms_from_ls(boneIdx);
		Vector3 targetPointLS = boneMS.location_to_local(targetPointMS);
		outputRelativeTargetLocVar.access<Vector3>() = targetPointLS;
	}

	// sounds
	if (rotationSound.is_valid())
	{
		if (!playedRotationSound.is_set() && playSound)
		{
			if (auto* ms = get_owner()->get_owner()->get_sound())
			{
				playedRotationSound = ms->play_sound(rotationSound);
			}
		}
		if (playedRotationSound.is_set() && !playSound)
		{
			playedRotationSound->stop();
			playedRotationSound = nullptr;
		}
	}


	debug_no_context();
	debug_no_filter();
}

void RotateTowards::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (bone.is_valid())
	{
		dependsOnParentBones.push_back(bone.get_index());
		providesBones.push_back(bone.get_index());
	}
	if (pointBone.is_valid())
	{
		usesBones.push_back(pointBone.get_index());
	}

	if (targetPointMSVar.is_valid())
	{
		dependsOnVariables.push_back(targetPointMSVar.get_name());
	}
	if (targetPointMSPtrVar.is_valid())
	{
		dependsOnVariables.push_back(targetPointMSPtrVar.get_name());
	}
	if (targetPlacementMSVar.is_valid())
	{
		dependsOnVariables.push_back(targetPlacementMSVar.get_name());
	}
	if (targetPresencePathVar.is_valid())
	{
		dependsOnVariables.push_back(targetPresencePathVar.get_name());
	}
	if (relativeToPresencePathVar.is_valid())
	{
		dependsOnVariables.push_back(relativeToPresencePathVar.get_name());
	}
	if (relativeToPresencePathOffsetOSPtrVar.is_valid())
	{
		dependsOnVariables.push_back(relativeToPresencePathOffsetOSPtrVar.get_name());
	}
	if (rotationSpeedVar.is_valid())
	{
		dependsOnVariables.push_back(rotationSpeedVar.get_name());
	}
	if (outputAtTargetVar.is_valid())
	{
		providesVariables.push_back(outputAtTargetVar.get_name());
	}
	// outputRelativeTargetLocVar is output
}

//

REGISTER_FOR_FAST_CAST(RotateTowardsData);

AppearanceControllerData* RotateTowardsData::create_data()
{
	return new RotateTowardsData();
}

void RotateTowardsData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(rotateTowards), create_data);
}

RotateTowardsData::RotateTowardsData()
{
}

bool RotateTowardsData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	
	result &= pointBone.load_from_xml(_node, TXT("pointBone"));
	result &= pointBonePlaneNormalBS.load_from_xml_child_node(_node, TXT("pointBonePlaneNormalBS"));
	
	result &= alignYawUsingTargetPlacementsPlaneNormalLS.load_from_xml_child_node(_node, TXT("alignYawUsingTargetPlacementsPlaneNormalLS"));
	
	result &= defaultTargetPointMS.load_from_xml_child_node(_node, TXT("defaultTargetPointMS"));
	result &= defaultTargetDirMS.load_from_xml_child_node(_node, TXT("defaultTargetDirMS"));
	result &= viewOffsetBS.load_from_xml_child_node(_node, TXT("viewOffsetBS"));

	result &= targetBone.load_from_xml(_node, TXT("targetBone"));
	result &= targetSocketVar.load_from_xml(_node, TXT("targetSocketVarID"));
	result &= targetPointMSVar.load_from_xml(_node, TXT("targetPointMSVarID"));
	result &= targetPointMSPtrVar.load_from_xml(_node, TXT("targetPointMSPtrVarID"));
	result &= targetPlacementMSVar.load_from_xml(_node, TXT("targetPlacementMSVarID"));
	result &= targetPresencePathVar.load_from_xml(_node, TXT("targetPresencePathVarID"));
	result &= relativeToPresencePathVar.load_from_xml(_node, TXT("relativeToPresencePathVarID"));
	result &= relativeToPresencePathOffsetOSPtrVar.load_from_xml(_node, TXT("relativeToPresencePathOffsetOSPtrVarID"));

	result &= rotationAxisBS.load_from_xml_child_node(_node, TXT("rotationAxisBS"));
	result &= forwardDirBS.load_from_xml_child_node(_node, TXT("forwardDirBS"));

	result &= range.load_from_xml(_node, TXT("range"));
	result &= forwardOrBackward.load_from_xml(_node, TXT("forwardOrBackward"));

	result &= outputRelativeTargetLocVar.load_from_xml(_node, TXT("outputRelativeTargetLocVarID"));

	pureRotationSpace = _node->get_bool_attribute_or_from_child_presence(TXT("pureRotationSpace"), pureRotationSpace);
	result &= rotationSpeedVar.load_from_xml(_node, TXT("rotationSpeedVarID"));
	result &= rotationSpeed.load_from_xml(_node, TXT("rotationSpeed"));
	
	result &= outputAtTargetVar.load_from_xml(_node, TXT("outputAtTargetVarID"));

	rotateYaw = _node->get_bool_attribute_or_from_child_presence(TXT("rotateYaw"), rotateYaw);
	rotatePitch = _node->get_bool_attribute_or_from_child_presence(TXT("rotatePitch"), rotatePitch);
	
	lookAtDoorIfRelativeToPresencePathHasNoTarget = _node->get_bool_attribute_or_from_child_presence(TXT("lookAtDoorIfRelativeToPresencePathHasNoTarget"), lookAtDoorIfRelativeToPresencePathHasNoTarget);

	result &= rotationSound.load_from_xml(_node, TXT("rotationSound"));
	result &= rotationThreshold.load_from_xml(_node, TXT("rotationThreshold"));
		
	return result;
}

bool RotateTowardsData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* RotateTowardsData::create_copy() const
{
	RotateTowardsData* copy = new RotateTowardsData();
	*copy = *this;
	return copy;
}

AppearanceController* RotateTowardsData::create_controller()
{
	return new RotateTowards(this);
}

void RotateTowardsData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
	pointBone.if_value_set([this, _rename](){ pointBone = _rename(pointBone.get(), NP); });
	targetBone.if_value_set([this, _rename](){ targetBone = _rename(targetBone.get(), NP); });
	targetSocketVar.if_value_set([this, _rename]() { targetSocketVar = _rename(targetSocketVar.get(), NP); });
	targetPointMSVar.if_value_set([this, _rename](){ targetPointMSVar = _rename(targetPointMSVar.get(), NP); });
	targetPointMSPtrVar.if_value_set([this, _rename](){ targetPointMSPtrVar = _rename(targetPointMSPtrVar.get(), NP); });
	targetPlacementMSVar.if_value_set([this, _rename](){ targetPlacementMSVar = _rename(targetPlacementMSVar.get(), NP); });
}

void RotateTowardsData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);

	defaultTargetPointMS.if_value_set([this, _transform]() { defaultTargetPointMS.access() = _transform.location_to_world(defaultTargetPointMS.get()); });
	defaultTargetDirMS.if_value_set([this, _transform]() { defaultTargetDirMS.access() = _transform.location_to_world(defaultTargetDirMS.get()); });
	viewOffsetBS.if_value_set([this, _transform]() { viewOffsetBS.access() = _transform.location_to_world(viewOffsetBS.get()); });
	rotationAxisBS.if_value_set([this, _transform]() { rotationAxisBS.access() = _transform.vector_to_world(rotationAxisBS.get()); });
	forwardDirBS.if_value_set([this, _transform]() { forwardDirBS.access() = _transform.vector_to_world(forwardDirBS.get()); });

	// this should get mirrors
	Vector3 resZAxis = Vector3::cross(_transform.get_x_axis(), _transform.get_y_axis());
	if (Vector3::dot(resZAxis, _transform.get_z_axis()) < 0.0f)
	{
		range.if_value_set([this]() { range.access() = -range.get(); });
	}
}

void RotateTowardsData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	pointBone.fill_value_with(_context);
	pointBonePlaneNormalBS.fill_value_with(_context);
	alignYawUsingTargetPlacementsPlaneNormalLS.fill_value_with(_context);
	defaultTargetPointMS.fill_value_with(_context);
	defaultTargetDirMS.fill_value_with(_context);
	viewOffsetBS.fill_value_with(_context);
	targetBone.fill_value_with(_context);
	targetSocketVar.fill_value_with(_context);
	targetPointMSVar.fill_value_with(_context);
	targetPointMSPtrVar.fill_value_with(_context);
	targetPlacementMSVar.fill_value_with(_context);
	rotationAxisBS.fill_value_with(_context);
	forwardDirBS.fill_value_with(_context);
	range.fill_value_with(_context);
	forwardOrBackward.fill_value_with(_context);
	rotationSound.fill_value_with(_context);
	rotationThreshold.fill_value_with(_context);
}

