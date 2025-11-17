#pragma once

#include "..\..\..\core\math\math.h"
#include "..\..\library\libraryStored.h"
#include "..\..\text\localisedString.h"

namespace Framework
{
	class ColourClashSprite;
	class TexturePart;
	class Font;
};

#ifdef AN_MINIGAME_PLATFORMER
namespace Platformer
{
	class Character;
	class Game;
	class Room;

	class Info
	: public ::Framework::LibraryStored
	{
		FAST_CAST_DECLARE(Info);
		FAST_CAST_BASE(::Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef ::Framework::LibraryStored base;
	public:
		Info(::Framework::Library * _library, ::Framework::LibraryName const & _name);
		virtual ~Info();

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

	public:
		::Framework::LocalisedString itemsCollectedInfo;
		::Framework::LocalisedString timeInfo;
		::Framework::LocalisedString startGameInfo;

		bool unlimitedLives = false;
		int livesLimit = 8;

		int fps = 34;
		int gameFrameEvery = 3;

		VectorInt2 screenResolution = VectorInt2(256, 192);
		VectorInt2 tileSize = VectorInt2(8, 8);

		int deadlyHeightFall = 5;

		::Framework::UsedLibraryStored<::Framework::Font> font;

		::Framework::UsedLibraryStored<Character> playerCharacter;

		::Framework::UsedLibraryStored<Room> startsInRoom;
		VectorInt2 startsAtTile = VectorInt2(15, 4);

		::Framework::UsedLibraryStored<Room> titleScreenRoom;
		::Framework::UsedLibraryStored<Room> gameOverScreenRoom;
		::Framework::UsedLibraryStored<::Framework::ColourClashSprite> gameOverCupOfTeaSprite;
		::Framework::UsedLibraryStored<::Framework::ColourClashSprite> gameOverGeorgeFallingSprite;
		::Framework::UsedLibraryStored<::Framework::ColourClashSprite> gameOverGeorgeInACup0Sprite;
		::Framework::UsedLibraryStored<::Framework::ColourClashSprite> gameOverGeorgeInACup1Sprite;

		VectorInt2 renderRoomOffset = VectorInt2(0, 64);

		friend class Game;
	};
};
#endif
