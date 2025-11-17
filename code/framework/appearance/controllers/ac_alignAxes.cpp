#include "ac_alignAxes.h"

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

DEFINE_STATIC_NAME(alignAxes);

//

REGISTER_FOR_FAST_CAST(AlignAxes);

AlignAxes::AlignAxes(AlignAxesData const * _data)
: base(_data)
, alignAxesData(_data)
, keepChildrenWhereTheyWere(_data->keepChildrenWhereTheyWere)
{
	bone = alignAxesData->bone.get();
	if (alignAxesData->alignAmount.is_set())
	{
		alignAmount = alignAxesData->alignAmount.get();
	}
	first.axis = alignAxesData->first.axis;
	first.bone = alignAxesData->first.bone.get();
	first.dirMS = alignAxesData->first.dirMS.get();
	second.axis = alignAxesData->second.axis;
	second.bone = alignAxesData->second.bone.get();
	second.dirMS = alignAxesData->second.dirMS.get();
}

AlignAxes::~AlignAxes()
{
}

void AlignAxes::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (auto* skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		bone.look_up(skeleton);
		first.bone.look_up(skeleton);
		second.bone.look_up(skeleton);

		if (bone.get_index() != NONE)
		{
			int const boneParentIdx = skeleton->get_parent_of(bone.get_index());

			if (boneParentIdx != NONE)
			{
				boneParent = Meshes::BoneID(skeleton, boneParentIdx);
			}
		}
	}

	keepChildrenWhereTheyWere.initialise(_owner, bone);
}

void AlignAxes::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void AlignAxes::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid() ||
		!first.bone.is_valid() ||
		!second.bone.is_valid() ||
		!boneParent.is_valid())
	{
		return;
	}

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(alignAxes_finalPose);
#endif

#ifdef AN_DEVELOPMENT
	if (!alignAxesData->debugDraw)
#endif
	{
		debug_filter(ac_alignAxes);
	}
	debug_context(this->get_owner()->get_owner()->get_presence()->get_in_room());
	debug_push_transform(this->get_owner()->get_owner()->get_presence()->get_placement());

	Meshes::Pose * poseLS = _context.access_pose_LS();
	auto* skeleton = poseLS->get_skeleton();

	keepChildrenWhereTheyWere.preform_pre_bone_changes(_context);

	Transform boneMS = poseLS->get_bone_ms_from_ls(bone.get_index());
	Transform firstBoneMS = poseLS->get_bone_ms_from_ls(first.bone.get_index());
	Transform secondBoneMS = poseLS->get_bone_ms_from_ls(second.bone.get_index());
	Transform firstBoneDefaultMS = skeleton->get_bones_default_placement_MS()[first.bone.get_index()];
	Transform secondBoneDefaultMS = skeleton->get_bones_default_placement_MS()[second.bone.get_index()];

	Vector3 firstDirBS = firstBoneDefaultMS.vector_to_local(first.dirMS);
	Vector3 secondDirBS = secondBoneDefaultMS.vector_to_local(second.dirMS);

	Vector3 firstDirMS = firstBoneMS.vector_to_world(firstDirBS).normal();
	Vector3 secondDirMS = secondBoneMS.vector_to_world(secondDirBS).normal();

	debug_draw_arrow(true, Colour::red.with_alpha(0.5f), firstBoneMS.get_translation(), firstBoneMS.get_translation() + firstDirMS * 0.07f);
	debug_draw_arrow(true, Colour::green.with_alpha(0.5f), secondBoneMS.get_translation(), secondBoneMS.get_translation() + secondDirMS * 0.07f);

	debug_draw_arrow(true, Colour::red, boneMS.get_translation(), boneMS.get_translation() + firstDirMS * 0.08f);
	debug_draw_arrow(true, Colour::green, boneMS.get_translation(), boneMS.get_translation() + secondDirMS * 0.08f);

	Transform actualBoneMS;
	if (actualBoneMS.build_from_two_axes(first.axis, second.axis, firstDirMS, secondDirMS, boneMS.get_translation()))
	{
		debug_draw_transform_size(true, actualBoneMS, 0.05f);
	}
	else
	{
		actualBoneMS = boneMS;
		warn(TXT("could not align axes"));
	}

	if (alignAmount < 1.0f)
	{
		actualBoneMS = lerp(alignAmount, boneMS, actualBoneMS);
	}

	Transform const boneParentMS = poseLS->get_bone_ms_from_ls(boneParent.get_index());
	poseLS->access_bones()[bone.get_index()] = boneParentMS.to_local(actualBoneMS);

	keepChildrenWhereTheyWere.preform_post_bone_changes(_context);

	debug_pop_transform();
	debug_no_context();
#ifdef AN_DEVELOPMENT
	if (!alignAxesData->debugDraw)
#endif
	{
		debug_no_filter();
	}
}

void AlignAxes::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (bone.is_valid())
	{
		dependsOnParentBones.push_back(bone.get_index());
		providesBones.push_back(bone.get_index());
	}
	if (first.bone.is_valid())
	{
		dependsOnParentBones.push_back(first.bone.get_index());
	}
	if (second.bone.is_valid())
	{
		dependsOnParentBones.push_back(second.bone.get_index());
	}
}

//

REGISTER_FOR_FAST_CAST(AlignAxesData);

AppearanceControllerData* AlignAxesData::create_data()
{
	return new AlignAxesData();
}

void AlignAxesData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(alignAxes), create_data);
}

AlignAxesData::AlignAxesData()
{
}

bool AlignAxesData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

#ifdef AN_DEVELOPMENT
	debugDraw = _node->get_bool_attribute_or_from_child_presence(TXT("debugDraw"), debugDraw);
#endif

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= alignAmount.load_from_xml(_node, TXT("alignAmount"));
	result &= first.load_from_xml(_node, TXT("first"));
	result &= second.load_from_xml(_node, TXT("second"));

	error_loading_xml_on_assert(bone.is_set(), result, _node, TXT("no bone provided"));
	error_loading_xml_on_assert(first.axis != second.axis, result, _node, TXT("same axes chosen"));

	result &= keepChildrenWhereTheyWere.load_from_xml(_node, _lc);

	return result;
}

bool AlignAxesData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* AlignAxesData::create_copy() const
{
	AlignAxesData* copy = new AlignAxesData();
	*copy = *this;
	return copy;
}

AppearanceController* AlignAxesData::create_controller()
{
	return new AlignAxes(this);
}

void AlignAxesData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
	first.bone.if_value_set([this, _rename](){ first.bone = _rename(first.bone.get(), NP); });
	second.bone.if_value_set([this, _rename]() { second.bone = _rename(second.bone.get(), NP); });
}

void AlignAxesData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);

	first.dirMS.if_value_set([this, _transform]() { first.dirMS.access() = _transform.vector_to_world(first.dirMS.get()); });
	second.dirMS.if_value_set([this, _transform]() { second.dirMS.access() = _transform.vector_to_world(second.dirMS.get()); });
}

void AlignAxesData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	alignAmount.fill_value_with(_context);
	first.bone.fill_value_with(_context);
	first.dirMS.fill_value_with(_context);
	second.bone.fill_value_with(_context);
	second.dirMS.fill_value_with(_context);

	keepChildrenWhereTheyWere.apply_mesh_gen_params(_context);
}

//

bool AlignAxesData::AxisInfo::load_from_xml(IO::XML::Node const * _node, tchar const * _childName)
{
	bool result = true;

	for_every(node, _node->children_named(_childName))
	{
		if (auto * attr = node->get_attribute(TXT("axis")))
		{
			axis = Axis::parse_from(attr->get_as_string());
		}
		else
		{
			error_loading_xml(node, TXT("no \"axes\" defined"));
			result = false;
		}

		bone.load_from_xml(node, TXT("bone"));
		error_loading_xml_on_assert(bone.is_set(), result, node, TXT("no \"bone\" defined"));

		dirMS.load_from_xml_child_node(node, TXT("dirMS"));
		error_loading_xml_on_assert(dirMS.is_set(), result, node, TXT("no \"dirMS\" defined"));
	}


	return result;
}

