#include "acc_keepChildrenWhereTheyWere.h"

#include "..\..\appearanceControllerPoseContext.h"

#include "..\..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\..\module\moduleAppearance.h"

//

using namespace Framework;
using namespace AppearanceControllersLib;
using namespace Components;

//

KeepChildrenWhereTheyWere::KeepChildrenWhereTheyWere(KeepChildrenWhereTheyWereData const & _data)
{
	if (_data.keepChildrenWhereTheyWere.is_set())
	{
		keepChildrenWhereTheyWere = _data.keepChildrenWhereTheyWere.get();
	}
}

void KeepChildrenWhereTheyWere::initialise(ModuleAppearance* _owner, Meshes::BoneID const & _bone)
{
	bone = _bone;
	childrenBones.clear();
	if (auto* skeleton = _owner->get_skeleton()->get_skeleton())
	{
		if (bone.get_index() != NONE)
		{
			if (keepChildrenWhereTheyWere)
			{
				for_every(b, skeleton->get_bones())
				{
					if (b->get_parent_index() == bone.get_index())
					{
						childrenBones.push_back(Meshes::BoneID(skeleton, for_everys_index(b)));
					}
				}
			}
		}
	}
}

void KeepChildrenWhereTheyWere::preform_pre_bone_changes(REF_ AppearanceControllerPoseContext& _context)
{
	if (bone.is_valid() && keepChildrenWhereTheyWere)
	{
		Meshes::Pose* poseLS = _context.access_pose_LS();
		preChangesBoneMS = poseLS->get_bone_ms_from_ls(bone.get_index());
	}
}

void KeepChildrenWhereTheyWere::preform_post_bone_changes(REF_ AppearanceControllerPoseContext& _context)
{
	if (bone.is_valid() && keepChildrenWhereTheyWere)
	{
		Meshes::Pose* poseLS = _context.access_pose_LS();
		Transform postChangesBoneMS = poseLS->get_bone_ms_from_ls(bone.get_index());
		for_every(cb, childrenBones)
		{
			auto& boneLS = poseLS->access_bones()[cb->get_index()];
			Transform childBoneMS = preChangesBoneMS.to_world(boneLS);
			boneLS = postChangesBoneMS.to_local(childBoneMS);
		}
	}
}

//

bool KeepChildrenWhereTheyWereData::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	result &= keepChildrenWhereTheyWere.load_from_xml(_node, TXT("keepChildrenWhereTheyWere"));

	return result;
}

void KeepChildrenWhereTheyWereData::apply_mesh_gen_params(MeshGeneration::GenerationContext const& _context)
{
	keepChildrenWhereTheyWere.fill_value_with(_context);
}
