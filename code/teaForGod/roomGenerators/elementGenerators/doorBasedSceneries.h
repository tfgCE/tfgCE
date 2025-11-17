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
		class DoorBasedSceneries;

		/**
		 *	Although can be used in any configuration
		 *	It might be best to be used with doors facing each other or facing in the same dir
		 */
		class DoorBasedSceneriesInfo
		: public Framework::RoomGeneratorInfo
		{
			FAST_CAST_DECLARE(DoorBasedSceneriesInfo);
			FAST_CAST_BASE(Framework::RoomGeneratorInfo);
			FAST_CAST_END();

			typedef Framework::RoomGeneratorInfo base;
		public:
			DoorBasedSceneriesInfo();
			virtual ~DoorBasedSceneriesInfo();

		public: // RoomGeneratorInfo
			implement_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			implement_ bool apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library = nullptr);
			implement_ Framework::RoomGeneratorInfoPtr create_copy() const;

			implement_ bool generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const;

		private:
			struct SceneryOrMeshGenerator
			{
				Framework::MeshGenParam<Framework::LibraryName> sceneryType;
				Framework::MeshGenParam<Framework::LibraryName> meshGenerator;

				bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
			};
			TagCondition doorsTagged;
			SceneryOrMeshGenerator somg;
			Array<SceneryOrMeshGenerator> orderedSOMGs; // we will use different wall scenery types, from lowest to highest value
			Framework::MeshGenParam<Vector3> orderDir = Vector3::zAxis; // in which direction does the order go (if orderDir:z=1 then the order goes with the z, -1, 0, 1 and so on)
			bool orderByFurthest = false; // will calculate the centre between objects and will start with the furthest (along door dir)
			bool unlimitedOrdered = false; // if true, will use ordered for each wall/door
			bool firstIsBackground = false; // if true, will double the first wall/door, to make it work as a background
			bool provideRoomGeneratorContextsSpaceUsedToFirst = false; // for the background, makes sense only if first is background
			bool atSameZMin = false; // all will have same height
			bool atSameZMax = false; // all will have same height
			Vector3 offsetLocation = Vector3::zero;
			bool groupAll = false;
			Optional<float> groupByYaw;
			Optional<float> groupByOffset; // max along dir

			friend class DoorBasedSceneries;
		};

		class DoorBasedSceneries
		{
		public:
			DoorBasedSceneries(DoorBasedSceneriesInfo const * _info);
			~DoorBasedSceneries();

			bool generate(Framework::Game* _game, Framework::Room* _room, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext);

		private:
			DoorBasedSceneriesInfo const * info;
		};

	};

};
