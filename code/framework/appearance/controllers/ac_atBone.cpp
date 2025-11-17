#include "ac_atBone.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(atBone);

//

REGISTER_FOR_FAST_CAST(AtBone);

AtBone::AtBone(AtBoneData const * _data)
: base(_data)
, atBoneData(_data)
{
	bone = atBoneData->bone.get();
	atBone = atBoneData->atBone.get();
	locationOnly = atBoneData->locationOnly.get();
}

AtBone::~AtBone()
{
}

void AtBone::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	bone.look_up(get_owner()->get_skeleton()->get_skeleton());
	atBone.look_up(get_owner()->get_skeleton()->get_skeleton());
}

void AtBone::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void AtBone::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(atBone_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid() ||
		!atBone.is_valid())
	{
		return;
	}

	Meshes::Pose * poseLS = _context.access_pose_LS();

	int const boneIdx = bone.get_index();

	Transform boneMS = poseLS->get_bone_ms_from_ls(bone.get_index());
	Transform atBoneMS = poseLS->get_bone_ms_from_ls(atBone.get_index());

	if (locationOnly)
	{
		boneMS.set_translation(atBoneMS.get_translation());
	}
	else
	{
		boneMS = atBoneMS;
	}

	int const boneParentIdx = poseLS->get_skeleton()->get_parent_of(boneIdx);
	Transform const boneParentMS = poseLS->get_bone_ms_from_ls(boneParentIdx);
	poseLS->access_bones()[boneIdx] = boneParentMS.to_local(boneMS);
}

void AtBone::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (bone.is_valid())
	{
		dependsOnParentBones.push_back(bone.get_index());
		providesBones.push_back(bone.get_index());
	}
	if (atBone.is_valid())
	{
		dependsOnParentBones.push_back(atBone.get_index());
	}
}

//

REGISTER_FOR_FAST_CAST(AtBoneData);

AppearanceControllerData* AtBoneData::create_data()
{
	return new AtBoneData();
}

void AtBoneData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(atBone), create_data);
}

AtBoneData::AtBoneData()
{
}

bool AtBoneData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= atBone.load_from_xml(_node, TXT("atBone"));
	result &= locationOnly.load_from_xml(_node, TXT("locationOnly"));
	result &= locationOnly.load_from_xml(_node, TXT("placementOnly"));

	if (!bone.is_set())
	{
		error_loading_xml(_node, TXT("no bone provided"));
		result = false;
	}
	
	return result;
}

bool AtBoneData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* AtBoneData::create_copy() const
{
	AtBoneData* copy = new AtBoneData();
	*copy = *this;
	return copy;
}

AppearanceController* AtBoneData::create_controller()
{
	return new AtBone(this);
}

void AtBoneData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
	atBone.if_value_set([this, _rename]() { atBone = _rename(atBone.get(), NP); });
}

void AtBoneData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	atBone.fill_value_with(_context);
	locationOnly.fill_value_with(_context);
}

