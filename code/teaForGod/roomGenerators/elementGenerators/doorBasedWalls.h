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
		class DoorBasedWalls;

		class DoorBasedWallsInfo
		: public Framework::RoomGeneratorInfo
		{
			FAST_CAST_DECLARE(DoorBasedWallsInfo);
			FAST_CAST_BASE(Framework::RoomGeneratorInfo);
			FAST_CAST_END();

			typedef Framework::RoomGeneratorInfo base;
		public:
			DoorBasedWallsInfo();
			virtual ~DoorBasedWallsInfo();

		public: // RoomGeneratorInfo
			implement_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			implement_ bool apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library = nullptr);
			implement_ Framework::RoomGeneratorInfoPtr create_copy() const;

			implement_ bool generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const;

		private:
			Framework::MeshGenParam<Framework::LibraryName> wallSceneryType;
			Array<Framework::MeshGenParam<Framework::LibraryName>> orderedWallSceneryTypes; // we will use different wall scenery types, from lowest to highest value
			Framework::MeshGenParam<Vector3> orderDir = Vector3::zAxis; // in which direction does the order go (if orderDir:z=1 then the order goes with the z, -1, 0, 1 and so on)
			bool unlimitedOrdered = false; // if true, will use ordered for each wall/door
			Framework::MeshGenParam<Range> wallHeightAbove = Range(3.0f, 10.0f);
			float wallBigTopOffsetChance = 0.0f;
			bool wallsOfSameHeight = false; // all will have same height
			bool eachDoorUnique = false; // for each door create a separate wall, otherwise will group them
			Vector3 wallOffsetLocation = Vector3::zero;

			friend class DoorBasedWalls;
		};

		class DoorBasedWalls
		{
		public:
			DoorBasedWalls(DoorBasedWallsInfo const * _info);
			~DoorBasedWalls();

			bool generate(Framework::Game* _game, Framework::Room* _room, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext);

		private:
			DoorBasedWallsInfo const * info;
		};

	};

};
