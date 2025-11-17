#include "ac_poseInject.h"

#include "ac_poseManager.h"

#include "ikUtils.h"

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

DEFINE_STATIC_NAME(poseInject);

// default var values
DEFINE_STATIC_NAME(poseManager);
DEFINE_STATIC_NAME(activePoseId);
DEFINE_STATIC_NAME(activePoseActive);
DEFINE_STATIC_NAME(activePoseBlendTime);

//

REGISTER_FOR_FAST_CAST(PoseInject);

PoseInject::PoseInject(PoseInjectData const * _data)
: base(_data)
{
	elements = _data->elements;

	if (_data->poseManagerPtrVar.is_set()) poseManagerPtrVar = _data->poseManagerPtrVar.get();
}

PoseInject::~PoseInject()
{
}

void PoseInject::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto& varStorage = get_owner()->access_controllers_variable_storage();

	poseManagerPtrVar.look_up<::SafePtr<PoseManager>>(varStorage);
	
	Meshes::Skeleton const* skel = get_owner()->get_skeleton()->get_skeleton();

	for_every(e, elements)
	{
		if (e->inject == PoseInjectElement::AsBone)
		{
			e->asBone.set_name(e->id);
			e->asBone.look_up(skel);
			e->parentBone = Meshes::BoneID(skel, skel->get_parent_of(e->asBone.get_index()));
		}
		else if (e->inject == PoseInjectElement::AsTransformVar)
		{
			e->asVariable.set_name(e->id);
			e->asVariable.look_up<Transform>(varStorage);
		}
		else if (e->inject == PoseInjectElement::AsVector3Var)
		{
			e->asVariable.set_name(e->id);
			e->asVariable.look_up<Vector3>(varStorage);
		}
		else if (e->inject == PoseInjectElement::AsFloatVar)
		{
			e->asVariable.set_name(e->id);
			e->asVariable.look_up<float>(varStorage);
		}
	}
}

void PoseInject::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void PoseInject::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(atPoseInject_finalPose);
#endif

	float controllerActive = get_active();

	if (controllerActive <= 0.0f)
	{
		return;
	}

	Meshes::Pose* poseLS = _context.access_pose_LS();

	if (auto* poseManager = poseManagerPtrVar.get<::SafePtr<PoseManager>>().get())
	{
		for_every(e, elements)
		{
			if (auto* v = poseManager->get_pose_value(e->from))
			{
				if (e->inject == PoseInjectElement::AsBone)
				{
					if (v->valueType != PoseManagerPose::Element::Transform)
					{
						error(TXT("invalid value type for bone"));
						continue;
					}
					if (e->asBone.get_index() != NONE)
					{
						Transform parentMS = poseLS->get_bone_ms_from_ls(e->parentBone.get_index());
						Transform boneMS = parentMS.to_world(poseLS->get_bone(e->asBone.get_index()));
						boneMS = lerp(v->weight * controllerActive, boneMS, v->as_transform());
						poseLS->access_bones()[e->asBone.get_index()] = parentMS.to_local(boneMS);
					}
				}
				else if (e->inject == PoseInjectElement::AsTransformVar)
				{
					if (v->valueType != PoseManagerPose::Element::Transform)
					{
						error(TXT("invalid value type for transform var"));
						continue;
					}
					auto& var = e->asVariable.access<Transform>();
					var = lerp(v->weight * controllerActive, var, v->as_transform());
				}
				else if (e->inject == PoseInjectElement::AsVector3Var)
				{
					if (v->valueType != PoseManagerPose::Element::Vector3)
					{
						error(TXT("invalid value type for transform var"));
						continue;
					}
					auto& var = e->asVariable.access<Vector3>();
					var = lerp(v->weight * controllerActive, var, v->as_vector3());
				}
				else if (e->inject == PoseInjectElement::AsFloatVar)
				{
					if (v->valueType != PoseManagerPose::Element::Float)
					{
						error(TXT("invalid value type for transform var"));
						continue;
					}
					auto& var = e->asVariable.access<float>();
					var = lerp(v->weight * controllerActive, var, v->as_float());
				}
			}
		}
	}
}

void PoseInject::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);
}

//

REGISTER_FOR_FAST_CAST(PoseInjectData);

AppearanceControllerData* PoseInjectData::create_data()
{
	return new PoseInjectData();
}

void PoseInjectData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(poseInject), create_data);
}

PoseInjectData::PoseInjectData()
{
}

bool PoseInjectData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	poseManagerPtrVar = NAME(poseManager);
	poseManagerPtrVar.load_from_xml(_node, TXT("poseManagerPtrVarID"));

	for_every(node, _node->children_named(TXT("inject")))
	{
		PoseInjectElement e;
		if (e.load_from_xml(node))
		{
			elements.push_back(e);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool PoseInjectData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* PoseInjectData::create_copy() const
{
	PoseInjectData* copy = new PoseInjectData();
	*copy = *this;
	return copy;
}

AppearanceController* PoseInjectData::create_controller()
{
	return new PoseInject(this);
}

void PoseInjectData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	poseManagerPtrVar.if_value_set([this, _rename]() { poseManagerPtrVar = _rename(poseManagerPtrVar.get(), NP); });
}

void PoseInjectData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	poseManagerPtrVar.fill_value_with(_context);
}

//

bool PoseInjectElement::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	if (auto* attr = _node->get_attribute(TXT("transformVar")))
	{
		id = attr->get_as_name();
		from = id;
		inject = AsTransformVar;
	}
	else if (auto* attr = _node->get_attribute(TXT("vector3Var")))
	{
		id = attr->get_as_name();
		from = id;
		inject = AsVector3Var;
	}
	else if (auto* attr = _node->get_attribute(TXT("floatVar")))
	{
		id = attr->get_as_name();
		from = id;
		inject = AsFloatVar;
	}
	else if (auto* attr = _node->get_attribute(TXT("bone")))
	{
		id = attr->get_as_name();
		from = id;
		inject = AsBone;
	}

	from = _node->get_name_attribute(TXT("from"), from);

	return result;
}
