#pragma once

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\types\colour.h"

#include "..\..\library\libraryStored.h"
#include "..\..\text\localisedString.h"

namespace Framework
{
	class TexturePart;
};

#ifdef AN_MINIGAME_PLATFORMER
namespace Platformer
{
	class Character;
	class Game;
	class Tile;
	class Info;

	namespace DoorDir
	{
		enum Type
		{
			None,
			Left,
			Right,
			Up,
			Down
		};

		inline Type get_opposite(Type _dir)
		{
			if (_dir == DoorDir::Left) return DoorDir::Right;
			if (_dir == DoorDir::Right) return DoorDir::Left;
			if (_dir == DoorDir::Down) return DoorDir::Up;
			if (_dir == DoorDir::Up) return DoorDir::Down;
			return DoorDir::None;
		}
	};

	/**
	 *	. empty space
	 *
	 *	@0	door 0 (always upper)
	 *	@}	going right
	 *
	 */
	class Room
	: public ::Framework::LibraryStored
	{
		FAST_CAST_DECLARE(Room);
		FAST_CAST_BASE(::Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef ::Framework::LibraryStored base;
	public:
		Room(::Framework::Library * _library, ::Framework::LibraryName const & _name);
		virtual ~Room();

		void get_items_at(Info* _info, RangeInt2 const & rect, OUT_ ArrayStack<int> & _itemsIdx) const;
		bool is_location_valid(Info* _info, RangeInt2 const & rect, RangeInt2 const & _ignore = RangeInt2::empty) const;
		bool does_kill(Info* _info, RangeInt2 const & rect) const;
		bool can_fall_onto(Info* _info, RangeInt2 const & rect, int _dir, OUT_ int & _atY, VectorInt2 const & _at, VectorInt2 const & _offsetForStairs, RangeInt const & _feetForStairs, int _step, OUT_ bool & _ontoStairs) const;
		bool can_step_onto_except_stairs(Info* _info, RangeInt2 const & rect) const;
		bool has_something_under_feet(Info* _info, RangeInt2 const & rect, VectorInt2 const & _at, VectorInt2 const & _offsetForStairs, RangeInt const & _feetForStairs, int _step) const;
		int get_auto_movement_dir(Info* _info, RangeInt2 const & rect) const;
		int get_offset_on_stairs(Info* _info, VectorInt2 const & _at, VectorInt2 const & _offsetForStairs, RangeInt const & _feetForStairs, int _dir, int _step) const;

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);
		override_ void prepare_to_unload();

	public:
		struct Door
		{
			RangeInt2 rect = RangeInt2::empty;
			::Framework::UsedLibraryStored<Room> connectsTo;
			::Framework::UsedLibraryStored<Tile> tile;
			int connectsToDoor = NONE;
			DoorDir::Type dir = DoorDir::None;
			bool autoDoor = false;
		};
		struct TileRef
		{
			tchar ch = '#';
			::Framework::UsedLibraryStored<Tile> tile;
		};
		struct Guardian
		{
			RangeInt2 rect = RangeInt2::empty;
			::Framework::UsedLibraryStored<Character> character;
			VectorInt2 startingLoc = VectorInt2(NONE, NONE);
			VectorInt2 startingDir = VectorInt2::zero;
		};

		Colour borderColour = Colour::red;
		Colour backgroundColour = Colour::black;
		Colour foregroundColour = Colour::white;
		Colour brightnessIndicator = Colour::black;

		Array<TileRef> tileRefs;

		Array<Door> doors;
		Array<VectorInt2> itemsAt;

		Array<Guardian> guardians;

		VectorInt2 size = VectorInt2(32, 16);
		::Framework::LocalisedString roomName;
		Array<int> tiles; // continuous array by rows (0,0 left bottom)

		int find_tile_by_char(tchar _ch) const;
		Tile const * get_tile_at(VectorInt2 const & _at) const;

		Door * find_door_at(VectorInt2 const & _at);

		friend class Game;
	};
};
#endif
