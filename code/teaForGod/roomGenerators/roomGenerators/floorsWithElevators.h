#pragma once

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\random\random.h"
#include "..\..\..\core\types\dirFourClockwise.h"

#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\world\regionGenerator.h"
#include "..\..\..\framework\world\roomGeneratorInfo.h"

#include "..\roomGenerationInfo.h"

#include "..\roomGeneratorBase.h"

#include "simpleElevator.h"

class SimpleVariableStorage;

namespace Framework
{
	class Actor;
	class DoorInRoom;
	class Game;
	class MeshGenerator;
	class Room;
	class SceneryType;
};

namespace TeaForGodEmperor
{
	namespace RoomGenerators
	{
		class FloorsWithElevators;

		/**
		 *	Each floor has on exit and has to be connected to any other floor with an elevator.
		 *	All floors have to be connected.
		 *	There might be empty floors (no exits).
		 *
		 */
		class FloorsWithElevatorsInfo
		: public Base
		{
			FAST_CAST_DECLARE(FloorsWithElevatorsInfo);
			FAST_CAST_BASE(Base);
			FAST_CAST_END();

			typedef Base base;
		public:
			FloorsWithElevatorsInfo();
			virtual ~FloorsWithElevatorsInfo();

		public: // RoomGeneratorInfo
			implement_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			implement_ bool apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library = nullptr);
			implement_ Framework::RoomGeneratorInfoPtr create_copy() const;

			implement_ bool internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const;

		private:
			/*
			 *	main mesh generator should create the floors
			 *	floors should be square
			 *	it should take variables:
			 *		"floorCount" (int)
			 *		"floorInnerSize" (float)
			 *		"floorOuterSize" (float)
			 *	it should add mesh nodes
			 *		"floor" with variables:
			 *			"index" (int) set from 0 to floorCount-1
			 * 
			 *	door mesh generator is used to provide a filling beyond the door
			 *	it should accept variable:
			 *		"doorWidth" (float)
			 */
			Framework::MeshGenParam<Framework::LibraryName> mainMeshGenerator;
			Framework::MeshGenParam<Framework::LibraryName> floorSceneryType;
			Framework::MeshGenParam<Framework::LibraryName> doorMeshGenerator; // placed where door is, to fit with everything else

			// cart points have to fill gap between door hole in floor scenery and elevator
			Framework::MeshGenParam<Framework::LibraryName> cartPointSceneryType;
			Framework::MeshGenParam<Framework::LibraryName> cartSceneryType;
			Framework::MeshGenParam<Framework::LibraryName> cartLaneSceneryType;

			Framework::MeshGenParam<Framework::LibraryName> blockSlidingLocomotionBoxSceneryType;
			
			Framework::MeshGenParam<Range> maxOuterRadiusAsTiles;

			Random::Number<int> extraFloors;
			Random::Number<int> totalFloors;

			Name setAnchorVariable;
			Name getAnchorPOI;
			Name roomCentrePOI;

			bool eachDoorAtUniqueFloor = false;

			friend class FloorsWithElevators;
		};

		class FloorsWithElevators
		{
		public:
			FloorsWithElevators(FloorsWithElevatorsInfo const * _info);
			~FloorsWithElevators();

			bool generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext);

		private:
			FloorsWithElevatorsInfo const * info = nullptr;

			SimpleElevatorAppearanceSetup elevatorAppearanceSetup;

			void add_block_sliding_locomotion_box(Framework::Game* _game, Framework::Room* _room, Framework::SceneryType* blockSlidingLocomotionBoxSceneryType, Transform const& _placement, Vector2 const& _boxSize, Vector2 const& _boxAtPt, Random::Generator& randomGenerator);
		};

	};

};
