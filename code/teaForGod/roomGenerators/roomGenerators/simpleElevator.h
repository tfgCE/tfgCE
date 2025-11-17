#pragma once

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\other\simpleVariableStorage.h"
#include "..\..\..\core\random\random.h"

#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\world\roomGeneratorInfo.h"

#include "..\roomGenerationInfo.h"

#include "..\roomGeneratorBase.h"

class SimpleVariableStorage;

namespace Framework
{
	class Scenery;
	class SceneryType;
	class DoorInRoom;
	class DoorType;
	class Game;
	class MeshGenerator;
	class Room;
	class RoomType;
};

namespace TeaForGodEmperor
{
	namespace RoomGenerators
	{
		class SimpleElevator;

		class SimpleElevatorInfo
		: public Base
		{
			FAST_CAST_DECLARE(SimpleElevatorInfo);
			FAST_CAST_BASE(Base);
			FAST_CAST_END();

			typedef Base base;
		public:
			SimpleElevatorInfo();
			virtual ~SimpleElevatorInfo();

		public: // RoomGeneratorInfo
			implement_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			implement_ bool apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library = nullptr);
			implement_ Framework::RoomGeneratorInfoPtr create_copy() const;

			implement_ bool internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const;

		private:
			enum ElevatorType
			{
				Vertical,
				Horizontal,
				RotateXY,
			};

			ElevatorType type = ElevatorType::Vertical;

			Framework::MeshGenParam<Range> floorDistance = Range(3.0f, 6.0f);

			Framework::MeshGenParam<Framework::LibraryName> elevatorDoorType;
			Framework::MeshGenParam<Framework::LibraryName> floorRoomType;
			// cart points have to fill gap between door hole in floor scenery and elevator
			Framework::MeshGenParam<Framework::LibraryName> cartPointSceneryType;
			Framework::MeshGenParam<Framework::LibraryName> cartSceneryType;
			Framework::MeshGenParam<Framework::LibraryName> cartLaneSceneryType;
			
			Framework::MeshGenParam<float> maxElevatorLength;
			Framework::MeshGenParam<float> maxElevatorTileLength;
			Framework::MeshGenParam<float> maxElevatorLengthAdd;

			friend class SimpleElevator;
		};

		struct SimpleElevatorAppearanceSetup
		{
			float elevatorHeight = 2.0f;
			float railingHeight = 1.0f;

			void setup_parameters(Framework::Room const* _room, OUT_ SimpleVariableStorage& _parameters) const;
			void setup_variables(Framework::Room const* _room, Framework::Scenery* _scenery) const;
		};

		class SimpleElevator
		{
		public:
			SimpleElevator(SimpleElevatorInfo const * _info);
			~SimpleElevator();

			void use_setup(SimpleElevatorAppearanceSetup const & _setup) { appearanceSetup = _setup; }

			bool generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext);

		private:
			SimpleElevatorInfo const * info;

			SimpleVariableStorage roomVariables;
			SimpleElevatorAppearanceSetup appearanceSetup;
		};

	};

};
