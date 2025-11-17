#pragma once

#include "..\library\libraryStored.h"
#include "..\library\usedLibraryStored.h"

namespace Framework
{
	class DoorInRoom;
	class DoorType;
	class DoorTypeReplacer;

	typedef RefCountObjectPtr<DoorTypeReplacer> DoorTypeReplacerPtr;

	struct DoorTypeReplacerContext
	{
		DoorTypeReplacerContext& is_world_separator(bool _isWorldSeparator = true) { isWorldSeparator = _isWorldSeparator; return *this; }

	private:
		bool isWorldSeparator = false;

		friend class DoorTypeReplacer;
	};

	/*
	 *	Door type replacer
	 *
	 *	Might be part of room type or separate object in library
	 *
	 *	Used in rooms to update/replace door types in doors
	 *
	 *	It is applied when the world is already generated
	 *
	 *	Each replacer handles separate door type (or may handle any)
	 *	Default operation, if nothing is defined is "disallow"
	 *	Operations:
	 *		allow (required by other's changeIfAllowed)
	 *		disallow (will stop any further processing)
	 *		changeIfAllowed (if other room allows to change, change door type to given)
	 *		changeIfBothAgree (if both rooms agree to change to specific door type, it is changed to given, useful for corridors)
	 *		forceChange (force)
	 *
	 *	Order of operations is important!
	 */
	class DoorTypeReplacer
	: public LibraryStored
	{
		FAST_CAST_DECLARE(DoorTypeReplacer);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		static DoorType* process(DoorType * _doorType, DoorTypeReplacerContext const & _context, DoorInRoom const * _aDIR, DoorTypeReplacer const * _a, DoorInRoom const* _bDIR, DoorTypeReplacer const * _b);

		DoorTypeReplacer();
		DoorTypeReplacer(Library * _library, LibraryName const & _name);

		int get_priority() const { return priority; }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ bool apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library);
		override_ void prepare_to_unload();

	private:
		struct Operation
		{
			enum Type
			{
				Allow,
				Disallow,
				ChangeIfAllowed,
				ChangeIfBothAgree,
				ForceChange,
			};
			Type type = Disallow;
			UsedLibraryStored<DoorType> doorType;
			TagCondition ifTagged; // if current/original door type tagged
			TagCondition ifActualDoorTagged; // checks combined tags for door and door in room for this door type replacer (does not include door type's tags although they may come basing on current door's door type via based_on)
			Optional<bool> isWorldSeparator;

			bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
			bool apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library);
		};

		struct For
		{
			UsedLibraryStored<DoorType> doorType;
			TagCondition tagged; // as above, just a group
			TagCondition actualDoorTagged; // as above, just a group
			Array<Operation> operations;

			bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
			bool apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library);

			bool process(REF_ DoorType * & _doorType, DoorTypeReplacerContext const& _context, DoorInRoom const * _thisDIR, For const * _other) const; // _thisDIR, door type replacer is for this door in room (side of the door)
		};
		
		int priority = 0;
		Array<For> forDoorTypes;

		For const * find_for(DoorType* _doorType, DoorInRoom const* _thisDIR) const;
		For const * find_default(DoorInRoom const* _thisDIR) const;

		void set(For const & _fdt);
	};
};
