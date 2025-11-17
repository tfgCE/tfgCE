#include "platformerInfo.h"

#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\video\colourClashSprite.h"
#include "..\..\video\texturePart.h"

#ifdef AN_MINIGAME_PLATFORMER

using namespace Platformer;

REGISTER_FOR_FAST_CAST(Info);

LIBRARY_STORED_DEFINE_TYPE(Info, minigamePlatformerInfo);

Info::Info(::Framework::Library * _library, ::Framework::LibraryName const & _name)
: base(_library, _name)
{
}

Info::~Info()
{
}

bool Info::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= itemsCollectedInfo.load_from_xml_child_required(_node, TXT("itemsCollectedInfo"), _lc, TXT("itemsCollectedInfo"));
	result &= timeInfo.load_from_xml_child_required(_node, TXT("timeInfo"), _lc, TXT("timeInfo"));
	result &= startGameInfo.load_from_xml_child_required(_node, TXT("startGameInfo"), _lc, TXT("startGameInfo"));

	if (_node->first_child_named(TXT("unlimitedLives")))
	{
		unlimitedLives = true;
	}
	unlimitedLives = _node->get_bool_attribute(TXT("unlimitedLives"), unlimitedLives);

	livesLimit = _node->get_int_attribute(TXT("livesLimit"), livesLimit);

	deadlyHeightFall = _node->get_int_attribute(TXT("deadlyHeightFall"), deadlyHeightFall);

	screenResolution.load_from_xml_child_node(_node, TXT("screenResolution"));
	tileSize.load_from_xml_child_node(_node, TXT("tileSize"));

	fps = _node->get_int_attribute(TXT("fps"), fps);
	fps = max(fps, 1);
	gameFrameEvery = _node->get_int_attribute(TXT("gameFrameEvery"), gameFrameEvery);
	gameFrameEvery = max(gameFrameEvery, 1);

	result &= font.load_from_xml(_node, TXT("font"), _lc);

	result &= playerCharacter.load_from_xml(_node, TXT("playerCharacter"), _lc);

	result &= startsInRoom.load_from_xml_child_node(_node, TXT("starts"), TXT("inRoom"), _lc);
	result &= startsAtTile.load_from_xml_child_node(_node, TXT("starts"));

	result &= titleScreenRoom.load_from_xml_child_node(_node, TXT("titleScreen"), TXT("room"), _lc);
	result &= gameOverScreenRoom.load_from_xml_child_node(_node, TXT("gameOverScreen"), TXT("room"), _lc);

	result &= gameOverCupOfTeaSprite.load_from_xml_child_node(_node, TXT("gameOverScreen"), TXT("cupOfTea"), _lc);
	result &= gameOverGeorgeFallingSprite.load_from_xml_child_node(_node, TXT("gameOverScreen"), TXT("georgeFalling"), _lc);
	result &= gameOverGeorgeInACup0Sprite.load_from_xml_child_node(_node, TXT("gameOverScreen"), TXT("georgeInACup0"), _lc);
	result &= gameOverGeorgeInACup1Sprite.load_from_xml_child_node(_node, TXT("gameOverScreen"), TXT("georgeInACup1"), _lc);

	result &= renderRoomOffset.load_from_xml_child_node(_node, TXT("renderRoomOffset"));
	
	return result;
}

bool Info::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= font.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);

	result &= playerCharacter.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);

	result &= startsInRoom.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);

	result &= titleScreenRoom.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);
	result &= gameOverScreenRoom.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);

	result &= gameOverCupOfTeaSprite.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);
	result &= gameOverGeorgeFallingSprite.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);
	result &= gameOverGeorgeInACup0Sprite.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);
	result &= gameOverGeorgeInACup1Sprite.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);

	return result;
}

#endif
