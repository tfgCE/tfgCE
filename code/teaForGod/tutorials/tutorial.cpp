#include "tutorial.h"

#include "..\game\game.h"
#include "..\library\library.h"
#include "..\tutorials\tutorialSystem.h"

#include "..\..\framework\gameScript\gameScript.h"
#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\object\actor.h"

// hub scenes
#include "..\loaders\hub\scenes\lhs_inGameMenu.h"

//

using namespace TeaForGodEmperor;

//

// loadout "what to open"
DEFINE_STATIC_NAME(main);
DEFINE_STATIC_NAME(fad);
DEFINE_STATIC_NAME(exm);
DEFINE_STATIC_NAME(weapon);

//

TutorialType::Type TutorialType::parse(String const& _t, Type _default)
{
	if (_t == TXT("world")) return World;
	if (_t == TXT("hub")) return Hub;

	if (!_t.is_empty())
	{
		error(TXT("tutorial type \"%S\" not recognised"), _t.to_char());
	}
	return _default;
}

//

REGISTER_FOR_FAST_CAST(Tutorial);
LIBRARY_STORED_DEFINE_TYPE(Tutorial, tutorial);

Tutorial::Tutorial(Framework::Library* _library, Framework::LibraryName const& _name)
: base(_library, _name)
, pilgrimSetup(&persistence)
{
}

Tutorial::~Tutorial()
{
}

Framework::GameScript::Script const* Tutorial::get_game_script() const
{
	return gameScript.get();
}

bool Tutorial::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= load_as_tutorial_tree_element_from_xml_child_node(_node, _lc);

	type = TutorialType::parse(_node->get_string_attribute(TXT("type")), TutorialType::World);

	regionType.load_from_xml(_node, TXT("regionType"), _lc);
	roomType.load_from_xml(_node, TXT("roomType"), _lc);
	playerSpawnPOI = _node->get_name_attribute(TXT("playerSpawnPOI"), playerSpawnPOI);
	
	hubScene = _node->get_name_attribute(TXT("hubScene"), hubScene);

	variables.load_from_xml_child_node(_node, TXT("variables"));
	gameScript.load_from_xml(_node, TXT("gameScript"), _lc);

	result &= pilgrimSetup.load_from_xml_child_node(_node, TXT("pilgrimSetup"));
	result &= persistence.load_from_xml_child_node(_node, TXT("persistence"));
	persistence.resolve_library_links();

	autoSetupPersistence = _node->get_bool_attribute_or_from_child_presence(TXT("autoSetupPersistence"), autoSetupPersistence);

	if (type == TutorialType::World)
	{
		error_loading_xml_on_assert(regionType.is_name_valid(), result, _node, TXT("world based tutorial requires \"regionType\""));
		error_loading_xml_on_assert(playerSpawnPOI.is_valid(), result, _node, TXT("world based tutorial requires \"playerSpawnPOI\""));
	}

	error_loading_xml_on_assert(gameScript.is_name_valid(), result, _node, TXT("world based tutorial requires \"playerSpawnPOI\""));

	return result;
}

bool Tutorial::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= regionType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= roomType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	
	result &= gameScript.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	IF_PREPARE_LEVEL(_pfgContext, TeaForGodEmperorLibraryPrepareLevel::Resolve)
	{
		persistence.resolve_library_links();
	}

	if (autoSetupPersistence)
	{
		IF_PREPARE_LEVEL(_pfgContext, TeaForGodEmperorLibraryPrepareLevel::PreparePersistence)
		{
			persistence.setup_auto();
		}
	}

	return result;
}

Loader::HubScene* Tutorial::create_hub_scene(Loader::Hub* _hub) const
{
	Framework::IModulesOwner* playerActor = nullptr;
	if (auto* g = Game::get_as<Game>())
	{
		playerActor = g->access_player().get_actor();
	}
	if (hubScene == TXT("inGameMenu")) return new Loader::HubScenes::InGameMenu();

	error(TXT("can't create hub scene \"%S\", I don't know how"), hubScene.to_char());
	return nullptr;
}
