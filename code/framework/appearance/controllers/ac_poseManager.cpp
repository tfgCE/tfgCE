#include "ac_poseManager.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

// default var values
DEFINE_STATIC_NAME(poseManager);
DEFINE_STATIC_NAME(poseManagerActivePoseId);
DEFINE_STATIC_NAME(poseManagerActivePoseActive);
DEFINE_STATIC_NAME(poseManagerActivePoseBlendTime);

//

REGISTER_FOR_FAST_CAST(PoseManager);

PoseManager::PoseManager(PoseManagerData const * _data)
: base(_data)
, SafeObject<PoseManager>(this)
{
	poses = _data->poses;

	if (_data->poseManagerPtrVar.is_set()) poseManagerPtrVar = _data->poseManagerPtrVar.get();
	if (_data->activePoseIdVar.is_set()) activePoseIdVar = _data->activePoseIdVar.get();
	if (_data->activePoseActiveVar.is_set()) activePoseActiveVar = _data->activePoseActiveVar.get();
	if (_data->activePoseBlendTimeVar.is_set()) activePoseBlendTimeVar = _data->activePoseBlendTimeVar.get();
}

PoseManager::~PoseManager()
{
	make_safe_object_unavailable();
}

void PoseManager::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto& varStorage = get_owner()->access_controllers_variable_storage();

	poseManagerPtrVar.look_up<::SafePtr<PoseManager>>(varStorage);
	activePoseIdVar.look_up<Name>(varStorage);
	activePoseActiveVar.look_up<float>(varStorage);
	activePoseBlendTimeVar.look_up<float>(varStorage);

	poseManagerPtrVar.access<::SafePtr<PoseManager>>() = this;

	for_every(pose, poses)
	{
		pose->activeVar.look_up<float>(varStorage);
		pose->targetActive = pose->active;
	}

	make_sure_current_pose_is_valid();
}

void PoseManager::make_sure_current_pose_is_valid()
{
	for_every(pose, poses)
	{
		for_every(element, pose->elements)
		{
			bool found = false;
			for_every(cpe, currentPose.elements)
			{
				if (cpe->id == element->id)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				currentPose.elements.push_back(*element);
				currentPose.elements.get_last().weight = 0.0f;
			}
		}
	}	
}

void PoseManager::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void PoseManager::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(atPoseManager_finalPose);
#endif

	float controllerActive = get_active();

	if (controllerActive <= 0.0f)
	{
		currentPose.elements.clear();
		return;
	}

	float deltaTime = _context.get_delta_time();

	Name activePoseId = activePoseIdVar.get<Name>();
	float activePoseActive = activePoseActiveVar.get<float>();
	float activePoseBlendTime = activePoseBlendTimeVar.get<float>();

	priorities.clear();
	for_every(p, poses)
	{
		if (activePoseId == p->id)
		{
			p->active = blend_to_using_time(p->active, activePoseActive, activePoseBlendTime, deltaTime);
		}
		else if (p->activeVar.is_valid())
		{
			p->active = blend_to_using_time(p->active, p->activeVar.get<float>(), activePoseBlendTime, deltaTime);
		}
		else
		{
			p->active = blend_to_using_time(p->active, 0.0f, activePoseBlendTime, deltaTime);
		}
		priorities.push_back_unique(p->priority);
	}

	sort(priorities);

	make_sure_current_pose_is_valid();

	for_every(e, currentPose.elements)
	{
		memory_zero(e->value, sizeof(e->value));
		float activesUsed = 0.0f;
		for_every_reverse(prio, priorities)
		{
			float activesHere = 0.0f;
			cachedElements.clear();
			for_every(p, poses)
			{
				if (p->priority == *prio &&
					p->active > 0.0f)
				{
					for_every(pe, p->elements)
					{
						if (pe->id == e->id)
						{
							activesHere += p->active;
							cachedElements.grow_size(1);
							auto& ce = cachedElements.get_last();
							ce.active = p->active;
							memory_copy(&ce.element, pe, sizeof(PoseManagerPose::Element));
						}
					}
				}
			}
			if (activesHere > 0.0f)
			{
				float activesAvailable = 1.0f - activesUsed;
				if (activesAvailable > 0.0f)
				{
					float useActive = (activesHere > activesAvailable ? activesAvailable / activesHere : 1.0f);

					for_every(ce, cachedElements)
					{
						float activeCE = useActive * ce->active;
						if (activeCE > 0.0f)
						{
							activesUsed += activeCE;
							float useCEPt = activeCE / activesUsed;
							if (e->valueType == PoseManagerPose::Element::Transform)
							{
								Transform v = ce->element.as_transform();
								if (ce->element.fromVar.is_valid())
								{
									v = get_owner()->access_controllers_variable_storage().get_value<Transform>(ce->element.fromVar, v);
								}
								if (e->as_transform().is_ok())
								{
									e->as_transform() = lerp(useCEPt, e->as_transform(), v);
								}
								else
								{
									e->as_transform() = v;
								}
							}
							else if (e->valueType == PoseManagerPose::Element::Vector3)
							{
								Vector3 v = ce->element.as_vector3();
								if (ce->element.fromVar.is_valid())
								{
									v = get_owner()->access_controllers_variable_storage().get_value<Vector3>(ce->element.fromVar, v);
								}
								e->as_vector3() = lerp(useCEPt, e->as_vector3(), v);
							}
							else if (e->valueType == PoseManagerPose::Element::Float)
							{
								float v = ce->element.as_float();
								if (ce->element.fromVar.is_valid())
								{
									v = get_owner()->access_controllers_variable_storage().get_value<float>(ce->element.fromVar, v);
								}
								e->as_float() = lerp(useCEPt, e->as_float(), v);
							}
						}
					}
				}
			}
			if (activesUsed >= 1.0f)
			{
				break;
			}
		}
		e->weight = activesUsed * controllerActive;
	}
}

PoseManagerPose::Element const* PoseManager::get_pose_value(Name const& _id) const
{
	for_every(c, currentPose.elements)
	{
		if (c->id == _id)
		{
			return c;
		}
	}

	return nullptr;
}

void PoseManager::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);
}

//

REGISTER_FOR_FAST_CAST(PoseManagerData);

AppearanceControllerData* PoseManagerData::create_data()
{
	return new PoseManagerData();
}

void PoseManagerData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(poseManager), create_data);
}

PoseManagerData::PoseManagerData()
{
}

bool PoseManagerData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	poseManagerPtrVar = NAME(poseManager);
	poseManagerPtrVar.load_from_xml(_node, TXT("poseManagerPtrVarID"));

	activePoseIdVar = NAME(poseManagerActivePoseId);
	activePoseIdVar.load_from_xml(_node, TXT("activePoseIdVarID"));

	activePoseActiveVar = NAME(poseManagerActivePoseActive);
	activePoseActiveVar.load_from_xml(_node, TXT("activePoseActiveVarID"));

	activePoseBlendTimeVar = NAME(poseManagerActivePoseBlendTime);
	activePoseBlendTimeVar.load_from_xml(_node, TXT("activePoseBlendTimeVarID"));

	for_every(node, _node->children_named(TXT("pose")))
	{
		PoseManagerPose pose;
		if (pose.load_from_xml(node))
		{
			poses.push_back(pose);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool PoseManagerData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* PoseManagerData::create_copy() const
{
	PoseManagerData* copy = new PoseManagerData();
	*copy = *this;
	return copy;
}

AppearanceController* PoseManagerData::create_controller()
{
	return new PoseManager(this);
}

void PoseManagerData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	poseManagerPtrVar.if_value_set([this, _rename]() { poseManagerPtrVar = _rename(poseManagerPtrVar.get(), NP); });
	activePoseIdVar.if_value_set([this, _rename]() { activePoseIdVar = _rename(activePoseIdVar.get(), NP); });
	activePoseActiveVar.if_value_set([this, _rename]() { activePoseActiveVar = _rename(activePoseActiveVar.get(), NP); });
	activePoseBlendTimeVar.if_value_set([this, _rename]() { activePoseBlendTimeVar = _rename(activePoseBlendTimeVar.get(), NP); });
}

void PoseManagerData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	poseManagerPtrVar.fill_value_with(_context);
	activePoseIdVar.fill_value_with(_context);
	activePoseActiveVar.fill_value_with(_context);
	activePoseBlendTimeVar.fill_value_with(_context);
}

//

bool PoseManagerPose::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	id = _node->get_name_attribute(TXT("id"), id);
	priority = _node->get_int_attribute(TXT("priority"), priority);
	active = _node->get_float_attribute(TXT("active"), active);
	activeVar.load_from_xml(_node, TXT("activeVarID"));

	for_every(node, _node->all_children())
	{
		if (node->get_name() == TXT("toWorldOf"))
		{
			Transform t = Transform::identity;
			t.load_from_xml(node);
			for_every(e, elements)
			{
				if (e->valueType == Element::Vector3)
				{
					if (e->isDir.get(true))
					{
						e->as_vector3() = t.vector_to_world(e->as_vector3());
					}
					else
					{
						e->as_vector3() = t.location_to_world(e->as_vector3());
					}
				}
				if (e->valueType == Element::Transform)
				{
					e->as_transform() = t.to_world(e->as_transform());
				}
			}
		}
		else if (node->get_name() == TXT("toLocalOf"))
		{
			Transform t = Transform::identity;
			t.load_from_xml(node);
			for_every(e, elements)
			{
				if (e->valueType == Element::Vector3)
				{
					if (e->isDir.get(true))
					{
						e->as_vector3() = t.vector_to_local(e->as_vector3());
					}
					else
					{
						e->as_vector3() = t.location_to_world(e->as_vector3());
					}
				}
				if (e->valueType == Element::Transform)
				{
					e->as_transform() = t.to_local(e->as_transform());
				}
			}
		}
		else
		{
			Element e;
			if (e.load_from_xml(node))
			{
				elements.push_back(e);
			}
			else
			{
				result = false;
			}
		}
	}

	return result;
}

//

bool PoseManagerPose::Element::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	id = _node->get_name_attribute_or_from_child(TXT("id"), id);
	fromVar = _node->get_name_attribute_or_from_child(TXT("fromVar"), fromVar);

	if (_node->get_name() == TXT("transform"))
	{
		valueType = Transform;
		as_transform() = ::Transform::identity;
		as_transform().load_from_xml(_node);
	}
	else if (_node->get_name() == TXT("vector3"))
	{
		valueType = Vector3;
		as_vector3() = ::Vector3::zero;
		as_vector3().load_from_xml(_node);
	}
	else if (_node->get_name() == TXT("vector3Dir"))
	{
		valueType = Vector3;
		as_vector3() = ::Vector3::zero;
		as_vector3().load_from_xml(_node);
		isDir = true;
	}
	else if (_node->get_name() == TXT("vector3Loc"))
	{
		valueType = Vector3;
		as_vector3() = ::Vector3::zero;
		as_vector3().load_from_xml(_node);
		isDir = false;
	}
	else if (_node->get_name() == TXT("float"))
	{
		valueType = Float;
		as_float() = 0.0f;
		as_float() = _node->get_float(0.0f);
	}
	else
	{
		valueType = Unknown;
		error_loading_xml(_node, TXT("type not recognised or not implemented"));
		result = false;
	}
	return result;
}

