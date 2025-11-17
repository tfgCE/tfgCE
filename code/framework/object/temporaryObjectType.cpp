#include "temporaryObjectType.h"

#include "..\library\library.h"

#include "..\module\modules.h"
#include "..\module\moduleDataImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(TemporaryObjectType);
LIBRARY_STORED_DEFINE_TYPE(TemporaryObjectType, temporaryObjectType);

TemporaryObjectType::TemporaryObjectType(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
{
	// remember to call set_defaults (called from library)
}

TemporaryObjectType::~TemporaryObjectType()
{
}

void TemporaryObjectType::set_defaults()
{
}

bool TemporaryObjectType::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (! _node)
	{
		//TODO Utils.Log.error("invalid xml node when loading TemporaryObjectType");
		return false;
	}
	bool result = LibraryStored::load_from_xml(_node, _lc);
	precreateCount = _node->get_int_attribute_or_from_child(TXT("precreateCount"), precreateCount);
	noResetOnReuse = _node->get_bool_attribute_or_from_child_presence(TXT("noResetOnReuse"), noResetOnReuse);
	for_every(node, _node->children_named(TXT("wheresMyPointOnInit")))
	{
		result &= wheresMyPointProcessorOnInit.load_from_xml(node);
	}
	result &= ai.load_from_xml(this, _node->first_child_named(TXT("ai")), _lc, Modules::ai);
	{
		int idx = 0;
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
		for_every(node, _node->children_named(TXT("custom")))
		{
			customs.grow_size(1);
			result &= customs[idx].load_from_xml(this, node, _lc, Modules::custom);
			++idx;
		}
	}
	gameplay.provide_default_type(_lc.get_library()->get_game()->get_customisation().library.defaultGameplayModuleTypeForTemporaryObjectType);
	result &= gameplay.load_from_xml(this, _node->first_child_named(TXT("gameplay")), _lc, Modules::gameplay);
	{
		int idx = 0;
		for_every(node, _node->children_named(TXT("movement")))
		{
			movements.grow_size(1);
			result &= movements[idx].load_from_xml(this, node, _lc, Modules::movement);
			++idx;
		}
	}
	result &= presence.load_from_xml(this, _node->first_child_named(TXT("presence")), _lc, Modules::presence);
	result &= sound.load_from_xml(this, _node->first_child_named(TXT("sound")), _lc, Modules::sound);
	result &= companionTemporaryObjects.load_from_xml(this, _node->first_child_named(TXT("companionTemporaryObjects")), _lc, Modules::temporaryObjects);
	result &= temporaryObjects.load_from_xml(this, _node->first_child_named(TXT("temporaryObjects")), _lc, Modules::temporaryObjects);

	skipForNonRecentlyRenderedRoom = _node->get_bool_attribute_or_from_child_presence(TXT("skipForNonRecentlyRenderedRoom"), skipForNonRecentlyRenderedRoom);
	particlesBasedExistence = _node->get_bool_attribute_or_from_child_presence(TXT("particlesBasedExistence"), particlesBasedExistence);
	soundBasedExistence = _node->get_bool_attribute_or_from_child_presence(TXT("soundBasedExistence"), soundBasedExistence);
	lightSourcesBasedExistence = _node->get_bool_attribute_or_from_child_presence(TXT("lightSourcesBasedExistence"), lightSourcesBasedExistence);

	for_every(node, _node->children_named(TXT("spawnBlocking")))
	{
		spawnBlocking.dist.load_from_xml(node, TXT("dist"));
		spawnBlocking.time.load_from_xml(node, TXT("time"));
		spawnBlocking.count.load_from_xml(node, TXT("count"));
	}

	return true;
}

bool TemporaryObjectType::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
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
	result &= presence.prepare_for_game(_library, _pfgContext);
	result &= sound.prepare_for_game(_library, _pfgContext);
	result &= companionTemporaryObjects.prepare_for_game(_library, _pfgContext);
	result &= temporaryObjects.prepare_for_game(_library, _pfgContext);
	return result;
}

void TemporaryObjectType::prepare_to_unload()
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
	presence.prepare_to_unload();
	sound.prepare_to_unload();
	companionTemporaryObjects.prepare_to_unload();
	temporaryObjects.prepare_to_unload();
}

// TODO LibraryName solve_reference(Name _reference, Name _ofType) { return librarySolver.solve_reference(_reference, _ofType); }

#ifdef AN_DEVELOPMENT
void TemporaryObjectType::ready_for_reload()
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
	companionTemporaryObjects = ModuleDef<ModuleTemporaryObjects, ModuleTemporaryObjectsData>();
	temporaryObjects = ModuleDef<ModuleTemporaryObjects, ModuleTemporaryObjectsData>();
}
#endif

void TemporaryObjectType::log(LogInfoContext & _context) const
{
	base::log(_context);
	LOG_INDENT(_context);
	if (skipForNonRecentlyRenderedRoom)
	{
		_context.log(TXT("skip for non recently rendered room"));
	}
	if (particlesBasedExistence)
	{
		_context.log(TXT("particles based existence"));
	}
	else
	{
		_context.log(TXT("non particles based existence"));
	}
	if (soundBasedExistence)
	{
		_context.log(TXT("sound based existence"));
	}
	else
	{
		_context.log(TXT("non sound based existence"));
	}	
	if (lightSourcesBasedExistence)
	{
		_context.log(TXT("light sources based existence"));
	}
	else
	{
		_context.log(TXT("non light sources based existence"));
	}
}

bool TemporaryObjectType::get_spawn_blocking_info(OUT_ float& _blockingDist, OUT_ Optional<float>& _blockingTime, OUT_ Optional<int>& _blockingCount) const
{
	if (spawnBlocking.dist.is_set())
	{
		_blockingDist = spawnBlocking.dist.get();
		_blockingTime = spawnBlocking.time;
		_blockingCount = spawnBlocking.count;
		return true;
	}
	else
	{
		return false;
	}
}
