#pragma once

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\random\random.h"

#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\meshGen\meshGenParam.h"
#include "..\..\..\framework\world\roomGeneratorInfo.h"

#include "..\roomGeneratorInfo.h"

class SimpleVariableStorage;

namespace Framework
{
	class Actor;
	class DoorInRoom;
	class Game;
	class MeshGenerator;
	class Room;

	namespace RoomGenerators
	{
		class ChooseRoomType
		: public RoomGeneratorInfo
		{
			FAST_CAST_DECLARE(ChooseRoomType);
			FAST_CAST_BASE(RoomGeneratorInfo);
			FAST_CAST_END();

			typedef RoomGeneratorInfo base;
		public:
			ChooseRoomType();
			virtual ~ChooseRoomType();

		public: // RoomGeneratorInfo
			implement_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			implement_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
			implement_ bool apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library = nullptr);
			implement_ RoomGeneratorInfoPtr create_copy() const;

			implement_ bool generate(Framework::Room* _room, bool _subGenerator, REF_ RoomGeneratingContext& _roomGeneratingContext) const;

		protected:
			struct Entry
			{
				MeshGenParam<LibraryName> roomType;
				float probabilityCoef = 1.0f;
			};
			static bool load_entries(Array<Entry> & _entries, IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			static bool generate_from_entries(Array<Entry> const & _entries, Framework::Room* _room, bool _subGenerator, REF_ RoomGeneratingContext& _roomGeneratingContext);

		private:
			Array<Entry> entries;

			friend class SimpleCorridor;
		};
	};

};
