#include "ac_alignmentRoll.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\performance\performanceUtils.h"

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(alignmentRoll);

//

REGISTER_FOR_FAST_CAST(AlignmentRoll);

AlignmentRoll::AlignmentRoll(AlignmentRollData const * _data)
: base(_data)
, alignmentRollData(_data)
{
	bone = alignmentRollData->bone.get();
	alignToBone = alignmentRollData->alignToBone.get();
	alignToBoneB = alignmentRollData->alignToBoneB.get(Name::invalid());
	alignToBoneBWeight = alignmentRollData->alignToBoneBWeight.get(0.0f);
	smoothAlignBoneTime = alignmentRollData->smoothAlignBoneTime.get(0.2f);
}

AlignmentRoll::~AlignmentRoll()
{
}

void AlignmentRoll::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (auto* skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		bone.look_up(skeleton);
		alignToBone.look_up(skeleton);
		alignToBoneB.look_up(skeleton);

		if (bone.get_index() != NONE)
		{
			int const boneParentIdx = skeleton->get_parent_of(bone.get_index());

			if (boneParentIdx != NONE)
			{
				boneParent = Meshes::BoneID(skeleton, boneParentIdx);
			}
		}
	}
}

void AlignmentRoll::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void AlignmentRoll::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid() ||
		!alignToBone.is_valid() ||
		!boneParent.is_valid())
	{
		return;
	}

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(alignmentRoll_finalPose);
#endif

#ifdef AN_DEVELOPMENT
	if (!alignmentRollData->debugDraw)
#endif
	{
		debug_filter(ac_alignmentRoll);
	}
	debug_context(this->get_owner()->get_owner()->get_presence()->get_in_room());
	debug_push_transform(this->get_owner()->get_owner()->get_presence()->get_placement());

	Meshes::Pose * poseLS = _context.access_pose_LS();

	Transform boneMS = poseLS->get_bone_ms_from_ls(bone.get_index());
	Transform alignToBoneMS = poseLS->get_bone_ms_from_ls(alignToBone.get_index());

	if (alignToBoneBWeight > 0.0f && alignToBoneB.is_valid())
	{
		Transform alignToBoneBMS = poseLS->get_bone_ms_from_ls(alignToBoneB.get_index());

		alignToBoneMS = Transform::lerp(alignToBoneBWeight, alignToBoneMS, alignToBoneBMS);

		// smooth
		{
			if (has_just_activated())
			{
				prevAlignBoneMS.clear();
			}
			if (prevAlignBoneMS.is_set())
			{
				alignToBoneMS.set_orientation(Quat::slerp(clamp(smoothAlignBoneTime == 0.0f? 1.0f : _context.get_delta_time() / smoothAlignBoneTime, 0.0f, 1.0f), prevAlignBoneMS.get().get_orientation(), alignToBoneMS.get_orientation()));
			}
			prevAlignBoneMS = alignToBoneMS;
		}
	}

	Vector3 alignToAxis = alignToBoneMS.get_axis(alignmentRollData->alignToAxis) * alignmentRollData->alignToAxisSign;

	if (alignmentRollData->maintainLastAlignedAxisSide && !lastAlignedAxisMS.is_zero())
	{
		Vector3 ourAxis = boneMS.get_axis(alignmentRollData->usingAxis);
		if (Vector3::dot(ourAxis, lastAlignedAxisMS) < 0.0f)
		{
			alignToAxis = -alignToAxis;
		}
	}

	debug_draw_arrow(true, Colour::green, boneMS.get_translation(), boneMS.get_translation() + alignToAxis * 0.2f);

	if (alignmentRollData->secondaryAlignToAxis.is_set())
	{
		if (alignmentRollData->usingAxis == Axis::X ||
			alignmentRollData->usingAxis == Axis::Z)
		{
			float aligningWithY = Vector3::dot(alignToAxis, boneMS.get_axis(Axis::Y));
			float useAlignToAxis = 1.0f - abs(aligningWithY);
			alignToAxis = (useAlignToAxis * alignToAxis + alignmentRollData->secondaryAlignToAxisSign * (aligningWithY)* alignToBoneMS.get_axis(alignmentRollData->secondaryAlignToAxis.get())).normal();
		}
		else
		{
			float aligningWithKeep = Vector3::dot(alignToAxis, boneMS.get_axis(alignmentRollData->forYAxisKeep));
			float useAlignToAxis = 1.0f - abs(aligningWithKeep);
			alignToAxis = (useAlignToAxis * alignToAxis + alignmentRollData->secondaryAlignToAxisSign * (aligningWithKeep)* alignToBoneMS.get_axis(alignmentRollData->secondaryAlignToAxis.get())).normal();
		}
	}

	if (alignmentRollData->usingAxis == Axis::X)
	{
		debug_draw_arrow(true, Colour::red, boneMS.get_translation(), boneMS.get_translation() + boneMS.get_axis(Axis::X) * 0.2f);
		boneMS = look_matrix_with_right(boneMS.get_translation(), boneMS.get_axis(Axis::Y), alignToAxis).to_transform();
	}
	else if (alignmentRollData->usingAxis == Axis::Z)
	{
		debug_draw_arrow(true, Colour::red, boneMS.get_translation(), boneMS.get_translation() + boneMS.get_axis(Axis::Z) * 0.2f);
		boneMS = look_matrix(boneMS.get_translation(), boneMS.get_axis(Axis::Y), alignToAxis).to_transform();
	}
	else if (alignmentRollData->usingAxis == Axis::Y)
	{
		debug_draw_arrow(true, Colour::red, boneMS.get_translation(), boneMS.get_translation() + boneMS.get_axis(Axis::Y) * 0.2f);
		if (alignmentRollData->forYAxisKeep == Axis::X)
		{
			Vector3 otherAxis = boneMS.get_axis(Axis::X);
			boneMS = look_matrix_with_right(boneMS.get_translation(), alignToAxis.drop_using_normalised(otherAxis).normal(), otherAxis).to_transform();
		}
		else if (alignmentRollData->forYAxisKeep == Axis::Z)
		{
			Vector3 otherAxis = boneMS.get_axis(Axis::Z);
			boneMS = look_matrix(boneMS.get_translation(), alignToAxis.drop_using_normalised(otherAxis).normal(), otherAxis).to_transform();
		}
		else
		{
			error_stop(TXT("this does not make any sense"));
		}
	}
	debug_draw_arrow(true, Colour::greenDark, boneMS.get_translation(), boneMS.get_translation() + alignToAxis * 0.2f);

	lastAlignedAxisMS = boneMS.get_axis(alignmentRollData->usingAxis);

	Transform const boneParentMS = poseLS->get_bone_ms_from_ls(boneParent.get_index());
	Transform& boneLS = poseLS->access_bones()[bone.get_index()];
	boneLS = lerp(get_active(), boneLS, boneParentMS.to_local(boneMS));

	debug_draw_transform_size(true, boneMS, 0.1f);

	debug_pop_transform();
	debug_no_context();
#ifdef AN_DEVELOPMENT
	if (!alignmentRollData->debugDraw)
#endif
	{
		debug_no_filter();
	}
}

void AlignmentRoll::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (bone.is_valid())
	{
		dependsOnParentBones.push_back(bone.get_index());
		providesBones.push_back(bone.get_index());
	}
	if (alignToBone.is_valid())
	{
		dependsOnParentBones.push_back(alignToBone.get_index());
	}
	if (alignToBoneB.is_valid())
	{
		dependsOnParentBones.push_back(alignToBoneB.get_index());
	}
}

//

REGISTER_FOR_FAST_CAST(AlignmentRollData);

AppearanceControllerData* AlignmentRollData::create_data()
{
	return new AlignmentRollData();
}

void AlignmentRollData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(alignmentRoll), create_data);
}

AlignmentRollData::AlignmentRollData()
{
}

bool AlignmentRollData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

#ifdef AN_DEVELOPMENT
	debugDraw = _node->get_bool_attribute_or_from_child_presence(TXT("debugDraw"), debugDraw);
#endif

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= alignToBone.load_from_xml(_node, TXT("alignTo"));
	result &= alignToBone.load_from_xml(_node, TXT("alignToBone"));
	result &= alignToBoneB.load_from_xml(_node, TXT("alignToB"));
	result &= alignToBoneB.load_from_xml(_node, TXT("alignToBoneB"));
	result &= alignToBoneBWeight.load_from_xml(_node, TXT("alignToBWeight"));
	result &= alignToBoneBWeight.load_from_xml(_node, TXT("alignToBoneBWeight"));
	result &= smoothAlignBoneTime.load_from_xml(_node, TXT("smoothAlignBoneTime"));

	usingAxis = Axis::parse_from(_node->get_string_attribute_or_from_child(TXT("usingAxis")), usingAxis);
	forYAxisKeep = Axis::parse_from(_node->get_string_attribute_or_from_child(TXT("forYAxisKeep")), forYAxisKeep);
	alignToAxis = usingAxis;
	alignToAxis = Axis::parse_from(_node->get_string_attribute_or_from_child(TXT("alignToAxis")), alignToAxis);
	alignToAxisSign = sign(_node->get_float_attribute_or_from_child(TXT("alignToAxisSign"), alignToAxisSign));

	{
		String attr = _node->get_string_attribute_or_from_child(TXT("secondaryAlignToAxis"));
		if (!attr.is_empty())
		{
			secondaryAlignToAxis = Axis::parse_from(attr);
			secondaryAlignToAxisSign = sign(_node->get_float_attribute_or_from_child(TXT("secondaryAlignToAxisSign"), secondaryAlignToAxisSign));
		}
	}


	maintainLastAlignedAxisSide = _node->get_bool_attribute_or_from_child_presence(TXT("maintainLastAlignedAxisSide"), maintainLastAlignedAxisSide);

	error_loading_xml_on_assert(bone.is_set(), result, _node, TXT("no bone provided"));
	error_loading_xml_on_assert(alignToBone.is_set(), result, _node, TXT("no bone provided"));

	return result;
}

bool AlignmentRollData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* AlignmentRollData::create_copy() const
{
	AlignmentRollData* copy = new AlignmentRollData();
	*copy = *this;
	return copy;
}

AppearanceController* AlignmentRollData::create_controller()
{
	return new AlignmentRoll(this);
}

void AlignmentRollData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
	alignToBone.if_value_set([this, _rename](){ alignToBone = _rename(alignToBone.get(), NP); });
	alignToBoneB.if_value_set([this, _rename](){ alignToBoneB = _rename(alignToBoneB.get(), NP); });
}

void AlignmentRollData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	alignToBone.fill_value_with(_context);
	alignToBoneB.fill_value_with(_context);
	alignToBoneBWeight.fill_value_with(_context);
	smoothAlignBoneTime.fill_value_with(_context);
}

