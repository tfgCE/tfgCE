#pragma once

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\other\simpleVariableStorage.h"
#include "..\..\..\core\random\random.h"

#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\world\roomGeneratorInfo.h"

#include "..\roomGenerationInfo.h"

class SimpleVariableStorage;

namespace Framework
{
	class Scenery;
	class SceneryType;
	class DoorInRoom;
	class Game;
	class MeshGenerator;
	class Room;
};

namespace TeaForGodEmperor
{
	namespace RoomGenerators
	{
		class SimpleElevatorWall;

		class SimpleElevatorWallInfo
		: public Framework::RoomGeneratorInfo
		{
			FAST_CAST_DECLARE(SimpleElevatorWallInfo);
			FAST_CAST_BASE(Framework::RoomGeneratorInfo);
			FAST_CAST_END();

			typedef Framework::RoomGeneratorInfo base;
		public:
			SimpleElevatorWallInfo();
			virtual ~SimpleElevatorWallInfo();

		public: // RoomGeneratorInfo
			implement_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			implement_ bool apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library = nullptr);
			implement_ Framework::RoomGeneratorInfoPtr create_copy() const;

			implement_ bool generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const;

		private:
			Range floorAltitude = Range(3.0f, 6.0f);

			Framework::MeshGenParam<Framework::LibraryName> wallSceneryType;

			friend class SimpleElevatorWall;
		};

		class SimpleElevatorWall
		{
		public:
			SimpleElevatorWall(SimpleElevatorWallInfo const * _info);
			~SimpleElevatorWall();

			bool generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext);

		private:
			SimpleElevatorWallInfo const * info;
		};

	};

};
