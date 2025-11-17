#include "ac_applyBoneOffset.h"

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

DEFINE_STATIC_NAME(ApplyBoneOffset);

//

REGISTER_FOR_FAST_CAST(ApplyBoneOffset);

ApplyBoneOffset::ApplyBoneOffset(ApplyBoneOffsetData const * _data)
: base(_data)
, applyBoneOffsetData(_data)
{
	useVar = applyBoneOffsetData->useVar.get();

	bone = applyBoneOffsetData->bone.get(Name::invalid());
	relToBone = applyBoneOffsetData->relToBone.get(Name::invalid());
}

ApplyBoneOffset::~ApplyBoneOffset()
{
}

void ApplyBoneOffset::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	useVar.look_up<Transform>(varStorage);

	if (Meshes::Skeleton const * skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		bone.look_up(skeleton);
		relToBone.look_up(skeleton);
	}
}

void ApplyBoneOffset::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void ApplyBoneOffset::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(ApplyBoneOffset_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();
	if (!mesh)
	{
		return;
	}

	Meshes::Pose * poseLS = _context.access_pose_LS();

	int const boneParentIdx = poseLS->get_skeleton()->get_parent_of(bone.get_index());
	Transform parentBoneMS = poseLS->get_bone_ms_from_ls(boneParentIdx);
	Transform relToBoneMS = poseLS->get_bone_ms_from_ls(relToBone.get_index());

	Transform boneMS = relToBoneMS.to_world(useVar.get<Transform>());
	
	auto& boneLS = poseLS->access_bones()[bone.get_index()];
	Transform newBoneLS = parentBoneMS.to_local(boneMS);

	boneLS = lerp(get_active(), boneLS, newBoneLS);
}

void ApplyBoneOffset::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);
}

//

REGISTER_FOR_FAST_CAST(ApplyBoneOffsetData);

AppearanceControllerData* ApplyBoneOffsetData::create_data()
{
	return new ApplyBoneOffsetData();
}

void ApplyBoneOffsetData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(ApplyBoneOffset), create_data);
}

ApplyBoneOffsetData::ApplyBoneOffsetData()
{
}

bool ApplyBoneOffsetData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= useVar.load_from_xml(_node, TXT("useVarID"));

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= relToBone.load_from_xml(_node, TXT("relToBone"));

	return result;
}

bool ApplyBoneOffsetData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* ApplyBoneOffsetData::create_copy() const
{
	ApplyBoneOffsetData* copy = new ApplyBoneOffsetData();
	*copy = *this;
	return copy;
}

AppearanceController* ApplyBoneOffsetData::create_controller()
{
	return new ApplyBoneOffset(this);
}

void ApplyBoneOffsetData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	useVar.if_value_set([this, _rename]() { useVar = _rename(useVar.get(), NP); });
	bone.if_value_set([this, _rename]() { bone = _rename(bone.get(), NP); });
	relToBone.if_value_set([this, _rename]() { relToBone = _rename(relToBone.get(), NP); });
}

void ApplyBoneOffsetData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	useVar.fill_value_with(_context);
	bone.fill_value_with(_context);
	relToBone.fill_value_with(_context);
}

void ApplyBoneOffsetData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}
