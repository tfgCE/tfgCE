#include "ac_a_tentacle.h"

#include "..\..\..\framework\appearance\appearanceControllerPoseContext.h"
#include "..\..\..\framework\appearance\appearanceControllers.h"
#include "..\..\..\framework\appearance\skeleton.h"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(a_tentacle);

// variables
DEFINE_STATIC_NAME(tentacleLength);
DEFINE_STATIC_NAME(tentacleState);
DEFINE_STATIC_NAME(tentacleStateIdx);
DEFINE_STATIC_NAME(tentacleStateLength);

// tentacle states
DEFINE_STATIC_NAME(readyToUnfold);
DEFINE_STATIC_NAME(unfold); // requires tentacleStateLength
DEFINE_STATIC_NAME(readyToBurst);
DEFINE_STATIC_NAME(burst); // requires tentacleStateLength
DEFINE_STATIC_NAME(angry); // requires tentacleStateLength
DEFINE_STATIC_NAME(idle);

//

REGISTER_FOR_FAST_CAST(A_Tentacle);

A_Tentacle::A_Tentacle(A_TentacleData const * _data)
: base(_data)
, a_tentacleData(_data)
{
	rg.set_seed(this);

	normal.toRelaxedBlendTime = a_tentacleData->normal.toRelaxedBlendTime.get();
	normal.toTightBlendTime = a_tentacleData->normal.toTightBlendTime.get();
	normal.maxAngleRelaxed = a_tentacleData->normal.maxAngleRelaxed.get();
	normal.fullAngleTight = a_tentacleData->normal.fullAngleTight.get();
	normal.changeTime = a_tentacleData->normal.changeTime.get();
	normal.tightenTime = a_tentacleData->normal.tightenTime.get();
	
	angryJiggle.jiggleTime = a_tentacleData->angryJiggle.jiggleTime.get();

	angry.waveLength = a_tentacleData->angry.waveLength.get();
	angry.waveSpeed = a_tentacleData->angry.waveSpeed.get();
	angry.waveChangeTime = a_tentacleData->angry.waveChangeTime.get();
	angry.angleAlong = a_tentacleData->angry.angleAlong.get();
	angry.angleCoef = a_tentacleData->angry.angleCoef.get();
	angry.maxAngle = a_tentacleData->angry.maxAngle.get();

	applyAngleRotator = a_tentacleData->applyAngleRotator.get();
	changeAngleRotatorTime = a_tentacleData->changeAngleRotatorTime.get();

	changeAllTime = a_tentacleData->changeAllTime.get();
	
	changePitchTime = a_tentacleData->changePitchTime.get();
	pitchRange = a_tentacleData->pitchRange.get();
	
	changeMainRollTime = a_tentacleData->changeMainRollTime.get();
	changeMainRollRange = a_tentacleData->changeMainRollRange.get();

	angryJiggleChance = a_tentacleData->angryJiggleChance.get();
	angryJigglePower = a_tentacleData->angryJigglePower.get();
	angryChance = a_tentacleData->angryChance.get();
}

A_Tentacle::~A_Tentacle()
{
}

void A_Tentacle::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto& variables = get_owner()->get_owner()->access_variables();
	// hardcoded names, check scenery type and mesh generator
	tentacleLength.set_name(NAME(tentacleLength));
	tentacleLength.look_up<float>(variables);
	// hardcoded names, check a_tentacle ai logic
	tentacleState.set_name(NAME(tentacleState));
	tentacleState.look_up<Name>(variables);
	tentacleStateIdx.set_name(NAME(tentacleStateIdx));
	tentacleStateIdx.look_up<int>(variables);
	tentacleStateLength.set_name(NAME(tentacleStateLength));
	tentacleStateLength.look_up<float>(variables);

	if (auto* skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		bones.clear();
		Meshes::BoneID startBone = a_tentacleData->startBone.get();
		Meshes::BoneID bone = a_tentacleData->endBone.get();
		startBone.look_up(skeleton);
		bone.look_up(skeleton);
		parentBone = Meshes::BoneID(skeleton, skeleton->get_parent_of(startBone.get_index()));
		while (bone.is_valid())
		{
			BoneInfo bi;
			bi.bone = bone;
			bi.defaultPlacementMS = skeleton->get_bones_default_placement_MS()[bone.get_index()];
			bi.placementMS = bi.defaultPlacementMS;
			bi.angryJiggle.requestedLocationMS = bi.defaultPlacementMS.get_translation();
			bi.angryJiggle.requestedPlacementMS = bi.defaultPlacementMS;
			bones.push_front(bi);
			if (bone.get_name() == startBone.get_name())
			{
				break;
			}
			int parentIndex = skeleton->get_parent_of(bone.get_index());
			if (parentIndex != NONE)
			{
				bone = Meshes::BoneID(skeleton, parentIndex);
			}
			else
			{
				break;
			}
		}
		float angleStickOffsetZero = (float)bones.get_size() * a_tentacleData->normal.angleStickOffsetZeroPt.get();
		float angleStickZeroRange = (float)bones.get_size() * a_tentacleData->normal.angleStickZeroRangePt.get();
		float waveLengthCoef = a_tentacleData->normal.waveLength.get() / (float)bones.get_size();

		// propagate length
		for(int i = bones.get_size() - 1; i >= 0; --i)
		{
			if (i > 0)
			{
				bones[i].length = (bones[i].defaultPlacementMS.get_translation() - bones[i - 1].defaultPlacementMS.get_translation()).length();
			}
			else
			{
				bones[i].length = 0.0f;
			}
			bones[i].pt = (float)i / (float)max(1, bones.get_size() - 1);
			bones[i].normal.waveOffset = (float)i * waveLengthCoef;
			bones[i].normal.angleStickOffset = clamp(((float)i - angleStickOffsetZero) / angleStickZeroRange, -1.0f, 1.0f);
		}
	}

	angleRotator.on_reached_time = [this]()
	{
		angleRotator.timeLeft = rg.get(changeAngleRotatorTime);
		float pitch = rg.get_float(0.0f, 1.0f);
		angleRotator.target.pitch = applyAngleRotator.pitch * pitch * (rg.get_chance(0.5f)? 1.0f: -1.0f);
		angleRotator.target.yaw = applyAngleRotator.yaw * (1.0f - pitch);
		angleRotator.target.roll = 0.0f;
		angleRotator.target = angleRotator.target.normal();
		angleRotator.target.pitch *= applyAngleRotator.pitch;
		angleRotator.target.yaw *= applyAngleRotator.yaw;
		angleRotator.target.roll = rg.get_float(0.0f, 1.0f) * applyAngleRotator.roll * (rg.get_chance(0.5f) ? 1.0f : -1.0f);
	};
	angleRotator.init();

	angry.useWaveLength.init(rg.get(angry.waveLength));
	angry.useWaveSpeed.init(rg.get(angry.waveSpeed));
	angry.useAngleCoef.init(rg.get(angry.angleCoef));

	normal.useRoll.init(0.0f);
}

void A_Tentacle::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void A_Tentacle::calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(a_tentacle_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!get_active())
	{
		return;
	}

	auto* imo = get_owner()->get_owner();
	auto* presence = imo->get_presence();

	debug_filter(ac_a_tentacle);

	debug_context(presence->get_in_room());

	Transform placementWS = presence->get_placement();

	Meshes::Pose * poseLS = _context.access_pose_LS();

	float deltaTime = _context.get_delta_time();

	{
		float tl = tentacleLength.get<float>();
		if (tl != 0.0f)
		{
			float scale = tl / 10.0f;
			scale = sqrt(max(0.01f, scale));
			float timeScale = clamp(1.0f / scale, 0.25f, 1.8f);
			if (timeScale > 1.0f)
			{
				timeScale = 1.0f - (timeScale - 1.0f) * 0.3f;
			}
			deltaTime *= timeScale;
		}
	}

	if (tentacleStateIdx.get<int>() != stateIdx)
	{
		stateIdx = tentacleStateIdx.get<int>();
		state = tentacleState.get<Name>();
		float stateLength = tentacleStateLength.get<float>();

		if (state == NAME(angry))
		{
			stateTimeLeft = stateLength;
		}

		if (state == NAME(readyToUnfold))
		{
			folded.fullyFoldedPt = 0.0f;
			folded.fullyUnfoldedPt = -0.3f;
			folded.foldPTSpeed = 0.0f;
		}
		if (state == NAME(unfold))
		{
			folded.foldPTSpeed = 1.0f / max(0.01f, stateLength);
			stateTimeLeft = stateLength;
		}

		if (state == NAME(readyToBurst))
		{
			burst.pt = 0.0f;
			burst.targetPt = 0.0f;
		}
		if (state == NAME(burst))
		{
			burst.targetPt = 1.0f;
			burst.blendTime = stateLength;
			stateTimeLeft = stateLength;
		}
	}

	if (stateTimeLeft.is_set())
	{
		stateTimeLeft = stateTimeLeft.get() - deltaTime;
		if (stateTimeLeft.get() <= 0.0f)
		{
			stateTimeLeft.clear();
			if (state == NAME(angry) ||
				state == NAME(unfold) ||
				state == NAME(burst))
			{
				state = NAME(idle);
			}
		}
	}


	timeToChangeAll -= deltaTime;
	if (timeToChangeAll <= 0.0f)
	{
		timeToChangeAll = rg.get(changeAllTime);
		targetWeights.useAngry = rg.get_chance(angryChance) ? 1.0f : 0.0f;
		targetWeights.useAngry = max(targetWeights.useAngry, state == NAME(angry)? 1.0f : 0.0f);
		targetWeights.useAngryJiggle = rg.get_chance(angryJiggleChance) ? angryJigglePower : 0.0f;
		targetWeights.useNormalWithPrev = rg.get_chance(0.4f) ? 1.0f : 0.0f;
		normal.useRoll.set_target(rg.get_chance(normal.rollChance) ? 1.0f : 0.0f, timeToChangeAll);
	}
	currWeights.useAngry = blend_to_using_time(currWeights.useAngry, targetWeights.useAngry, 0.3f, deltaTime);
	currWeights.useAngryJiggle = blend_to_using_time(currWeights.useAngryJiggle, targetWeights.useAngryJiggle, 0.3f, deltaTime);
	currWeights.useNormalWithPrev = blend_to_using_time(currWeights.useNormalWithPrev, targetWeights.useNormalWithPrev, 0.3f, deltaTime);
	normal.useRoll.advance(deltaTime);
	if (burst.pt < 0.95f)
	{
		currWeights.useAngry = 1.0f;
	}
	if (burst.pt < 0.6f)
	{
		currWeights.useAngryJiggle = 1.0f;
	}
	currWeights.useAngryJiggle = max(currWeights.useAngryJiggle, currWeights.useAngry * 0.3f);

	changePitchTimeLeft -= deltaTime;
	if (changePitchTimeLeft <= 0.0f)
	{
		changePitchTimeLeft = rg.get(changePitchTime);
		targetPitch = rg.get(pitchRange);
	}
	currPitch = blend_to_using_time(currPitch, targetPitch, 2.0f, deltaTime);

	changeMainRollTimeLeft -= deltaTime;
	if (changeMainRollTimeLeft <= 0.0f)
	{
		changeMainRollTimeLeft = rg.get(changeMainRollTime);
		targetMainRoll += rg.get(changeMainRollRange);
	}
	currMainRoll = blend_to_using_time(currMainRoll, targetMainRoll, 0.8f, deltaTime);
	while (currMainRoll > 180.0f)
	{
		currMainRoll -= 360.0f;
		targetMainRoll -= 360.0f;
	}
	while (currMainRoll < -180.0f)
	{
		currMainRoll += 360.0f;
		targetMainRoll += 360.0f;
	}

	angleRotator.advance(deltaTime);

	Transform const tentancleParentMS = poseLS->get_bone_ms_from_ls(parentBone.get_index());

	// normal tentacle processing
	{
		normal.waveT += 60.0f * deltaTime;
		if (normal.timeToTight > 0.0f)
		{
			normal.timeToTight -= deltaTime;
			if (normal.timeToTight <= 0.0f)
			{
				normal.tightenAt = normal.tightenAt > 0.0f ? -1.0f : 1.0f;
				normal.angleStick = normal.tightenAt;
			}
		}
		normal.timeToChange -= deltaTime;
		if (normal.timeToChange < 0.0f)
		{
			normal.timeToChange = rg.get(normal.changeTime);
			normal.timeToTight = rg.get(normal.tightenTime);
			normal.angleStick.clear();
		}

		// always first do a pass of pure tightening
		BoneInfo* prev = nullptr;
		for_every(bone, bones)
		{
			{
				if (normal.angleStick.is_set())
				{
					bone->normal.angleStickNoPrev.power = blend_to_using_time(bone->normal.angleStickNoPrev.power, 1.0f, normal.toTightBlendTime * 0.5f, deltaTime);
					bone->normal.angleStickNoPrev.stick = blend_to_using_time(bone->normal.angleStickNoPrev.stick, normal.angleStick.get(), normal.toTightBlendTime * 0.5f, deltaTime);
				}
				else
				{
					bone->normal.angleStickNoPrev.power = blend_to_using_time(bone->normal.angleStickNoPrev.power, 0.0f, normal.toRelaxedBlendTime * 0.5f, deltaTime);
				}
			}
			if (!prev)
			{
				bone->normal.angleStickWithPrev = bone->normal.angleStickNoPrev;
			}
			else
			{
				bone->normal.angleStickWithPrev.power = blend_to_using_time(bone->normal.angleStickWithPrev.power, prev->normal.angleStickWithPrev.power > 0.95f? 1.0f : 0.0f, prev->normal.angleStickWithPrev.power > bone->normal.angleStickWithPrev.power ? normal.toTightBlendTime : normal.toRelaxedBlendTime, deltaTime);
				bone->normal.angleStickWithPrev.stick = blend_to_using_time(bone->normal.angleStickWithPrev.stick, prev->normal.angleStickWithPrev.stick, prev->normal.angleStickWithPrev.power > bone->normal.angleStickWithPrev.power ? normal.toTightBlendTime : normal.toRelaxedBlendTime, deltaTime);
			}
			bone->normal.angleStick.stick = lerp(currWeights.useNormalWithPrev, bone->normal.angleStickNoPrev.stick, bone->normal.angleStickWithPrev.stick);
			bone->normal.angleStick.power = lerp(currWeights.useNormalWithPrev, bone->normal.angleStickNoPrev.power, bone->normal.angleStickWithPrev.power);

			float waveAngle = sin_deg(normal.waveT + 360.0f * bone->normal.waveOffset);
			float angleStickAngle = bone->normal.angleStick.stick * bone->normal.angleStickOffset;
			float angle = bone->normal.angleStick.power * (normal.fullAngleTight / (float)(bones.get_size() - 1)) * angleStickAngle + (1.0f - bone->normal.angleStick.power) * waveAngle * normal.maxAngleRelaxed;
			angle *= clamp(bone->pt / 0.3f, 0.0f, 1.0f);

			bone->normal.angle = angle;

			int boneIdx = bone->bone.get_index();
			Transform boneLS = poseLS->get_bone(boneIdx);
			Rotator3 useAngleRotator = angleRotator.curr;
			useAngleRotator.yaw = 1.0f; 
			useAngleRotator.roll *= normal.useRoll.curr;
			Rotator3 useAngle = useAngleRotator * bone->normal.angle;
			useAngle.pitch += currPitch * (1.0f - bone->pt);
			boneLS.set_orientation(useAngle.to_quat());
			bone->normal.resultPlacementLS = boneLS;

			prev = bone;
		}
	}

	// angry jiggle
	{
		float prevTimeToJiggle = angryJiggle.timeToJiggle;
		angryJiggle.timeToJiggle = max(0.0f, angryJiggle.timeToJiggle - deltaTime);

		if (currWeights.useAngryJiggle > 0.0f)
		{
			float useNew = prevTimeToJiggle > 0.0f ? clamp(1.0f - angryJiggle.timeToJiggle / prevTimeToJiggle, 0.0f, 1.0f) : 1.0f;
			for (int i = 0; i < bones.get_size(); ++i)
			{
				BoneInfo* bone = &bones[i];
				BoneInfo* prev = i > 0 ? &bones[i - 1] : nullptr;
				if (!prev)
				{
					bone->angryJiggle.requestedPlacementMS = bone->defaultPlacementMS;
				}
				else
				{
					BoneInfo* next = i < bones.get_size() - 1 ? &bones[i + 1] : nullptr;
					Vector3 locMS = bone->angryJiggle.requestedLocationMS;
					Vector3 prevMS = prev ? prev->angryJiggle.requestedLocationMS : locMS;
					Vector3 nextMS = next ? next->angryJiggle.requestedLocationMS : locMS;
					Vector3 dirMS = (nextMS - prevMS).normal();
					bone->angryJiggle.requestedPlacementMS = Transform::lerp(useNew, bone->angryJiggle.requestedPlacementMS, look_matrix(bone->angryJiggle.requestedLocationMS, dirMS, bone->defaultPlacementMS.get_axis(Axis::Up)).to_transform());
				}
			}

			{
				Transform reqParentMS = tentancleParentMS;
				Transform storeParentMS = tentancleParentMS;
				for_every(bone, bones)
				{
					Transform reqBoneLS = reqParentMS.to_local(bone->angryJiggle.requestedPlacementMS);
					Transform poseBoneLS = poseLS->get_bone(bone->bone.get_index());
					poseBoneLS = reqBoneLS;
					bone->angryJiggle.resultPlacementLS = poseBoneLS;
					storeParentMS = storeParentMS.to_world(bone->angryJiggle.resultPlacementLS);
					reqParentMS = bone->angryJiggle.requestedPlacementMS;
				}
			}
		}

		if (angryJiggle.timeToJiggle <= 0.0f)
		{
			angryJiggle.timeToJiggle = rg.get(angryJiggle.jiggleTime);
			BoneInfo* prev = nullptr;
			for_every(bone, bones)
			{
				if (prev)
				{
					Vector3 randOff(Vector3::zAxis * rg.get_float(0.0f, bone->length * 1.8f));
					randOff = Rotator3(0.0f, 0.0f, rg.get_float(-180.0f, 180.0f)).to_quat().to_world(randOff);
					bone->angryJiggle.requestedLocationMS = bone->defaultPlacementMS.get_translation() + bone->defaultPlacementMS.vector_to_world(randOff);
					float const usePrev = 0.8f;
					Vector3 prevInHere = bone->defaultPlacementMS.location_to_local(prev->angryJiggle.requestedLocationMS);
					prevInHere.y = 0.0f;
					prevInHere = bone->defaultPlacementMS.location_to_world(prevInHere);
					bone->angryJiggle.requestedLocationMS = lerp(usePrev, bone->angryJiggle.requestedLocationMS, prevInHere);
				}
				else
				{
					bone->angryJiggle.requestedLocationMS = bone->defaultPlacementMS.get_translation();
				}

				prev = bone;
			}
		}
	}

	// angry
	{
		angry.waveChangeTimeLeft -= deltaTime;
		if (angry.waveChangeTimeLeft <= 0.0f)
		{
			angry.waveChangeTimeLeft = rg.get(angry.waveChangeTime);
			angry.useWaveLength.set_target(rg.get(angry.waveLength), angry.waveChangeTimeLeft);
			angry.useWaveSpeed.set_target(rg.get(angry.waveSpeed), angry.waveChangeTimeLeft);
			angry.useAngleCoef.set_target(rg.get(angry.angleCoef), angry.waveChangeTimeLeft);
		}
		angry.useWaveLength.advance(deltaTime);
		angry.useWaveSpeed.advance(deltaTime);
		angry.useAngleCoef.advance(deltaTime);

		float extraAngleCoef = 1.0f;
		angry.wavePt = mod(angry.wavePt + deltaTime * angry.useWaveSpeed.curr / max(0.001f, angry.useWaveLength.curr), 1.0f);
		for (int i = 0; i < bones.get_size(); ++i)
		{
			BoneInfo* bone = &bones[i];

			float wavePt = angry.wavePt + bone->pt / max(0.001f, angry.useWaveLength.curr);

			float maxAngle = angry.angleAlong.get_at(bone->pt) * extraAngleCoef;
			maxAngle = min(maxAngle, angry.maxAngle);
			bone->angry.angle = sin_deg(360.0f * wavePt) * maxAngle;

			extraAngleCoef *= angry.useAngleCoef.curr;

			int boneIdx = bone->bone.get_index();
			Transform boneLS = poseLS->get_bone(boneIdx);
			Rotator3 useAngleRotator = angleRotator.curr;
			useAngleRotator.pitch *= 0.8f;
			useAngleRotator.yaw = 1.0f;
			useAngleRotator.roll *= 0.1f;
			boneLS.set_orientation((useAngleRotator * bone->angry.angle).to_quat());
			bone->angry.resultPlacementLS = boneLS;
		}
	}

	// folded
	{
		if ((folded.foldPTSpeed > 0.0f && folded.fullyUnfoldedPt < 1.0f) ||
			(folded.foldPTSpeed < 0.0f && folded.fullyFoldedPt > 0.0f))
		{
			folded.fullyFoldedPt += folded.foldPTSpeed * deltaTime;
			folded.fullyUnfoldedPt += folded.foldPTSpeed * deltaTime;
		}
	}

	// burst
	{
		burst.pt = clamp(blend_to_using_time(burst.pt, burst.targetPt, burst.blendTime, deltaTime), 0.0f, 1.0f);
	}

	// mix all and store current placement MS
	{
		Range fold(folded.fullyUnfoldedPt, folded.fullyFoldedPt);
		Transform parentMS = tentancleParentMS;
		for (int i = 0; i < bones.get_size(); ++i)
		{
			BoneInfo* bone = &bones[i];

			Transform thisBoneLS = poseLS->get_bone(bone->bone.get_index());
			thisBoneLS = bone->normal.resultPlacementLS;
			thisBoneLS = Transform::lerp(currWeights.useAngryJiggle, thisBoneLS, bone->angryJiggle.resultPlacementLS);
			thisBoneLS = Transform::lerp(currWeights.useAngry, thisBoneLS, bone->angry.resultPlacementLS);
			{
				float foldedBone = clamp(fold.get_pt_from_value(bone->pt), 0.0f, 1.0f);
				if (foldedBone > 0.0f)
				{
					Transform foldedBoneLS = thisBoneLS;
					foldedBoneLS.set_orientation(Rotator3(0.0f, folded.foldedAngle, 0.0f).to_quat());
					thisBoneLS = Transform::lerp(foldedBone, thisBoneLS, foldedBoneLS);
				}
			}
			{
				float boneBurstPt = 1.0f - bone->pt;
				float burstHideThis = 0.0f;
				if (burst.pt < boneBurstPt)
				{
					burstHideThis = 1.0f;
				}
				else
				{
					float prevBoneBurstPt = 1.0f - bones[max(0, i - 1)].pt;
					if (burst.pt < prevBoneBurstPt && boneBurstPt != prevBoneBurstPt)
					{
						// if burst.pt is at prev, we're fully exposed (not hidden), if it is at us, we're fully hidden
						burstHideThis = remap_value(burst.pt, prevBoneBurstPt, boneBurstPt, 0.0f, 1.0f);
					}
				}

				burstHideThis = clamp(burstHideThis, 0.0f, 1.0f);
				if (burstHideThis > 0.0f)
				{
					thisBoneLS = Transform::lerp(burstHideThis, thisBoneLS, Transform(Vector3(0.0f, i == 0? -0.01f : 0.0f, 0.0f), Quat::identity));
				}
			}
			poseLS->set_bone(bone->bone.get_index(), thisBoneLS);
			bone->placementMS = parentMS.to_world(thisBoneLS);
			parentMS = bone->placementMS;
		}
	}

	{
		Transform thisBoneLS = poseLS->get_bone(parentBone.get_index());
		thisBoneLS = Transform(Vector3::zero, Rotator3(0.0f, currMainRoll, 0.0f).to_quat()).to_world(thisBoneLS);
		poseLS->set_bone(parentBone.get_index(), thisBoneLS);
	}

	debug_no_context();

	debug_no_filter();
}

void A_Tentacle::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	for_every(bone, bones)
	{
		providesBones.push_back(bone->bone.get_index());
	}
}

//

REGISTER_FOR_FAST_CAST(A_TentacleData);

Framework::AppearanceControllerData* A_TentacleData::create_data()
{
	return new A_TentacleData();
}

void A_TentacleData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(a_tentacle), create_data);
}

A_TentacleData::A_TentacleData()
{
}

bool A_TentacleData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= startBone.load_from_xml(_node, TXT("startBone"));
	result &= endBone.load_from_xml(_node, TXT("endBone"));
	
	for_every(node, _node->children_named(TXT("normal")))
	{
		result &= normal.fullAngleTight.load_from_xml(node, TXT("fullAngleTight"));
		result &= normal.maxAngleRelaxed.load_from_xml(node, TXT("maxAngleRelaxed"));
		result &= normal.toRelaxedBlendTime.load_from_xml(node, TXT("toRelaxedBlendTime"));
		result &= normal.toTightBlendTime.load_from_xml(node, TXT("toTightBlendTime"));
	
		result &= normal.angleStickOffsetZeroPt.load_from_xml(node, TXT("angleStickOffsetZeroPt"));
		result &= normal.angleStickZeroRangePt.load_from_xml(node, TXT("angleStickZeroRangePt"));
		result &= normal.waveLength.load_from_xml(node, TXT("waveLength"));

		result &= normal.changeTime.load_from_xml(node, TXT("changeTime"));
		result &= normal.tightenTime.load_from_xml(node, TXT("tightenTime"));
		
		result &= normal.rollChance.load_from_xml(node, TXT("rollChance"));
	}

	for_every(node, _node->children_named(TXT("angryJiggle")))
	{
		result &= angryJiggle.jiggleTime.load_from_xml(node, TXT("jiggleTime"));
	}

	for_every(node, _node->children_named(TXT("angry")))
	{
		result &= angry.waveLength.load_from_xml(node, TXT("waveLength"));
		result &= angry.waveSpeed.load_from_xml(node, TXT("waveSpeed"));
		result &= angry.waveChangeTime.load_from_xml(node, TXT("waveChangeTime"));
		result &= angry.angleAlong.load_from_xml(node, TXT("angleAlong"));
		result &= angry.angleCoef.load_from_xml(node, TXT("angleCoef"));
		result &= angry.maxAngle.load_from_xml(node, TXT("maxAngle"));
	}

	result &= applyAngleRotator.load_from_xml(_node, TXT("applyAngleRotator"));
	result &= changeAngleRotatorTime.load_from_xml(_node, TXT("changeAngleRotatorTime"));

	result &= changeAllTime.load_from_xml(_node, TXT("changeAllTime"));

	result &= changePitchTime.load_from_xml(_node, TXT("changePitchTime"));
	result &= pitchRange.load_from_xml(_node, TXT("pitchRange"));

	result &= changeMainRollTime.load_from_xml(_node, TXT("changeMainRollTime"));
	result &= changeMainRollRange.load_from_xml(_node, TXT("changeMainRollRange"));

	result &= angryJiggleChance.load_from_xml(_node, TXT("angryJiggleChance"));
	result &= angryJigglePower.load_from_xml(_node, TXT("angryJigglePower"));
	result &= angryChance.load_from_xml(_node, TXT("angryChance"));

	return result;
}

bool A_TentacleData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::AppearanceControllerData* A_TentacleData::create_copy() const
{
	A_TentacleData* copy = new A_TentacleData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* A_TentacleData::create_controller()
{
	return new A_Tentacle(this);
}

void A_TentacleData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	startBone.if_value_set([this, _rename](){ startBone = _rename(startBone.get(), NP); });
	endBone.if_value_set([this, _rename]() { endBone = _rename(endBone.get(), NP); });
}

void A_TentacleData::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	startBone.fill_value_with(_context);
	endBone.fill_value_with(_context);

	normal.fullAngleTight.fill_value_with(_context);
	normal.maxAngleRelaxed.fill_value_with(_context);
	normal.toRelaxedBlendTime.fill_value_with(_context);
	normal.toTightBlendTime.fill_value_with(_context);
	
	normal.angleStickOffsetZeroPt.fill_value_with(_context);
	normal.angleStickZeroRangePt.fill_value_with(_context);
	normal.waveLength.fill_value_with(_context);

	normal.changeTime.fill_value_with(_context);
	normal.tightenTime.fill_value_with(_context);
	
	normal.rollChance.fill_value_with(_context);

	angryJiggle.jiggleTime.fill_value_with(_context);
	
	angry.waveLength.fill_value_with(_context);
	angry.waveSpeed.fill_value_with(_context);
	angry.waveChangeTime.fill_value_with(_context);
	angry.angleAlong.fill_value_with(_context);
	angry.angleCoef.fill_value_with(_context);
	angry.maxAngle.fill_value_with(_context);

	applyAngleRotator.fill_value_with(_context);
	changeAngleRotatorTime.fill_value_with(_context);

	changeAllTime.fill_value_with(_context);
	
	changePitchTime.fill_value_with(_context);
	pitchRange.fill_value_with(_context);

	changeMainRollTime.fill_value_with(_context);
	changeMainRollRange.fill_value_with(_context);

	angryJiggleChance.fill_value_with(_context);
	angryJigglePower.fill_value_with(_context);
	angryChance.fill_value_with(_context);
}

