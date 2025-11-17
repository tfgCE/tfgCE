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
		class UseMeshGenerator;

		/**
		 *	Can be used as a stand alone room, that's why it uses Base and is a room generator (not just room element)
		 * 
		 *	It has an elaborate system for placing doors
		 *		1.	all doors (tagged in a specific manner -> "placeDoorsTagged") may be placed using POIs (-> "doorPOI")
		 *		2.	to match POI to a door, it might be done via "forTaggedDoor" (set on POI), where DoorInRoom's tags (! as they are set by generator) are checked
		 *			if that fails, in order they are created
		 *			note that only one door in room may use each door POI
		 *		3.	vr anchor may be chosen for all doors tagged in a specific manner -> "placeDoorsTagged") using available vr anchors (-> "vrAnchorPOI")
		 *		4.	vr anchor may be destined for specific door POI (it has to have name "forDoorPOI" set - it is then ONLY for this POI, unless vrAnchorId is set)
		 *			if that fails, vr anchor is matched to a door by "vrAnchorID" variable set (door and vrAnchor POIs)
		 *				this requires placing door in room using POI (1&2) where vrAnchorId is read from door POIs that will match vrAnchorId set for any vrAnchor POIs
		 *			if that fails, the closest vr anchor is chosen (note that it is highly discouraged to use this method unless doors are significantly far away
		 */
		class UseMeshGeneratorInfo
		: public Base
		{
			FAST_CAST_DECLARE(UseMeshGeneratorInfo);
			FAST_CAST_BASE(Base);
			FAST_CAST_END();

			typedef Base base;
		public:
			UseMeshGeneratorInfo();
			virtual ~UseMeshGeneratorInfo();

		public: // RoomGeneratorInfo
			implement_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			implement_ bool apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library = nullptr);
			implement_ Framework::RoomGeneratorInfoPtr create_copy() const;

			implement_ bool internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const;

		private:
			Framework::MeshGenParam<Framework::LibraryName> meshGenerator;
			Name atPOI;

			Name vrAnchorPOI;
			Name doorPOI; // general POI used to place doors - if set, provides vr placement for all door
			
			bool worldSeparatorDoor = false;
			TagCondition placeDoorsTagged; // (door-in -room's tags)
			TagCondition setVRPlacementForDoorsTagged; // set vr placement relative to vr anchor basing on POIs (door-in -room's tags)

			bool allowVRAnchorTurn180 = true; // allow turning vr anchor 180 to fit doors better

			friend class UseMeshGenerator;
		};

		class UseMeshGenerator
		{
		public:
			UseMeshGenerator(UseMeshGeneratorInfo const * _info);
			~UseMeshGenerator();

			bool generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext);

		private:
			UseMeshGeneratorInfo const * info = nullptr;
		};

	};

};
