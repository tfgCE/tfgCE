#pragma once

#include "..\library\usedLibraryStored.h"

namespace Framework
{
	class RoomType;
	struct LibraryStoredRenamer;

	struct DoorVRCorridor
	{
	public:
		bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		RoomType* get_room(Random::Generator& _rg) const;
		RoomType* get_elevator(Random::Generator& _rg) const;
		bool get_priority(Random::Generator& _rg, OUT_ float & _result) const;
		bool get_priority_range(OUT_ Range & _result) const;

		void set_priority(Optional<Range> const & _priority) { priority = _priority; }

		void clear_room_types();

	public:
		DoorVRCorridor create_copy() const;
		bool apply_renamer(LibraryStoredRenamer const & _renamer, Library * _library = nullptr);

	private:
		struct Entry
		{
			float probabilityCoef = 1.0f; // will use roomType's probCoef from tags too!
			UsedLibraryStored<RoomType> roomType;

			bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			bool apply_renamer(LibraryStoredRenamer const & _renamer, Library * _library = nullptr);
		};

		Array<Entry> rooms;
		Array<Entry> elevators;
		Optional<Range> priority;
	};

};