#include "ac_storeBoneOffset.h"

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

DEFINE_STATIC_NAME(StoreBoneOffset);

//

REGISTER_FOR_FAST_CAST(StoreBoneOffset);

StoreBoneOffset::StoreBoneOffset(StoreBoneOffsetData const * _data)
: base(_data)
, storeBoneOffsetData(_data)
{
	storeVar = storeBoneOffsetData->storeVar.get();

	bone = storeBoneOffsetData->bone.get(Name::invalid());
	relToBone = storeBoneOffsetData->relToBone.get(Name::invalid());
}

StoreBoneOffset::~StoreBoneOffset()
{
}

void StoreBoneOffset::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	storeVar.look_up<Transform>(varStorage);

	if (Meshes::Skeleton const * skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		bone.look_up(skeleton);
		relToBone.look_up(skeleton);
	}
}

void StoreBoneOffset::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void StoreBoneOffset::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(StoreBoneOffset_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();
	if (!mesh)
	{
		return;
	}

	Meshes::Pose * poseLS = _context.access_pose_LS();

	Transform boneMS = poseLS->get_bone_ms_from_ls(bone.get_index());
	Transform relToBoneMS = poseLS->get_bone_ms_from_ls(relToBone.get_index());

	Transform offset = relToBoneMS.to_local(boneMS);
	storeVar.access<Transform>() = offset;
}

void StoreBoneOffset::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);
}

//

REGISTER_FOR_FAST_CAST(StoreBoneOffsetData);

AppearanceControllerData* StoreBoneOffsetData::create_data()
{
	return new StoreBoneOffsetData();
}

void StoreBoneOffsetData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(StoreBoneOffset), create_data);
}

StoreBoneOffsetData::StoreBoneOffsetData()
{
}

bool StoreBoneOffsetData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= storeVar.load_from_xml(_node, TXT("storeVarID"));

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= relToBone.load_from_xml(_node, TXT("relToBone"));

	return result;
}

bool StoreBoneOffsetData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* StoreBoneOffsetData::create_copy() const
{
	StoreBoneOffsetData* copy = new StoreBoneOffsetData();
	*copy = *this;
	return copy;
}

AppearanceController* StoreBoneOffsetData::create_controller()
{
	return new StoreBoneOffset(this);
}

void StoreBoneOffsetData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	storeVar.if_value_set([this, _rename]() { storeVar = _rename(storeVar.get(), NP); });
	bone.if_value_set([this, _rename]() { bone = _rename(bone.get(), NP); });
	relToBone.if_value_set([this, _rename]() { relToBone = _rename(relToBone.get(), NP); });
}

void StoreBoneOffsetData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	storeVar.fill_value_with(_context);
	bone.fill_value_with(_context);
	relToBone.fill_value_with(_context);
}

void StoreBoneOffsetData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}
