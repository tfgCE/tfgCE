#include "moduleTemporaryObjectsData.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\random\randomUtils.h"
#include "..\..\core\system\core.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

bool ModuleTemporaryObjectsSingle::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	id = _node->get_name_attribute(TXT("id"));
	if (! id.is_valid())
	{
		error_loading_xml(_node, TXT("no id provided for temporary object in module TemporaryObjects data"));
		result = false;
	}
	tags.load_from_xml_attribute_or_child_node(_node, TXT("tags"));

	systemTagRequired.load_from_xml_attribute_or_child_node(_node, TXT("systemTagRequired"));
	result &= companionCount.load_from_xml(_node, TXT("companionCount"));

	socket = _node->get_name_attribute_or_from_child(TXT("socket"), socket);
	socketPrefix = _node->get_name_attribute_or_from_child(TXT("socketPrefix"), socketPrefix);
	bone = _node->get_name_attribute_or_from_child(TXT("bone"), bone);

	placement.load_from_xml_child_node(_node, TXT("placement"));
	randomDirAngle.load_from_xml(_node, TXT("randomDirAngle"));
	randomOffsetLocation.load_from_xml_child_node(_node, TXT("randomOffsetLocation"));
	randomOffsetRotation.load_from_xml_child_node(_node, TXT("randomOffsetRotation"));

	autoSpawn.load_from_xml(_node, TXT("autoSpawn"));
	if (_node->first_child_named(TXT("autoSpawn")))
	{
		autoSpawn = true;
	}
	if (_node->first_child_named(TXT("noAutoSpawn")))
	{
		autoSpawn = false;
	}
	if (auto* attr = _node->get_attribute(TXT("noAutoSpawn")))
	{
		autoSpawn = attr->get_as_bool();
	}
	spawnDetached = _node->get_bool_attribute_or_from_child_presence(TXT("spawnDetached"), spawnDetached);
	spawnDetached = _node->get_bool_attribute_or_from_child_presence(TXT("spawnInRoom"), spawnDetached);
	spawnDetached = _node->get_bool_attribute_or_from_child_presence(TXT("detached"), spawnDetached);

	result &= variables.load_from_xml_child_node(_node, TXT("parameters"));
	result &= variables.load_from_xml_child_node(_node, TXT("variables"));

	for_every(copyNode, _node->children_named(TXT("copyVariables")))
	{
		for_every(node, copyNode->children_named(TXT("copy")))
		{
			Name var = node->get_name_attribute(TXT("var"));
			if (var.is_valid())
			{
				copyVariables.push_back(var);
			}
			else
			{
				error_loading_xml(_node, TXT("no \"var\" attribute"));
				result = false;
			}
		}
	}
	for_every(node, _node->children_named(TXT("copy")))
	{
		warn_loading_xml(node, TXT("put into copyVariables"));
		Name var = node->get_name_attribute(TXT("var"));
		if (var.is_valid())
		{
			copyVariables.push_back(var);
		}
		else
		{
			error_loading_xml(_node, TXT("no \"var\" attribute"));
			result = false;
		}
	}

	{
		bool modulesAffectedByVariablesNodePresent = false;
		for_every(node, _node->children_named(TXT("modulesAffectedByVariables")))
		{
			modulesAffectedByVariablesNodePresent = true;
			for_every(mnode, node->children_named(TXT("module")))
			{
				Name var = mnode->get_name_attribute(TXT("type"));
				if (var.is_valid())
				{
					modulesAffectedByVariables.push_back(var);
				}
				else
				{
					error_loading_xml(_node, TXT("no \"type\" attribute"));
					result = false;
				}
			}
		}
		error_loading_xml_on_assert(modulesAffectedByVariablesNodePresent || (copyVariables.is_empty() && variables.is_empty()), result, _node, TXT("need to provide modulesAffectedByVariablesNodePresent even if empty if copyVariables and variables used"));
	}

	if (auto* attr = _node->get_attribute(TXT("type")))
	{
		Framework::UsedLibraryStored<Framework::TemporaryObjectType> tempType;
		if (tempType.load_from_xml(attr, _lc))
		{
			types.push_back(ModuleTemporaryObjectsSingle::Type(tempType));
		}
		else
		{
			error_loading_xml(_node, TXT("problem loading type from attribute"));
			result = false;
		}
	}

	for_every(node, _node->children_named(TXT("type")))
	{
		Framework::UsedLibraryStored<Framework::TemporaryObjectType> tempType;
		if (tempType.load_from_xml(node->get_attribute(TXT("type")), _lc))
		{
			types.push_back(ModuleTemporaryObjectsSingle::Type(tempType, node->get_float_attribute(TXT("probCoef"), 1.0f)));
		}
		else
		{
			error_loading_xml(node, TXT("problem loading type from child node"));
			result = false;
		}
	}

	if (types.is_empty())
	{
		error_loading_xml(_node, TXT("no type provided"));
		result = false;
	}

	return result;
}

bool ModuleTemporaryObjectsSingle::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	if (types.is_empty())
	{
		error(TXT("no type provided"));
		result = false;
	}

	for_every(type, types)
	{
		result &= type->type.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	return result;
}

Framework::TemporaryObjectType* ModuleTemporaryObjectsSingle::get_type(Random::Generator & rng) const
{
	int idx = RandomUtils::ChooseFromContainer< Array<ModuleTemporaryObjectsSingle::Type>, ModuleTemporaryObjectsSingle::Type>::choose(rng, types, [](ModuleTemporaryObjectsSingle::Type const & _type) { return _type.probCoef; });
	return types.is_index_valid(idx) ? types[idx].type.get() : nullptr;
}

Transform ModuleTemporaryObjectsSingle::ready_placement(OPTIONAL_ Random::Generator * _rg) const
{
	Transform p = placement;
	if (!randomOffsetLocation.is_empty())
	{
		Random::Generator& userg = _rg ? *_rg : rg;
		Vector3 offsetLocation;
		offsetLocation.x = userg.get(randomOffsetLocation.x);
		offsetLocation.y = userg.get(randomOffsetLocation.y);
		offsetLocation.z = userg.get(randomOffsetLocation.z);
		p.set_translation(p.location_to_world(offsetLocation));
	}
	if (!randomDirAngle.is_empty())
	{
		Random::Generator& userg = _rg ? *_rg : rg;
		Vector3 loc = p.get_translation();
		p.set_translation(Vector3::zero);
		p = offset_transform_by_forward_angle(p, REF_ userg, randomDirAngle);
		p.set_translation(loc);
	}
	if (!randomOffsetRotation.is_empty())
	{
		Random::Generator& userg = _rg ? *_rg : rg;
		Rotator3 offsetRotation;
		offsetRotation.pitch = userg.get(randomOffsetRotation.x);
		offsetRotation.roll = userg.get(randomOffsetRotation.y);
		offsetRotation.yaw = userg.get(randomOffsetRotation.z);
		p.set_orientation(p.get_orientation().to_world(offsetRotation.to_quat()));
	}
	return p;
}

//

REGISTER_FOR_FAST_CAST(ModuleTemporaryObjectsData);

ModuleTemporaryObjectsData::ModuleTemporaryObjectsData(LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool ModuleTemporaryObjectsData::read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("forceExistence"))
	{
		forceExistence = true;
		return true;
	}
	if (_node->get_name() == TXT("temporaryObject"))
	{
		TagCondition systemTagRequired;
		systemTagRequired.load_from_xml_attribute_or_child_node(_node, TXT("systemTagRequired"));
		if (systemTagRequired.check(System::Core::get_system_tags()))
		{
			Name id = _node->get_name_attribute(TXT("id"));
			if (id.is_valid())
			{
				RefCountObjectPtr<ModuleTemporaryObjectsSingle> to;
				to = new ModuleTemporaryObjectsSingle();
				if (to->load_from_xml(_node, _lc))
				{
					temporaryObjects.push_back(to);
					return true;
				}
				else
				{
					error_loading_xml(_node, TXT("invalid temporary object in module TemporaryObjects data"));
					return false;
				}
			}
			else
			{
				error_loading_xml(_node, TXT("no id provided for temporary object in module TemporaryObjects data"));
				return false;
			}
		}
		else
		{
			return true;
		}
	}
	else
	{
		return base::read_parameter_from(_node, _lc);
	}
}

bool ModuleTemporaryObjectsData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	if (_node)
	{
		forceExistence = true; // just because the node exists, force existence
	}

	return result;
}

bool ModuleTemporaryObjectsData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every_ref(temporaryObject, temporaryObjects)
	{
		result &= temporaryObject->prepare_for_game(_library, _pfgContext);
	}

	return result;
}
