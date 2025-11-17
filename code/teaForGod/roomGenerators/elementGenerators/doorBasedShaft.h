#pragma once

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\other\simpleVariableStorage.h"
#include "..\..\..\core\random\random.h"

#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\meshGen\meshGenParam.h"
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
		class DoorBasedShaft;

		class DoorBasedShaftInfo
		: public Framework::RoomGeneratorInfo
		{
			FAST_CAST_DECLARE(DoorBasedShaftInfo);
			FAST_CAST_BASE(Framework::RoomGeneratorInfo);
			FAST_CAST_END();

			typedef Framework::RoomGeneratorInfo base;
		public:
			DoorBasedShaftInfo();
			virtual ~DoorBasedShaftInfo();

		public: // RoomGeneratorInfo
			implement_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			implement_ bool apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library = nullptr);
			implement_ Framework::RoomGeneratorInfoPtr create_copy() const;

			implement_ bool generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const;

		private:

			Framework::MeshGenParam<Framework::LibraryName> shaftSceneryType;
			Framework::MeshGenParam<Range> sizeInTiles = Range(2.0f, 6.0f);

			Framework::MeshGenParam<Name> beyondDoorDepthVar; // get from door and offset it
			Framework::MeshGenParam<Name> beyondDoorBeWiderVar; // get from door and make size wider
			Framework::MeshGenParam<Framework::LibraryName> beyondDoorSceneryType; // if not provided, will use generic
			Framework::MeshGenParam<Framework::LibraryName> beyondDoorMeshGenerator;

			Framework::MeshGenParam<Framework::LibraryName> pipeSceneryType;
			Framework::MeshGenParam<Range> pipeRadius = Range(0.15f, 0.25f);
			Framework::MeshGenParam<float> extraMargin = 0.0f;
			Framework::MeshGenParam<float> horizontalPipeChance = 0.4f;
			Framework::MeshGenParam<Range> horizontalPipeDensity = Range(0.2f, 0.8f);
			Framework::MeshGenParam<float> horizontalPipeUsing2IPChance = 1.0f;
			Framework::MeshGenParam<float> verticalPipeChance = 0.8f;
			Framework::MeshGenParam<Range> verticalPipeDensity = Range(0.2f, 0.8f);
			Framework::MeshGenParam<RangeInt> verticalPipeCluster = RangeInt(1, 6);
			Framework::MeshGenParam<float> pipeTopLimit = 9999.0f;

			friend class DoorBasedShaft;
		};

		class DoorBasedShaft
		{
		public:
			DoorBasedShaft(DoorBasedShaftInfo const * _info);
			~DoorBasedShaft();

			bool generate(Framework::Game* _game, Framework::Room* _room, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext);

		private:
			DoorBasedShaftInfo const * info;
		};

	};

};
