#include "ac_reorientate.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\moduleSound.h"

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

DEFINE_STATIC_NAME(reorientate);

//

REGISTER_FOR_FAST_CAST(Reorientate);

Reorientate::Reorientate(ReorientateData const * _data)
: base(_data)
, reorientateData(_data)
{
	bone = reorientateData->bone.get();
	applyTransform = Transform::identity;
	if (reorientateData->orientationFromBone.is_set())
	{
		orientationFromBone = reorientateData->orientationFromBone.get();
	}
	if (reorientateData->rotate.is_set())
	{
		applyTransform.set_orientation(reorientateData->rotate.get().to_quat());
	}
}

Reorientate::~Reorientate()
{
}

void Reorientate::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	childrenBones.clear();
	if (auto* skel = get_owner()->get_skeleton()->get_skeleton())
	{
		bone.look_up(skel);
		orientationFromBone.look_up(skel);
		parentBone = Meshes::BoneID(skel, skel->get_parent_of(bone.get_index()));
		for_every(b, skel->get_bones())
		{
			if (b->get_parent_index() == bone.get_index())
			{
				childrenBones.push_back(Meshes::BoneID(skel, for_everys_index(b)));
			}
		}
	}
}

void Reorientate::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void Reorientate::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(reorientate_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid() ||
		get_active() == 0.0f)
	{
		return;
	}

	Meshes::Pose * poseLS = _context.access_pose_LS();

	int const boneIdx = bone.get_index();

	Transform const parentMS = poseLS->get_bone_ms_from_ls(parentBone.get_index());
	auto const boneMS = parentMS.to_world(poseLS->get_bones()[boneIdx]);

	Transform newBoneMS = boneMS.to_world(applyTransform);

	if (orientationFromBone.is_valid())
	{
		auto otherBoneMS = poseLS->get_bone_ms_from_ls(orientationFromBone.get_index());
		newBoneMS.set_orientation(otherBoneMS.get_orientation());
	}

	{
		auto& boneLS = poseLS->access_bones()[boneIdx];
		boneLS = parentMS.to_local(newBoneMS);
	}

	for_every(cb, childrenBones)
	{
		auto& boneLS = poseLS->access_bones()[cb->get_index()];
		Transform childBoneMS = boneMS.to_world(boneLS);
		boneLS = newBoneMS.to_local(childBoneMS);
	}
}

void Reorientate::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (bone.is_valid())
	{
		dependsOnParentBones.push_back(bone.get_index());
		providesBones.push_back(bone.get_index());
	}
	if (orientationFromBone.is_valid())
	{
		dependsOnBones.push_back(orientationFromBone.get_index());
	}
	for_every(cb, childrenBones)
	{
		providesBones.push_back(cb->get_index());
	}
}

//

REGISTER_FOR_FAST_CAST(ReorientateData);

AppearanceControllerData* ReorientateData::create_data()
{
	return new ReorientateData();
}

void ReorientateData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(reorientate), create_data);
}

ReorientateData::ReorientateData()
{
}

bool ReorientateData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= orientationFromBone.load_from_xml(_node, TXT("orientationFromBone"));
	result &= orientationFromBone.load_from_xml(_node, TXT("orientationAsBone"));
	result &= rotate.load_from_xml_child_node(_node, TXT("rotate"));

	if (!bone.is_set())
	{
		error_loading_xml(_node, TXT("no bone provided"));
		result = false;
	}

	return result;
}

bool ReorientateData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* ReorientateData::create_copy() const
{
	ReorientateData* copy = new ReorientateData();
	*copy = *this;
	return copy;
}

AppearanceController* ReorientateData::create_controller()
{
	return new Reorientate(this);
}

void ReorientateData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
}

void ReorientateData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	orientationFromBone.fill_value_with(_context);
	rotate.fill_value_with(_context);
}

