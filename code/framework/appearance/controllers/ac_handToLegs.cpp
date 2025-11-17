#include "ac_handToLegs.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(handToLegs);

//

REGISTER_FOR_FAST_CAST(HandToLegs);

HandToLegs::HandToLegs(HandToLegsData const * _data)
: base(_data)
, isValid(false)
{
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(bone);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(provideBone);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(relToBone);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(offsetBlendTime);

	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(useScaleFromLeg);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(addLength);

	alongAxis = _data->alongAxis;
	awayAxis = _data->awayAxis;
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(addOffsetToAlong);

	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(diffMul);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(diffRot);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(addOffset);
	
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(pelvisBone);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(spineTopBone);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(spineRadius);

	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(legBone);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(legRelToBone);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(otherLegBone);
}

HandToLegs::~HandToLegs()
{
}

void HandToLegs::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (!provideBone.get_name().is_valid())
	{
		provideBone = bone;
	}

	if (Meshes::Skeleton const* skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		// build chain
		bone.look_up(skeleton);
		provideBone.look_up(skeleton);
		relToBone.look_up(skeleton);

		pelvisBone.look_up(skeleton);
		spineTopBone.look_up(skeleton);

		legBone.look_up(skeleton);
		legRelToBone.look_up(skeleton);
		otherLegBone.look_up(skeleton);
	}

	isValid = bone.is_valid() && relToBone.is_valid() && legBone.is_valid() && legRelToBone.is_name_valid();
}

void HandToLegs::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void HandToLegs::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	auto* fskeleton = get_owner()->get_skeleton();
	if (!isValid || ! fskeleton || !fskeleton->get_skeleton() || get_active() <= 0.0f)
	{
		offset = Vector3::zero;
		return;
	}

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(handToLegs_finalPose);
#endif

	debug_filter(ac_handToLegs);
	debug_context(this->get_owner()->get_owner()->get_presence()->get_in_room());
	debug_push_transform(this->get_owner()->get_owner()->get_presence()->get_placement());

	Meshes::Pose const * prevPoseMS = get_owner()->get_final_pose_MS(true);
	Meshes::Pose * poseLS = _context.access_pose_LS();

	Vector3 relToBoneMS = prevPoseMS->get_bone(relToBone.get_index()).get_translation();
	Vector3 legBoneMS = prevPoseMS->get_bone(legBone.get_index()).get_translation();
	Vector3 legRelToBoneMS = prevPoseMS->get_bone(legRelToBone.get_index()).get_translation();

	Optional<Vector3> pelvisBoneMS;
	Optional<Vector3> spineTopBoneMS;
	
	Optional<Vector3> otherLegBoneMS;

	if (pelvisBone.is_valid()) pelvisBoneMS = prevPoseMS->get_bone(pelvisBone.get_index()).get_translation();
	if (spineTopBone.is_valid()) spineTopBoneMS = prevPoseMS->get_bone(spineTopBone.get_index()).get_translation();
	if (otherLegBone.is_valid()) otherLegBoneMS = prevPoseMS->get_bone(otherLegBone.get_index()).get_translation();

	auto* skeleton = fskeleton->get_skeleton();

	Vector3 defBoneMS = skeleton->get_bones_default_placement_MS()[bone.get_index()].get_translation();
	Vector3 defRelToBoneMS = skeleton->get_bones_default_placement_MS()[relToBone.get_index()].get_translation();
	Vector3 defLegBoneMS = skeleton->get_bones_default_placement_MS()[legBone.get_index()].get_translation();
	Vector3 defLegRelToBoneMS = skeleton->get_bones_default_placement_MS()[legRelToBone.get_index()].get_translation();
	
	float defLength = (defRelToBoneMS - defBoneMS).length();
	float legLength = abs((legRelToBoneMS - legBoneMS).z);
	float defLegLength = abs((defLegRelToBoneMS - defLegBoneMS).z);

	Vector3 legDiff = (legBoneMS - legRelToBoneMS) - (defLegBoneMS - defLegRelToBoneMS);

	Vector3 baseOffset = (defLegBoneMS - defLegRelToBoneMS) * legLength / defLegLength;

	Vector3 useDiff = diffMul * legDiff;
	useDiff *= legLength / defLegLength;

	useDiff = diffRot.to_quat().to_world(useDiff);

	Vector3 wholeOffset = baseOffset + useDiff + addOffset;
	{
		float beLength = defLength * lerp(useScaleFromLeg, 1.0f, legLength / defLegLength) + addLength;
		wholeOffset = wholeOffset.normal() * beLength;

		offset = blend_to_using_time(offset, wholeOffset, offsetBlendTime, _context.get_delta_time());

		offset = offset.normal() * beLength;
		wholeOffset = offset;
	}
	Vector3 newLocMS = relToBoneMS + wholeOffset;

	todo_note(TXT("spine/pelvis distance"));

	Transform finalBoneMS;
	finalBoneMS.build_from_two_axes(alongAxis, awayAxis,
		/* along */	((newLocMS - relToBoneMS).normal() + addOffsetToAlong).normal(),
		/* away  */	lerp(0.5f, defBoneMS.normal_2d(), newLocMS.normal_2d()).normal(),
					newLocMS);

	Transform currentBoneMS = poseLS->get_bone_ms_from_ls(provideBone.get_index());
	finalBoneMS = lerp(get_active(), currentBoneMS, finalBoneMS);

	poseLS->set_bone_ms(provideBone.get_index(), finalBoneMS);
}

void HandToLegs::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	// depends on bones from previous
	providesBones.push_back(provideBone.get_index());
}

//

REGISTER_FOR_FAST_CAST(HandToLegsData);

AppearanceControllerData* HandToLegsData::create_data()
{
	return new HandToLegsData();
}

void HandToLegsData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(handToLegs), create_data);
}

HandToLegsData::HandToLegsData()
{
}

bool HandToLegsData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= provideBone.load_from_xml(_node, TXT("provideBone"));
	result &= relToBone.load_from_xml(_node, TXT("relToBone"));
	result &= offsetBlendTime.load_from_xml(_node, TXT("offsetBlendTime"));

	result &= useScaleFromLeg.load_from_xml(_node, TXT("useScaleFromLeg"));
	result &= addLength.load_from_xml(_node, TXT("addLength"));

	if (auto* attr = _node->get_attribute(TXT("alongAxis")))
	{
		alongAxis = Axis::parse_from(attr->get_as_string());
	}
	if (auto* attr = _node->get_attribute(TXT("awayAxis")))
	{
		awayAxis = Axis::parse_from(attr->get_as_string());
	}
	result &= addOffsetToAlong.load_from_xml(_node, TXT("addOffsetToAlong"));

	result &= diffMul.load_from_xml(_node, TXT("diffMul"));
	result &= diffRot.load_from_xml(_node, TXT("diffRot"));
	result &= addOffset.load_from_xml(_node, TXT("addOffset"));

	result &= pelvisBone.load_from_xml(_node, TXT("pelvisBone"));
	result &= spineTopBone.load_from_xml(_node, TXT("spineTopBone"));
	result &= spineRadius.load_from_xml(_node, TXT("spineRadius"));

	result &= legBone.load_from_xml(_node, TXT("legBone"));
	result &= legRelToBone.load_from_xml(_node, TXT("legRelToBone"));
	result &= otherLegBone.load_from_xml(_node, TXT("otherLegBone"));

	return result;
}

bool HandToLegsData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* HandToLegsData::create_copy() const
{
	HandToLegsData* copy = new HandToLegsData();
	*copy = *this;
	return copy;
}

AppearanceController* HandToLegsData::create_controller()
{
	return new HandToLegs(this);
}

void HandToLegsData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
	provideBone.if_value_set([this, _rename](){ provideBone = _rename(provideBone.get(), NP); });
	relToBone.if_value_set([this, _rename](){ relToBone = _rename(relToBone.get(), NP); });
	
	pelvisBone.if_value_set([this, _rename](){ pelvisBone = _rename(pelvisBone.get(), NP); });
	spineTopBone.if_value_set([this, _rename](){ spineTopBone = _rename(spineTopBone.get(), NP); });
	
	legBone.if_value_set([this, _rename](){ legBone = _rename(legBone.get(), NP); });
	legRelToBone.if_value_set([this, _rename](){ legRelToBone = _rename(legRelToBone.get(), NP); });
	otherLegBone.if_value_set([this, _rename](){ otherLegBone = _rename(otherLegBone.get(), NP); });
}

void HandToLegsData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void HandToLegsData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	provideBone.fill_value_with(_context);
	relToBone.fill_value_with(_context);
	offsetBlendTime.fill_value_with(_context);

	useScaleFromLeg.fill_value_with(_context);
	addLength.fill_value_with(_context);
	
	addOffsetToAlong.fill_value_with(_context);

	diffMul.fill_value_with(_context);
	diffRot.fill_value_with(_context);
	addOffset.fill_value_with(_context);
	
	pelvisBone.fill_value_with(_context);
	spineTopBone.fill_value_with(_context);
	spineRadius.fill_value_with(_context);
	
	legBone.fill_value_with(_context);
	legRelToBone.fill_value_with(_context);
	otherLegBone.fill_value_with(_context);
}

