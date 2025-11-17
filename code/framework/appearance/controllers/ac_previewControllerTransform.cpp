#include "ac_previewControllerTransform.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\moduleGameplayPreview.h"

#include "..\..\..\core\debug\debugRenderer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(previewControllerTransform);

//

REGISTER_FOR_FAST_CAST(AppearanceControllersLib::PreviewControllerTransform);

AppearanceControllersLib::PreviewControllerTransform::PreviewControllerTransform(PreviewControllerTransformData const * _data)
: base(_data)
{
	for_every(cpct, _data->copyControllerTransforms)
	{
		CopyPreviewControllerTransformInstance cpcti;
		cpcti.controllerTransformId = cpct->controllerTransformId.get();
		if (cpct->toBone.is_set())
		{
			cpcti.toBone = cpct->toBone.get();
		}
		if (cpct->toMSVar.is_set())
		{
			cpcti.toMSVar.set_name(cpct->toMSVar.get());
		}
		copyControllerTransforms.push_back(cpcti);
	}
}

AppearanceControllersLib::PreviewControllerTransform::~PreviewControllerTransform()
{
}

void AppearanceControllersLib::PreviewControllerTransform::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto& varStorage = get_owner()->access_controllers_variable_storage();
	for_every(cpct, copyControllerTransforms)
	{
		if (cpct->toMSVar.get_name().is_valid())
		{
			cpct->toMSVar.look_up<Transform>(varStorage);
		}
	}
}

void AppearanceControllersLib::PreviewControllerTransform::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	Meshes::Pose * poseLS = _context.access_pose_LS();

	if (ModuleAppearance const * appearance = get_owner())
	{
		if (IModulesOwner const * owner = appearance->get_owner())
		{
			if (appearance->get_skeleton() &&
				appearance->get_skeleton()->get_skeleton())
			{
				debug_context(owner->get_presence()->get_in_room());
				debug_push_transform(owner->get_presence()->get_placement());

				if (auto* gp = owner->get_gameplay_as<ModuleGameplayPreview>())
				{
					auto* skeleton = appearance->get_skeleton()->get_skeleton();
					for_every(cpct, copyControllerTransforms)
					{
						int ctIdx = gp->find_controller_transform_index(cpct->controllerTransformId);
						Transform ctMS = gp->get_controller_transform(ctIdx);
						if (cpct->toBone.is_valid())
						{
							int boneIdx = skeleton->find_bone_index(cpct->toBone);
							int parentBoneIdx = skeleton->get_parent_of(boneIdx);
							if (ctIdx != NONE && boneIdx != NONE && parentBoneIdx != NONE)
							{
								Transform parentMS = poseLS->get_bone_ms_from_ls(parentBoneIdx);
								poseLS->set_bone(boneIdx, parentMS.to_local(ctMS));
#ifdef AN_DEVELOPMENT
								Transform ctOS = appearance->get_ms_to_os_transform().to_world(ctMS);
								debug_draw_transform_size(true, ctOS, 0.05f);
#endif
							}
						}
						if (cpct->toMSVar.is_valid())
						{
							cpct->toMSVar.access<Transform>() = ctMS;
						}
					}
				}

#ifndef AN_CLANG
				an_assert(poseLS->is_ok());
#endif
				debug_pop_transform();
				debug_no_context();
			}
		}
	}
}

//

REGISTER_FOR_FAST_CAST(PreviewControllerTransformData);

AppearanceControllerData* PreviewControllerTransformData::create_data()
{
	return new PreviewControllerTransformData();
}

void PreviewControllerTransformData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(previewControllerTransform), create_data);
}

PreviewControllerTransformData::PreviewControllerTransformData()
{
}

bool PreviewControllerTransformData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("copy")))
	{
		CopyPreviewControllerTransform cpct;
		cpct.controllerTransformId.load_from_xml(node, TXT("controllerTransformId"));
		cpct.toBone.load_from_xml(node, TXT("toBone"));
		cpct.toMSVar.load_from_xml(node, TXT("toMSVarID"));
		copyControllerTransforms.push_back(cpct);
	}

	return result;
}

AppearanceControllerData* PreviewControllerTransformData::create_copy() const
{
	PreviewControllerTransformData* copy = new PreviewControllerTransformData();
	*copy = *this;
	return copy;
}

AppearanceController* PreviewControllerTransformData::create_controller()
{
	return new PreviewControllerTransform(this);
}

void PreviewControllerTransformData::apply_mesh_gen_params(MeshGeneration::GenerationContext const& _context)
{
	base::apply_mesh_gen_params(_context);

	for_every(cpct, copyControllerTransforms)
	{
		cpct->controllerTransformId.fill_value_with(_context);
		cpct->toBone.fill_value_with(_context);
		cpct->toMSVar.fill_value_with(_context);
	}
}
