#pragma once

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\types\colour.h"

#include "..\..\library\libraryStored.h"

struct VectorInt2;

namespace Framework
{
	class ColourClashSprite;
};

#ifdef AN_MINIGAME_PLATFORMER
namespace Platformer
{
	class Character
	: public ::Framework::LibraryStored
	{
		FAST_CAST_DECLARE(Character);
		FAST_CAST_BASE(::Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef ::Framework::LibraryStored base;
	public:
		Character(::Framework::Library * _library, ::Framework::LibraryName const & _name);
		virtual ~Character();

		::Framework::ColourClashSprite* get_sprite_at(VectorInt2 const & _loc, VectorInt2 const & _dir, int _frameNo);

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

	public:
		int width = NONE; // if none, is taken from current sprite
		int height = NONE; // if none, is taken from current sprite
		int step = 2; // how big is step - when it moves, calculates look etc
		VectorInt2 movementCollisionOffset = VectorInt2::zero; // player only!
		RangeInt2 movementCollisionBox = RangeInt2::zero; // player only!
		Array<int> jumpPattern;
		bool harmless = false;
		Array<::Framework::UsedLibraryStored<::Framework::ColourClashSprite>> leftDirSprites; // 0 at pixel 0, 1 at pixel -1 and so on
		Array<::Framework::UsedLibraryStored<::Framework::ColourClashSprite>> rightDirSprites; // 0 at pixel 0, 1 at pixel 1 and so on
		Array<::Framework::UsedLibraryStored<::Framework::ColourClashSprite>> downDirSprites;
		Array<::Framework::UsedLibraryStored<::Framework::ColourClashSprite>> upDirSprites;
		Array<::Framework::UsedLibraryStored<::Framework::ColourClashSprite>> noDirSprites;
		Optional<Colour> foregroundColourOverride;
		Optional<Colour> backgroundColourOverride;
		Optional<Colour> brightColourOverride;
	};
};
#endif
