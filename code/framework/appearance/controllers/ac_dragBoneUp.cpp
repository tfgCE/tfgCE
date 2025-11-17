#include "ac_dragBoneUp.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\mesh\pose.h"
#include "..\..\..\core\performance\performanceUtils.h"

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(dragBoneUp);

//

REGISTER_FOR_FAST_CAST(DragBoneUp);

DragBoneUp::DragBoneUp(DragBoneUpData const * _data)
: base(_data)
, dragBoneUpData(_data)
{
	bone = dragBoneUpData->bone.get();
	toBone = dragBoneUpData->toBone.get();
	if (dragBoneUpData->upDirBS.is_set())
	{
		upDirBS = dragBoneUpData->upDirBS.get();
	}
	if (dragBoneUpData->rightDirBS.is_set())
	{
		rightDirBS = dragBoneUpData->rightDirBS.get();
	}
	forwardDirBS = dragBoneUpData->forwardDirBS.get();
	if (dragBoneUpData->angleLimit.is_set())
	{
		angleLimitDot = cos_deg(dragBoneUpData->angleLimit.get());
	}	
}

DragBoneUp::~DragBoneUp()
{
}

void DragBoneUp::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (auto* skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		bone.look_up(skeleton);
		toBone.look_up(skeleton);
		parentBone = Meshes::BoneID(skeleton, skeleton->get_parent_of(bone.get_index()));
	}
}

void DragBoneUp::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void DragBoneUp::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(dragBoneUp_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid() ||
		!toBone.is_valid() ||
		!get_active())
	{
		return;
	}

	auto* imo = get_owner()->get_owner();
	auto* presence = imo->get_presence();

	debug_filter(ac_dragBoneUp);

	debug_context(presence->get_in_room());

	Transform placementWS = presence->get_placement();
	Transform prevPlacementWS = placementWS.to_world(presence->get_prev_placement_offset());

	Meshes::Pose * poseLS = _context.access_pose_LS();
	Meshes::Pose * prevPoseLS = _context.access_prev_pose_LS();

	int const boneIdx = bone.get_index();

	Transform boneMS = poseLS->get_bone_ms_from_ls(bone.get_index());
	Transform boneWS = placementWS.to_world(boneMS);
	Transform parentBoneMS = poseLS->get_bone_ms_from_ls(parentBone.get_index());
	Transform parentBoneWS = placementWS.to_world(parentBoneMS);
	Transform dragBoneUpWS = prevPlacementWS.to_world(prevPoseLS->get_bone_ms_from_ls(toBone.get_index()));

	debug_draw_arrow(true, Colour::blue, boneWS.get_translation(), boneWS.location_to_world(forwardDirBS));
	debug_draw_arrow(true, Colour::green, boneWS.get_translation(), dragBoneUpWS.get_translation());

	Vector3 requestedDirWS = (dragBoneUpWS.get_translation() - boneWS.get_translation()).normal();
	Transform currentWS = Transform::identity;
	Transform requestedWS = Transform::identity;

	Vector3 forwardDirWS = boneWS.vector_to_world(forwardDirBS);
	if (angleLimitDot.is_set())
	{
		requestedDirWS = requestedDirWS.keep_within_angle_normalised(forwardDirWS, angleLimitDot.get());
	}
	requestedDirWS.normalise(); // why?
	if (upDirBS.is_set())
	{
		Vector3 upDirWS= boneWS.vector_to_world(upDirBS.get());
		currentWS = look_matrix(boneWS.get_translation(), forwardDirWS, upDirWS).to_transform();
		requestedWS = look_matrix(boneWS.get_translation(), requestedDirWS, upDirWS).to_transform();
	}
	else if (rightDirBS.is_set())
	{
		Vector3 rightDirWS = boneWS.vector_to_world(rightDirBS.get());
		currentWS = look_matrix_with_right(boneWS.get_translation(), forwardDirWS, rightDirWS).to_transform();
		requestedWS = look_matrix_with_right(boneWS.get_translation(), requestedDirWS, rightDirWS).to_transform();
	}
	
	Transform newBoneWS = requestedWS.to_world(currentWS.to_local(boneWS));

	Transform finalBoneWS = get_active() < 1.0f? Transform::lerp(get_active(), boneWS, newBoneWS) : newBoneWS;
	poseLS->access_bones()[boneIdx] = parentBoneWS.to_local(finalBoneWS);

	debug_no_context();

	debug_no_filter();
}

void DragBoneUp::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (bone.is_valid())
	{
		dependsOnParentBones.push_back(bone.get_index());
		providesBones.push_back(bone.get_index());
	}
	if (toBone.is_valid())
	{
		dependsOnParentBones.push_back(toBone.get_index());
	}
}

//

REGISTER_FOR_FAST_CAST(DragBoneUpData);

AppearanceControllerData* DragBoneUpData::create_data()
{
	return new DragBoneUpData();
}

void DragBoneUpData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(dragBoneUp), create_data);
}

DragBoneUpData::DragBoneUpData()
{
}

bool DragBoneUpData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= toBone.load_from_xml(_node, TXT("toBone"));
	
	result &= upDirBS.load_from_xml(_node, TXT("upDirBS"));
	result &= rightDirBS.load_from_xml(_node, TXT("rightDirBS"));	
	result &= forwardDirBS.load_from_xml(_node, TXT("forwardDirBS"));

	result &= angleLimit.load_from_xml(_node, TXT("angleLimit"));

	if (!bone.is_set())
	{
		error_loading_xml(_node, TXT("no bone provided"));
		result = false;
	}
	
	return result;
}

bool DragBoneUpData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* DragBoneUpData::create_copy() const
{
	DragBoneUpData* copy = new DragBoneUpData();
	*copy = *this;
	return copy;
}

AppearanceController* DragBoneUpData::create_controller()
{
	return new DragBoneUp(this);
}

void DragBoneUpData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
	toBone.if_value_set([this, _rename]() { toBone = _rename(toBone.get(), NP); });
}

void DragBoneUpData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	toBone.fill_value_with(_context);

	upDirBS.fill_value_with(_context);
	rightDirBS.fill_value_with(_context);
	forwardDirBS.fill_value_with(_context);
	angleLimit.fill_value_with(_context);
}

