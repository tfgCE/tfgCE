#pragma once

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\random\random.h"

#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\world\roomGeneratorInfo.h"

#include "..\roomGenerationInfo.h"

#include "..\roomGeneratorBase.h"

class SimpleVariableStorage;

namespace Framework
{
	class Actor;
	class DoorInRoom;
	class Game;
	class MeshGenerator;
	class Room;
};

namespace TeaForGodEmperor
{
	namespace RoomGenerators
	{
		class RoomWithDoorBlocks;

		/**
		 *	This is a room with blocks that hold doors
		 *	There can be any number of doors, mesh nodes are used to generated blocks themselves
		 *	Blocks are rectangular in shape
		 */
		class RoomWithDoorBlocksInfo
		: public Base
		{
			FAST_CAST_DECLARE(RoomWithDoorBlocksInfo);
			FAST_CAST_BASE(Base);
			FAST_CAST_END();

			typedef Base base;
		public:
			RoomWithDoorBlocksInfo();
			virtual ~RoomWithDoorBlocksInfo();

		public: // RoomGeneratorInfo
			implement_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			implement_ bool apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library = nullptr);
			implement_ Framework::RoomGeneratorInfoPtr create_copy() const;

			implement_ bool internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const;

		private:
			Framework::UsedLibraryStored<Framework::MeshGenerator> roomMeshGenerator;

			bool forceMeshSizeToPlayAreaSize = false;

			friend class RoomWithDoorBlocks;
		};

		class RoomWithDoorBlocks
		{
		public:
			RoomWithDoorBlocks(RoomWithDoorBlocksInfo const * _info);
			~RoomWithDoorBlocks();

			bool generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext);

		private:
			RoomWithDoorBlocksInfo const * info = nullptr;
		};

	};

};
