#include "ac_readyHumanoidSpine.h"

#include "ikUtils.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"
#include "..\..\..\core\wheresMyPoint\iWMPTool.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\performance\performanceUtils.h"
#include "..\..\..\core\system\input.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(ReadyHumanoidSpine);

//

REGISTER_FOR_FAST_CAST(ReadyHumanoidSpine);

ReadyHumanoidSpine::ReadyHumanoidSpine(ReadyHumanoidSpineData const * _data)
: base(_data)
, readyHumanoidSpineData(_data)
{
	pelvisBone = readyHumanoidSpineData->pelvisBone.get();
	headBone = readyHumanoidSpineData->headBone.get();
	rightHandBone = readyHumanoidSpineData->rightHandBone.get();
	leftHandBone = readyHumanoidSpineData->leftHandBone.get();
	rightHandControlBone = readyHumanoidSpineData->rightHandControlBone.get();
	leftHandControlBone = readyHumanoidSpineData->leftHandControlBone.get();

	outPelvisMSVar = readyHumanoidSpineData->outPelvisMSVar.get();
	outTopSpineMSVar = readyHumanoidSpineData->outTopSpineMSVar.get();

	if (readyHumanoidSpineData->rightArmRequestedLengthXYPtVar.is_set()) { rightArmRequestedLengthXYPtVar = readyHumanoidSpineData->rightArmRequestedLengthXYPtVar.get(); }
	if (readyHumanoidSpineData->leftArmRequestedLengthXYPtVar.is_set()) { leftArmRequestedLengthXYPtVar = readyHumanoidSpineData->leftArmRequestedLengthXYPtVar.get(); }

	offsetHandRight = readyHumanoidSpineData->offsetHandRight.get();
	
	spineToHeadOffset = readyHumanoidSpineData->spineToHeadOffset.get(Transform::identity);
	spineToHeadOffsetVar = readyHumanoidSpineData->spineToHeadOffsetVar.get();

	useDirFromHead = readyHumanoidSpineData->useDirFromHead.get();
	useDirFromPelvis = readyHumanoidSpineData->useDirFromPelvis.get();
	useDirFromHands = readyHumanoidSpineData->useDirFromHands.get();
	minRequestedLengthXYPtToUseDirFromHands = readyHumanoidSpineData->minRequestedLengthXYPtToUseDirFromHands.get();
	maxRequestedLengthXYPtToUseDirFromPelvis = readyHumanoidSpineData->maxRequestedLengthXYPtToUseDirFromPelvis.get();
}

ReadyHumanoidSpine::~ReadyHumanoidSpine()
{
}

void ReadyHumanoidSpine::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	outPelvisMSVar.look_up<Transform>(varStorage);
	outTopSpineMSVar.look_up<Transform>(varStorage);
	spineToHeadOffsetVar.look_up<Transform>(varStorage);
	rightArmRequestedLengthXYPtVar.look_up<float>(varStorage);
	leftArmRequestedLengthXYPtVar.look_up<float>(varStorage);

	if (Meshes::Skeleton const * skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		pelvisBone.look_up(skeleton);
		headBone.look_up(skeleton);
		rightHandBone.look_up(skeleton);
		leftHandBone.look_up(skeleton);
		rightHandControlBone.look_up(skeleton);
		leftHandControlBone.look_up(skeleton);

		for_count(int, i, 2)
		{
			auto& handBone = i == 0 ? rightHandBone : leftHandBone;
			auto& armBone = i == 0 ? rightArmBone : leftArmBone;
			int forearm = skeleton->get_parent_of(handBone.get_index());
			int arm = skeleton->get_parent_of(forearm);
			armBone = Meshes::BoneID(skeleton, arm);
		}
	}
}

void ReadyHumanoidSpine::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void ReadyHumanoidSpine::calculate_final_pose(REF_ AppearanceControllerPoseContext& _context)
{
	base::calculate_final_pose(_context);

	auto const* mesh = get_owner()->get_mesh();
	if (!mesh)
	{
		return;
	}

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(readyHumanoidSpine_finalPose);
#endif

	debug_filter(ac_readyHumanoidSpine);
	debug_context(this->get_owner()->get_owner()->get_presence()->get_in_room());
	debug_push_transform(this->get_owner()->get_owner()->get_presence()->get_placement());

	Meshes::Pose* poseLS = _context.access_pose_LS();

	Transform pelvisDefaultMS = get_owner()->get_skeleton()->get_skeleton()->get_bones_default_placement_MS()[pelvisBone.get_index()];
	Transform headDefaultMS = get_owner()->get_skeleton()->get_skeleton()->get_bones_default_placement_MS()[headBone.get_index()];
	Transform pelvisMS = poseLS->get_bone_ms_from_ls(pelvisBone.get_index());
	Transform headMS = poseLS->get_bone_ms_from_ls(headBone.get_index());
	Transform rightHandControlMS = poseLS->get_bone_ms_from_ls(rightHandControlBone.get_index());
	Transform leftHandControlMS = poseLS->get_bone_ms_from_ls(leftHandControlBone.get_index());

	debug_draw_transform_size_coloured(true, headMS, 0.1f, Colour::grey, Colour::grey, Colour::grey);

	Transform flatHeadMS = headMS;
	{
		float offXYPlane = flatHeadMS.get_axis(Axis::Y).z;
		Quat orientation = flatHeadMS.get_orientation();
		orientation = orientation.rotate_by(Rotator3(-offXYPlane * 70.0f, 0.0f, 0.0f).to_quat()); // should be enough to bring us to proper front
		Rotator3 rotation = orientation.to_rotator();
		rotation.pitch = 0.0f;
		rotation.roll = 0.0f;
		orientation = rotation.to_quat();
		flatHeadMS.set_orientation(orientation);
	}

	Vector3 rightHandRel = flatHeadMS.location_to_local(rightHandControlMS.get_translation());
	Vector3 leftHandRel = flatHeadMS.location_to_local(leftHandControlMS.get_translation());
	float rightHandRelYaw = Rotator3::get_yaw(rightHandRel);
	float leftHandRelYaw = Rotator3::get_yaw(leftHandRel);

	{
		Vector3 headRightMS = headMS.get_axis(Axis::X);
		Vector3 pelvisRightMS = pelvisMS.get_axis(Axis::X);
		Vector3 rightFromArms = ((rightHandControlMS.get_translation() + offsetHandRight) - (leftHandControlMS.get_translation() - offsetHandRight)).normal();

		debug_draw_arrow(true, Colour::white, flatHeadMS.location_to_world(Vector3::zero), flatHeadMS.location_to_world(Vector3::yAxis * 0.2f));
		debug_draw_arrow(true, Colour::red, flatHeadMS.get_translation(), flatHeadMS.get_translation() + rightFromArms * 0.2f);

		float useHands = 0.0f;
		useHands += clamp((rightHandRelYaw - leftHandRelYaw) / 10.0f, 0.0f, 1.0f); // hands on other sides (or both in front, then this will be 0)

		{
			float targetActualUseDirFromHands = 1.0f;
			float targetActualUseDirFromPelvis = 1.0f;
			float targetActualUseDirFromHead = 1.0f;
			if (rightArmRequestedLengthXYPtVar.is_valid() &&
				leftArmRequestedLengthXYPtVar.is_valid())
			{
				float maxReqLXYPt = max(rightArmRequestedLengthXYPtVar.get<float>(), leftArmRequestedLengthXYPtVar.get<float>());
				float minReqLXYPtToUseHands = minRequestedLengthXYPtToUseDirFromHands;
				float maxReqLXYPtToUsePelvis = maxRequestedLengthXYPtToUseDirFromPelvis;
				
				float useHands = minReqLXYPtToUseHands < 1.0f? clamp(minReqLXYPtToUseHands + (maxReqLXYPt - minReqLXYPtToUseHands) / (1.0f - minReqLXYPtToUseHands), 0.0f, 1.0f) : 0.0f;
				targetActualUseDirFromHands *= useHands;

				float usePelvis = maxReqLXYPtToUsePelvis > 0.0f ? clamp(maxReqLXYPt / maxReqLXYPtToUsePelvis, 0.0f, 1.0f) : 0.0f;
				targetActualUseDirFromPelvis = usePelvis;
				targetActualUseDirFromHead = (1.0f - 0.8f * usePelvis);
			}
			actualUseDirFromHead = blend_to_using_time(actualUseDirFromHands, targetActualUseDirFromHands, 0.2f, _context.get_delta_time());
			actualUseDirFromHands = blend_to_using_time(actualUseDirFromHands, targetActualUseDirFromHands, 0.2f, _context.get_delta_time());
			actualUseDirFromPelvis = blend_to_using_time(actualUseDirFromPelvis, targetActualUseDirFromPelvis, 0.2f, _context.get_delta_time());
		}

		headRightMS = headRightMS * (actualUseDirFromHead * useDirFromHead)
					+ rightFromArms * (actualUseDirFromHands * useDirFromHands * useHands)
					+ pelvisRightMS * (actualUseDirFromPelvis * useDirFromPelvis);

		if (headRightMS.is_zero())
		{
			headRightMS = headMS.get_axis(Axis::X);
		}

		headRightMS.z *= 0.2f;

		headMS = matrix_from_up_right(headMS.get_translation(), (headMS.get_axis(Axis::Z) * 0.5f + Vector3::zAxis * 0.4f).normal(), headRightMS.normal()).to_transform();
		debug_draw_transform_size(true, headMS, 0.2f);
	}

	Vector3 const headMSy = headMS.get_axis(Axis::Y);
	float const headZPt = headMS.get_translation().z / headDefaultMS.get_translation().z;

	// pelvis
	Transform properPelvisMS = pelvisMS;
	{
		Vector3 pelvisLoc = properPelvisMS.get_translation();
		if (readyHumanoidSpineData->pelvisLocFromHeadLocZPt.is_valid())
		{
			Vector3 pelvisLocFromHeadLocZPt = readyHumanoidSpineData->pelvisLocFromHeadLocZPt.remap(headZPt);
			pelvisLoc.z *= pelvisLocFromHeadLocZPt.z;
			pelvisLoc += properPelvisMS.get_axis(Axis::Y) * pelvisLocFromHeadLocZPt.y;
		}
		else
		{
			float keepPelvisZ = clamp(headZPt, 0.0f, 1.0f);
			pelvisLoc.z *= 0.3f * sqr(keepPelvisZ) + 0.7f * keepPelvisZ;
		}
		if (readyHumanoidSpineData->pelvisLocOffsetFromHeadAxisYZ.is_valid())
		{
			Vector3	pelvisLocOffsetFromHeadAxisYZ = readyHumanoidSpineData->pelvisLocOffsetFromHeadAxisYZ.remap(headMSy.z);
			pelvisLoc += pelvisLocOffsetFromHeadAxisYZ;
		}
		else
		{
			float moveY = -0.2f * clamp(1.0f - headZPt, 0.0f, 1.0f);
			{
				moveY += (-0.15f + moveY) * clamp(sqrt(abs(headMSy.z)), 0.0f, 0.8f) * sign(headMSy.z);
			}
			pelvisLoc += properPelvisMS.get_axis(Axis::Y) * moveY;
			debug_draw_text(true, Colour::blue, headMS.location_to_world(Vector3(0.0f, 1.0f, 0.0)), Vector2::half, true, 0.5f, NP, TXT("%.3f <- %.3f"), moveY, abs(headMS.get_axis(Axis::Y).z));
		}
		properPelvisMS.set_translation(pelvisLoc);

		// rotate pelvis when trying to grab pocket with opposite hand
		Vector3 rightHandRel = properPelvisMS.location_to_local(rightHandControlMS.get_translation());
		Vector3 leftHandRel = properPelvisMS.location_to_local(leftHandControlMS.get_translation());

		float const minDistToPelvis = 0.25f;
		float const maxDistToPelvis = 0.5f;
		Range const distPelvisRange(minDistToPelvis, maxDistToPelvis);
		Range const toShouldRotate(1.0f, 0.0f);
		//float const leftHandDist = leftHandRel.length();
		//float const rightHandDist = rightHandRel.length();
		float const shouldRotateLeft = Range::transform_clamp(leftHandRel.length(), distPelvisRange, toShouldRotate);
		float const shouldRotateRight = Range::transform_clamp(rightHandRel.length(), distPelvisRange, toShouldRotate);
		if (shouldRotateLeft || shouldRotateRight)
		{
			float const rotateAngle = 30.0f;
			float const atDist = 0.10f;
			float const atOffset = 0.08f;
			float const rotateRight = rotateAngle * shouldRotateRight * clamp((-rightHandRel.x + atOffset) / atDist, 0.0f, 1.0f);
			float const rotateLeft = -rotateAngle * shouldRotateLeft * clamp((leftHandRel.x + atOffset) / atDist, 0.0f, 1.0f);
			float const rotateYaw = rotateRight + rotateLeft;

			//debug_draw_text(true, Colour::red, rightHandControlMS.get_translation(), Vector2::half, true, 0.5f, NP, TXT("d:%.1f sr:%.1f ry:%.1f"), rightHandDist, shouldRotateRight, rotateYaw);
			debug_draw_text(true, Colour::red, rightHandControlMS.get_translation(), Vector2::half, true, 0.5f, NP, TXT("(%.2f -> %.1f)->(%.1f)<-(%.1f <- %.2f)"), leftHandRel.x, rotateLeft, rotateYaw, rotateRight, rightHandRel.x);

			if (rotateYaw != 0.0f)
			{
				Quat o = properPelvisMS.get_orientation();
				o = o.rotate_by(Rotator3(0.0f, rotateYaw, 0.0f).to_quat());
				properPelvisMS.set_orientation(o);
			}
		}

		if (readyHumanoidSpineData->pelvisRotFromHeadLocZPt.is_valid())
		{
			Rotator3 pelvisRotFromHeadLocZPt = readyHumanoidSpineData->pelvisRotFromHeadLocZPt.remap(headZPt);
			Quat ori = properPelvisMS.get_orientation();
			ori = ori.to_world(pelvisRotFromHeadLocZPt.to_quat());
			properPelvisMS.set_orientation(ori);
		}

		debug_draw_arrow(true, Colour::green, pelvisMS.get_translation(), properPelvisMS.get_translation());
	}

	outPelvisMSVar.access<Transform>() = properPelvisMS;

	// spine (top)
	Transform properSpineMS = headMS;
	{
		Transform useSpineToHeadOffset = spineToHeadOffset;
		if (spineToHeadOffsetVar.is_valid())
		{
			useSpineToHeadOffset = spineToHeadOffsetVar.get<Transform>();
		}
		properSpineMS.set_orientation(headDefaultMS.get_orientation());
		properSpineMS = properSpineMS.to_world(useSpineToHeadOffset);
		float useSpineToHeadOffsetLength = useSpineToHeadOffset.get_translation().length();
		bool usedOffsets = false;
		if (readyHumanoidSpineData->spineLocOffsetFromHeadLocZPt.is_valid())
		{
			Vector3 spineLocOffsetFromHeadLocZPt = readyHumanoidSpineData->spineLocOffsetFromHeadLocZPt.remap(headZPt);
			Vector3 newLocMS = properSpineMS.get_translation() + spineLocOffsetFromHeadLocZPt;
			properSpineMS.set_translation(headMS.get_translation() + (newLocMS - headMS.get_translation()).normal() * useSpineToHeadOffsetLength);
			usedOffsets = true;
		}
		if (readyHumanoidSpineData->spineLocOffsetFromHeadAxisYZ.is_valid())
		{
			Vector3 spineLocOffsetFromHeadAxisYZ = readyHumanoidSpineData->spineLocOffsetFromHeadAxisYZ.remap(headMSy.z);
			Vector3 newLocMS = properSpineMS.get_translation() + spineLocOffsetFromHeadAxisYZ;
			properSpineMS.set_translation(headMS.get_translation() + (newLocMS - headMS.get_translation()).normal() * useSpineToHeadOffsetLength);
			usedOffsets = true;
		}
		{
			Transform currentProperSpineMS = properSpineMS;
			properSpineMS = headMS.to_world(useSpineToHeadOffset);

			Vector3 uprightLoc = lerp(0.3f, properPelvisMS.get_translation(), properSpineMS.get_translation());
			uprightLoc.z = properSpineMS.get_translation().z;

			float spineHeadDist = (headMS.get_translation() - properSpineMS.get_translation()).length();
			Vector3 pelvisLineAlignedLoc = headMS.get_translation() + (properPelvisMS.get_translation() - headMS.get_translation()).normal() * spineHeadDist;

			Vector3 spineMS = lerp(0.1f, properSpineMS.get_translation(),
							  lerp(0.4f, uprightLoc, pelvisLineAlignedLoc));
			if (usedOffsets)
			{
				// keep x to get the roll
				spineMS.y = currentProperSpineMS.get_translation().y;
				spineMS.z = currentProperSpineMS.get_translation().z;
				spineMS = headMS.get_translation() + (spineMS - headMS.get_translation()).normal() * useSpineToHeadOffsetLength;
			}
			properSpineMS.set_translation(spineMS);
		}
	}

	outTopSpineMSVar.access<Transform>() = properSpineMS;

	debug_draw_transform_size(true, properSpineMS, 0.2f);

	debug_pop_transform();
	debug_no_context();
	debug_no_filter();
}

void ReadyHumanoidSpine::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);
}

//

REGISTER_FOR_FAST_CAST(ReadyHumanoidSpineData);

AppearanceControllerData* ReadyHumanoidSpineData::create_data()
{
	return new ReadyHumanoidSpineData();
}

void ReadyHumanoidSpineData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(ReadyHumanoidSpine), create_data);
}

ReadyHumanoidSpineData::ReadyHumanoidSpineData()
{
}

bool ReadyHumanoidSpineData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= pelvisBone.load_from_xml(_node, TXT("pelvisBone"));
	result &= headBone.load_from_xml(_node, TXT("headBone"));
	result &= rightHandBone.load_from_xml(_node, TXT("rightHandBone"));
	result &= leftHandBone.load_from_xml(_node, TXT("leftHandBone"));
	result &= rightHandControlBone.load_from_xml(_node, TXT("rightHandControlBone"));
	result &= leftHandControlBone.load_from_xml(_node, TXT("leftHandControlBone"));
	result &= outPelvisMSVar.load_from_xml(_node, TXT("outPelvisMSVarID"));
	result &= outTopSpineMSVar.load_from_xml(_node, TXT("outTopSpineMSVarID"));
	result &= rightArmRequestedLengthXYPtVar.load_from_xml(_node, TXT("rightArmRequestedLengthXYPtVarID"));
	result &= leftArmRequestedLengthXYPtVar.load_from_xml(_node, TXT("leftArmRequestedLengthXYPtVarID"));
	result &= offsetHandRight.load_from_xml(_node, TXT("offsetHandRight"));
	result &= spineToHeadOffset.load_from_xml(_node, TXT("spineToHeadOffset"));
	result &= spineToHeadOffsetVar.load_from_xml(_node, TXT("spineToHeadOffsetVarID"));
	result &= useDirFromHead.load_from_xml(_node, TXT("useDirFromHead"));
	result &= useDirFromPelvis.load_from_xml(_node, TXT("useDirFromPelvis"));
	result &= useDirFromHands.load_from_xml(_node, TXT("useDirFromHands"));
	result &= minRequestedLengthXYPtToUseDirFromHands.load_from_xml(_node, TXT("minRequestedLengthXYPtToUseDirFromHands"));
	result &= maxRequestedLengthXYPtToUseDirFromPelvis.load_from_xml(_node, TXT("maxRequestedLengthXYPtToUseDirFromPelvis"));

	pelvisLocFromHeadLocZPt.clear();
	for_every(node, _node->children_named(TXT("pelvisLocFromHeadLocZPt")))
	{
		result &= pelvisLocFromHeadLocZPt.load_from_xml(node);
	}
	//
	pelvisLocOffsetFromHeadAxisYZ.clear();
	for_every(node, _node->children_named(TXT("pelvisLocOffsetFromHeadAxisYZ")))
	{
		result &= pelvisLocOffsetFromHeadAxisYZ.load_from_xml(node);
	}
	//
	pelvisRotFromHeadLocZPt.clear();
	for_every(node, _node->children_named(TXT("pelvisRotFromHeadLocZPt")))
	{
		result &= pelvisRotFromHeadLocZPt.load_from_xml(node);
	}

	spineLocOffsetFromHeadLocZPt.clear();
	for_every(node, _node->children_named(TXT("spineLocOffsetFromHeadLocZPt")))
	{
		result &= spineLocOffsetFromHeadLocZPt.load_from_xml(node);
	}
	//
	spineLocOffsetFromHeadAxisYZ.clear();
	for_every(node, _node->children_named(TXT("spineLocOffsetFromHeadAxisYZ")))
	{
		result &= spineLocOffsetFromHeadAxisYZ.load_from_xml(node);
	}

	return result;
}

bool ReadyHumanoidSpineData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* ReadyHumanoidSpineData::create_copy() const
{
	ReadyHumanoidSpineData* copy = new ReadyHumanoidSpineData();
	*copy = *this;
	return copy;
}

AppearanceController* ReadyHumanoidSpineData::create_controller()
{
	return new ReadyHumanoidSpine(this);
}

void ReadyHumanoidSpineData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	pelvisBone.if_value_set([this, _rename]() { pelvisBone = _rename(pelvisBone.get(), NP); });
	headBone.if_value_set([this, _rename]() { headBone = _rename(headBone.get(), NP); });
	rightHandBone.if_value_set([this, _rename]() { rightHandBone = _rename(rightHandBone.get(), NP); });
	leftHandBone.if_value_set([this, _rename]() { leftHandBone = _rename(leftHandBone.get(), NP); });
	rightHandControlBone.if_value_set([this, _rename]() { rightHandControlBone = _rename(rightHandControlBone.get(), NP); });
	leftHandControlBone.if_value_set([this, _rename]() { leftHandControlBone = _rename(leftHandControlBone.get(), NP); });
	outPelvisMSVar.if_value_set([this, _rename]() { outPelvisMSVar = _rename(outPelvisMSVar.get(), NP); });
	outTopSpineMSVar.if_value_set([this, _rename]() { outTopSpineMSVar = _rename(outTopSpineMSVar.get(), NP); });
	spineToHeadOffsetVar.if_value_set([this, _rename]() { spineToHeadOffsetVar = _rename(spineToHeadOffsetVar.get(), NP); });
	rightArmRequestedLengthXYPtVar.if_value_set([this, _rename]() { rightArmRequestedLengthXYPtVar = _rename(rightArmRequestedLengthXYPtVar.get(), NP); });
	leftArmRequestedLengthXYPtVar.if_value_set([this, _rename]() { leftArmRequestedLengthXYPtVar = _rename(leftArmRequestedLengthXYPtVar.get(), NP); });
}

void ReadyHumanoidSpineData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	pelvisBone.fill_value_with(_context);
	headBone.fill_value_with(_context);
	rightHandBone.fill_value_with(_context);
	leftHandBone.fill_value_with(_context);
	rightHandControlBone.fill_value_with(_context);
	leftHandControlBone.fill_value_with(_context);
	outPelvisMSVar.fill_value_with(_context);
	outTopSpineMSVar.fill_value_with(_context);
	offsetHandRight.fill_value_with(_context);
	spineToHeadOffset.fill_value_with(_context);
	spineToHeadOffsetVar.fill_value_with(_context);
	rightArmRequestedLengthXYPtVar.fill_value_with(_context);
	leftArmRequestedLengthXYPtVar.fill_value_with(_context);

	useDirFromHead.fill_value_with(_context);
	useDirFromPelvis.fill_value_with(_context);
	useDirFromHands.fill_value_with(_context);
	minRequestedLengthXYPtToUseDirFromHands.fill_value_with(_context);
	maxRequestedLengthXYPtToUseDirFromPelvis.fill_value_with(_context);
}

void ReadyHumanoidSpineData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);

	// this should get mirrors
	Vector3 resZAxis = Vector3::cross(_transform.get_x_axis(), _transform.get_y_axis());
	if (Vector3::dot(resZAxis, _transform.get_z_axis()) < 0.0f)
	{
		spineToHeadOffset.if_value_set([this, _transform]() { spineToHeadOffset.access().set_translation(_transform.location_to_world(spineToHeadOffset.get().get_translation()));
															  spineToHeadOffset.access().set_orientation(look_matrix33(_transform.vector_to_world(spineToHeadOffset.get().get_axis(Axis::Y)),
																												       _transform.vector_to_world(spineToHeadOffset.get().get_axis(Axis::Z))).to_quat()); });
	}
}
