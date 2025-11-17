#include "ac_providePlacementFromBoneMS.h"

#include "ikUtils.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"
#include "..\..\..\core\wheresMyPoint\iWMPTool.h"

#include "..\..\..\core\performance\performanceUtils.h"
#include "..\..\..\core\system\input.h"

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(providePlacementFromBoneMS);

//

REGISTER_FOR_FAST_CAST(ProvidePlacementFromBoneMS);

ProvidePlacementFromBoneMS::ProvidePlacementFromBoneMS(ProvidePlacementFromBoneMSData const * _data)
: base(_data)
, providePlacementFromBoneMSData(_data)
{
	resultPlacementMSVar = providePlacementFromBoneMSData->resultPlacementMSVar.get();

	bone = providePlacementFromBoneMSData->bone.get(Name::invalid());
	
	offset = providePlacementFromBoneMSData->offset.get(Transform::identity);
}

ProvidePlacementFromBoneMS::~ProvidePlacementFromBoneMS()
{
}

void ProvidePlacementFromBoneMS::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	resultPlacementMSVar.look_up<Transform>(varStorage);

	if (Meshes::Skeleton const * skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		bone.look_up(skeleton);
	}
}

void ProvidePlacementFromBoneMS::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void ProvidePlacementFromBoneMS::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(providePlacementFromBoneMS_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();
	if (!mesh)
	{
		return;
	}

	Meshes::Pose * poseLS = _context.access_pose_LS();

	Transform placementMS = poseLS->get_bone_ms_from_ls(bone.get_index());

	placementMS = placementMS.to_world(offset);

	resultPlacementMSVar.access<Transform>() = placementMS;
}

void ProvidePlacementFromBoneMS::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);
}

//

REGISTER_FOR_FAST_CAST(ProvidePlacementFromBoneMSData);

AppearanceControllerData* ProvidePlacementFromBoneMSData::create_data()
{
	return new ProvidePlacementFromBoneMSData();
}

void ProvidePlacementFromBoneMSData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(providePlacementFromBoneMS), create_data);
}

ProvidePlacementFromBoneMSData::ProvidePlacementFromBoneMSData()
{
}

bool ProvidePlacementFromBoneMSData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= resultPlacementMSVar.load_from_xml(_node, TXT("resultPlacementMSVarID"));
	result &= resultPlacementMSVar.load_from_xml(_node, TXT("outPlacementMSVarID"));

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= offset.load_from_xml(_node, TXT("offset"));

	return result;
}

bool ProvidePlacementFromBoneMSData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* ProvidePlacementFromBoneMSData::create_copy() const
{
	ProvidePlacementFromBoneMSData* copy = new ProvidePlacementFromBoneMSData();
	*copy = *this;
	return copy;
}

AppearanceController* ProvidePlacementFromBoneMSData::create_controller()
{
	return new ProvidePlacementFromBoneMS(this);
}

void ProvidePlacementFromBoneMSData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	resultPlacementMSVar.if_value_set([this, _rename]() { resultPlacementMSVar = _rename(resultPlacementMSVar.get(), NP); });
	bone.if_value_set([this, _rename]() { bone = _rename(bone.get(), NP); });
}

void ProvidePlacementFromBoneMSData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	resultPlacementMSVar.fill_value_with(_context);
	bone.fill_value_with(_context);
	offset.fill_value_with(_context);
}

void ProvidePlacementFromBoneMSData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);

	// this should get mirrors
	Vector3 resZAxis = Vector3::cross(_transform.get_x_axis(), _transform.get_y_axis());
	if (Vector3::dot(resZAxis, _transform.get_z_axis()) < 0.0f)
	{
		offset.if_value_set([this, _transform]() { offset.access().set_translation(_transform.location_to_world(offset.get().get_translation()));
												   offset.access().set_orientation(look_matrix33(_transform.vector_to_world(offset.get().get_axis(Axis::Y)),
																								 _transform.vector_to_world(offset.get().get_axis(Axis::Z))).to_quat()); });
	}
}
