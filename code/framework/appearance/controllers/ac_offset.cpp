#include "ac_offset.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(offset);

//

REGISTER_FOR_FAST_CAST(Offset);

Offset::Offset(OffsetData const * _data)
: base(_data)
, offsetData(_data)
{
	bone = offsetData->bone.get();
	if (offsetData->offsetPlacementBSVar.is_set())
	{
		offsetPlacementBSVar = offsetData->offsetPlacementBSVar.get();
	}
	offsetPlacementBS = offsetData->offsetPlacementBS.get();
	offsetLocationBS = offsetData->offsetLocationBS.get();
	offsetRotationBS = offsetData->offsetRotationBS.get();
}

Offset::~Offset()
{
}

void Offset::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	bone.look_up(get_owner()->get_skeleton()->get_skeleton());

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	offsetPlacementBSVar.look_up<Transform>(varStorage);
}

void Offset::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void Offset::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	if (! get_active())
	{
		return;
	}

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(offset_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid())
	{
		return;
	}

	// calculate offset to apply
	Transform offsetTotal = Transform::identity;

	if (offsetPlacementBSVar.is_valid())
	{
		offsetTotal = offsetPlacementBSVar.get<Transform>();
	}
	offsetTotal = offsetTotal.to_world(offsetPlacementBS);
	offsetTotal.set_translation(offsetTotal.get_translation() + offsetLocationBS);
	offsetTotal.set_orientation(offsetTotal.get_orientation().rotate_by(offsetRotationBS.to_quat()));

	offsetTotal = Transform::lerp(get_active(), Transform::identity, offsetTotal);

	// apply!
	Meshes::Pose * poseLS = _context.access_pose_LS();
	int const boneIdx = bone.get_index();
	Transform boneLS = poseLS->get_bone(boneIdx);
	boneLS = boneLS.to_world(offsetTotal);
	
	// write back
	poseLS->access_bones()[boneIdx] = boneLS;
}

void Offset::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (bone.is_valid())
	{
		dependsOnParentBones.push_back(bone.get_index());
		providesBones.push_back(bone.get_index());
	}
	if (offsetPlacementBSVar.is_valid())
	{
		dependsOnVariables.push_back(offsetPlacementBSVar.get_name());
	}
}

//

REGISTER_FOR_FAST_CAST(OffsetData);

AppearanceControllerData* OffsetData::create_data()
{
	return new OffsetData();
}

void OffsetData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(offset), create_data);
}

OffsetData::OffsetData()
{
}

bool OffsetData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= offsetPlacementBSVar.load_from_xml(_node, TXT("offsetPlacementBSVarID"));
	result &= offsetPlacementBS.load_from_xml_child_node(_node, TXT("offsetPlacementBS"));
	result &= offsetLocationBS.load_from_xml_child_node(_node, TXT("offsetLocationBS"));
	result &= offsetRotationBS.load_from_xml_child_node(_node, TXT("offsetRotationBS"));

	return result;
}

bool OffsetData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* OffsetData::create_copy() const
{
	OffsetData* copy = new OffsetData();
	*copy = *this;
	return copy;
}

AppearanceController* OffsetData::create_controller()
{
	return new Offset(this);
}

void OffsetData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	offsetPlacementBSVar.fill_value_with(_context);
	offsetPlacementBS.fill_value_with(_context);
	offsetLocationBS.fill_value_with(_context);
	offsetRotationBS.fill_value_with(_context);
}

void OffsetData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);

	// this should get mirrors
	Vector3 resZAxis = Vector3::cross(_transform.get_x_axis(), _transform.get_y_axis());
	if (Vector3::dot(resZAxis, _transform.get_z_axis()) < 0.0f)
	{
		offsetPlacementBS.if_value_set([this, _transform]() { offsetPlacementBS.access().set_translation(_transform.location_to_world(offsetPlacementBS.get().get_translation()));
															  offsetPlacementBS.access().set_orientation(look_matrix33(_transform.vector_to_world(offsetPlacementBS.get().get_axis(Axis::Y)),
																													   _transform.vector_to_world(offsetPlacementBS.get().get_axis(Axis::Z))).to_quat()); });
		offsetLocationBS.if_value_set([this, _transform]() { offsetLocationBS.access() = _transform.location_to_world(offsetLocationBS.get()); });
		offsetRotationBS.if_value_set([this, _transform]() { offsetRotationBS.access() = look_matrix33(_transform.vector_to_world(offsetRotationBS.get().get_axis(Axis::Y)),
																									   _transform.vector_to_world(offsetRotationBS.get().get_axis(Axis::Z))).to_quat().to_rotator(); });
	}
}

void OffsetData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
	offsetPlacementBSVar.if_value_set([this, _rename](){ offsetPlacementBSVar = _rename(offsetPlacementBSVar.get(), NP); });
}
