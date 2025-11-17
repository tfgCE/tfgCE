#pragma once

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\types\colour.h"

#include "..\..\library\libraryStored.h"
#include "..\..\text\localisedString.h"

namespace Framework
{
	class ColourClashSprite;
};

#ifdef AN_MINIGAME_PLATFORMER
namespace Platformer
{
	class Game;
	class Room;

	class Tile
	: public ::Framework::LibraryStored
	{
		FAST_CAST_DECLARE(Tile);
		FAST_CAST_BASE(::Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef ::Framework::LibraryStored base;
	public:
		Tile(::Framework::Library * _library, ::Framework::LibraryName const & _name);
		virtual ~Tile();

		int calc_stairs_at(int _x, int _step, int _tileWidth) const;

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

	public:
		int stairsUp = 0; // 1 right, -1 left
		int autoMovement = 0;
		bool foreground = false;
		bool isItem = false;
		bool blocks = false;
		bool canStepOnto = false; 
		bool kills = false;

		int frameStep = 1;
		Array<::Framework::UsedLibraryStored<::Framework::ColourClashSprite>> frames;

		Optional<Colour> foregroundColourOverride;
		Optional<Colour> backgroundColourOverride;
		Optional<Colour> brightColourOverride;

		friend class Game;
		friend class Room;
	};
};
#endif
