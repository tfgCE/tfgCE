#include "objectType.h"

#include "object.h"

#include "..\library\library.h"

#include "..\module\modules.h"
#include "..\module\moduleDataImpl.inl"

using namespace Framework;

REGISTER_FOR_FAST_CAST(ObjectType);
LIBRARY_STORED_DEFINE_TYPE(ObjectType, objectType);

ObjectType::ObjectType(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
, individualityFactor(0.2f)
{
	// remember to call set_defaults (called from library)
}

ObjectType::~ObjectType()
{
}

void ObjectType::set_defaults()
{
	individualityFactor = 0.2f;
}

Object* ObjectType::create(String const & _name) const
{
	todo_important(TXT("are you sure? maybe it should be derived class?"));
	return nullptr;
}

bool ObjectType::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (! _node)
	{
		//TODO Utils.Log.error("invalid xml node when loading ObjectType");
		return false;
	}
	bool result = LibraryStored::load_from_xml(_node, _lc);

	subObject = _node->get_bool_attribute_or_from_child_presence(TXT("subObject"), subObject);

	advanceOnce = _node->get_bool_attribute_or_from_child_presence(TXT("advanceOnce"), advanceOnce);
	if (advanceOnce)
	{
		// unless told otherwise
		advancementSuspendable = false;
	}
	if (advancementSuspendable) // only if still allowed
	{
		advancementSuspendable = _node->get_bool_attribute_or_from_child_presence(TXT("advancementSuspendable"), advancementSuspendable);
		advancementSuspendable = !_node->get_bool_attribute_or_from_child_presence(TXT("advancementNotSuspendable"), !advancementSuspendable);
	}

	for_every(node, _node->children_named(TXT("wheresMyPointOnInit")))
	{
		result &= wheresMyPointProcessorOnInit.load_from_xml(node);
	}
	individualityFactor = _node->get_float_attribute(TXT("individuality"), individualityFactor);
	individualityFactor = _node->get_float_attribute(TXT("ind"), individualityFactor);
	result &= ai.load_from_xml(this, _node->first_child_named(TXT("ai")), _lc, Modules::ai);
	{
		int idx = 0;
		appearances.clear();
		for_every(node, _node->children_named(TXT("appearance")))
		{
			appearances.grow_size(1);
			result &= appearances[idx].load_from_xml(this, node, _lc, Modules::appearance);
			++idx;
		}
	}
	result &= collision.load_from_xml(this, _node->first_child_named(TXT("collision")), _lc, Modules::collision);
	result &= controller.load_from_xml(this, _node->first_child_named(TXT("controller")), _lc, Modules::controller);
	{
		int idx = 0;
		customs.clear();
		for_every(node, _node->children_named(TXT("custom")))
		{
			customs.grow_size(1);
			result &= customs[idx].load_from_xml(this, node, _lc, Modules::custom);
			++idx;
		}
	}
	gameplay.provide_default_type(_lc.get_library()->get_game()->get_customisation().library.defaultGameplayModuleTypeForObjectType);
	result &= gameplay.load_from_xml(this, _node->first_child_named(TXT("gameplay")), _lc, Modules::gameplay);
	{
		int idx = 0;
		movements.clear();
		for_every(node, _node->children_named(TXT("movement")))
		{
			movements.grow_size(1);
			result &= movements[idx].load_from_xml(this, node, _lc, Modules::movement);
			++idx;
		}
	}
	result &= navElement.load_from_xml(this, _node->first_child_named(TXT("navElement")), _lc, Modules::navElement);
	result &= presence.load_from_xml(this, _node->first_child_named(TXT("presence")), _lc, Modules::presence);
	result &= sound.load_from_xml(this, _node->first_child_named(TXT("sound")), _lc, Modules::sound);
	result &= temporaryObjects.load_from_xml(this, _node->first_child_named(TXT("temporaryObjects")), _lc, Modules::temporaryObjects);
	return true;
}

bool ObjectType::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	result &= LibraryStored::prepare_for_game(_library, _pfgContext);
	result &= ai.prepare_for_game(_library, _pfgContext);
	for_every(appearance, appearances)
	{
		result &= appearance->prepare_for_game(_library, _pfgContext);
	}
	result &= collision.prepare_for_game(_library, _pfgContext);
	result &= controller.prepare_for_game(_library, _pfgContext);
	for_every(custom, customs)
	{
		result &= custom->prepare_for_game(_library, _pfgContext);
	}
	result &= gameplay.prepare_for_game(_library, _pfgContext);
	for_every(movement, movements)
	{
		result &= movement->prepare_for_game(_library, _pfgContext);
	}
	result &= navElement.prepare_for_game(_library, _pfgContext);
	result &= presence.prepare_for_game(_library, _pfgContext);
	result &= sound.prepare_for_game(_library, _pfgContext);
	result &= temporaryObjects.prepare_for_game(_library, _pfgContext);
	return result;
}

void ObjectType::prepare_to_unload()
{
	base::prepare_to_unload();
	ai.prepare_to_unload();
	for_every(appearance, appearances)
	{
		appearance->prepare_to_unload();
	}
	collision.prepare_to_unload();
	controller.prepare_to_unload();
	for_every(custom, customs)
	{
		custom->prepare_to_unload();
	}
	gameplay.prepare_to_unload();
	for_every(movement, movements)
	{
		movement->prepare_to_unload();
	}
	navElement.prepare_to_unload();
	presence.prepare_to_unload();
	sound.prepare_to_unload();
	temporaryObjects.prepare_to_unload();
}

#ifdef AN_DEVELOPMENT
void ObjectType::ready_for_reload()
{
	base::ready_for_reload();

	wheresMyPointProcessorOnInit.clear();

	ai = ModuleDef<ModuleAI, ModuleAIData>(ModuleDefType::None);
	appearances.clear();
	collision = ModuleDef<ModuleCollision, ModuleCollisionData>();
	controller = ModuleDef<ModuleController, ModuleData>();
	customs.clear();
	gameplay = ModuleDef<ModuleGameplay, ModuleData>(ModuleDefType::None);
	movements.clear();
	presence = ModuleDef<ModulePresence, ModulePresenceData>();
	sound = ModuleDef<ModuleSound, ModuleSoundData>();
	temporaryObjects = ModuleDef<ModuleTemporaryObjects, ModuleTemporaryObjectsData>();
}
#endif

// TODO LibraryName solve_reference(Name _reference, Name _ofType) { return librarySolver.solve_reference(_reference, _ofType); }
