#include "gameDirectorDefinition.h"

#include "..\story\storyChapter.h"
#include "..\story\storyScene.h"

#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

bool GameDirectorDefinition::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	{	// storyChapter
		result &= storyChapter.load_from_xml(_node, TXT("storyChapter"), _lc);
	}

	startWithBlockedDoors = _node->get_bool_attribute_or_from_child_presence(TXT("startWithBlockedDoors"), startWithBlockedDoors);

	for_every(node, _node->children_named(TXT("autoHostiles")))
	{
		result &= autoHostiles.load_from_xml(node);
	}

	return result;
}

bool GameDirectorDefinition::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= storyChapter.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

//

bool GameDirectorDefinition::AutoHostiles::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	fullHostilesChance.load_from_xml(_node, TXT("fullHostilesChance"));
	fullHostilesTimeInMinutes.load_from_xml(_node, TXT("fullHostilesTimeInMinutes"));
	normalHostilesTimeInMinutes.load_from_xml(_node, TXT("normalHostilesTimeInMinutes"));
	breakTimeInMinutes.load_from_xml(_node, TXT("breakTimeInMinutes"));

	return result;
}

void GameDirectorDefinition::AutoHostiles::override_with(AutoHostiles const& _ah)
{
	if (_ah.fullHostilesChance.is_set()) fullHostilesChance = _ah.fullHostilesChance;
	if (_ah.fullHostilesTimeInMinutes.is_set()) fullHostilesTimeInMinutes = _ah.fullHostilesTimeInMinutes;
	if (_ah.normalHostilesTimeInMinutes.is_set()) normalHostilesTimeInMinutes = _ah.normalHostilesTimeInMinutes;
	if (_ah.breakTimeInMinutes.is_set()) breakTimeInMinutes = _ah.breakTimeInMinutes;
}