#pragma once

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\random\random.h"
#include "..\..\..\core\types\dirFourClockwise.h"

#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\world\regionGenerator.h"
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
	class SceneryType;
};

namespace TeaForGodEmperor
{
	namespace RoomGenerators
	{
		class Balconies;

		// these are more like suggestions and can be ignored by actual mesh generators, although it is advised to stick to max size
		struct BalconiesPrepareVariablesContext
		{
		public:
			Vector2 maxSize = Vector2::zero;
			Vector2 maxTileSize = Vector2::zero;
			float tileSize = 0.0;
			float balconyDoorWidth = 0.0f;
			float balconyDoorHeight = 0.0f;
			Vector2 availableSpaceForBalcony = Vector2::zero; // it has to be at least 2x2 tiles
			float maxBalconyWidth = 0.0;

			void use(RoomGenerationInfo const& _roomGenerationInfo) { roomGenerationInfo = &_roomGenerationInfo; }
			//
			void use(SimpleVariableStorage const& _variables) { variables = &_variables; }
			void use(WheresMyPoint::IOwner const* _wmpOwner) { wmpOwner = _wmpOwner; }

			void process();

		private:
			RoomGenerationInfo const* roomGenerationInfo = nullptr;
			//
			SimpleVariableStorage const* variables = nullptr;
			WheresMyPoint::IOwner const* wmpOwner = nullptr;
		};

		/**
		 *	Balconies are placed in a line (+ fake balconies). Always counter clockwise!
		 *	Exits:
		 *		in (1)
		 *		out (1)
		 *			These are placed on first an last balcony
		 *		in-extra
		 *			These are placed inside passages
		 *
		 *
		 *	Note:	
		 *
		 *			POSSIBLE LAYOUTS
		 *
		 *	----	Not looped
		 *
		 *	 first	  passage passage passage passage passage   last
		 *	  in  \	   /  \    /  \    /  \    /  \    /  \    / out
		 *		   f  n	   p  n    p  n    p  n    p  n    p  l
		 *			b0		b1		b2		b3		b4		b5
		 *
		 *			this requires "looped" set to false (or not set)
		 *			first and last have to be defined
		 *			prev and next have to be defined
		 *
		 *
		 *	----	Looped, with or without connector tags
		 *			counter clockwise!
		 *
		 *							  passage
		 *							   /  \
		 *							  n    p
		 *							b0		b3
		 *						   p		  n
		 *						  /			   \
		 *			  in---passage			    passage---out
		 *						  \			   /
		 *						   n		  p
		 *							b1		b2
		 *							  p    n
		 *							   \  /
		 *							  passage
		 *
		 *			this requires "looped" set to true
		 *			prev and next have to be defined
		 *
		 *	Main difference between towers and balconies is that towers may have 2 or 3 exits
		 *	and exits to other regions are placed on towers, while for balconies they are hidden
		 *	within the passages.
		 *
		 */
		class BalconiesInfo
		: public Base
		{
			FAST_CAST_DECLARE(BalconiesInfo);
			FAST_CAST_BASE(Base);
			FAST_CAST_END();

			typedef Base base;
		public:
			BalconiesInfo();
			virtual ~BalconiesInfo();

		public: // RoomGeneratorInfo
			implement_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			implement_ bool apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library = nullptr);
			implement_ Framework::RoomGeneratorInfoPtr create_copy() const;

			implement_ RefCountObjectPtr<PieceCombiner::Piece<Framework::RegionGenerator>> generate_piece_for_region_generator(REF_ PieceCombiner::Generator<Framework::RegionGenerator>& _generator, Framework::Region* _region, REF_ Random::Generator & _randomGenerator) const;
			implement_ bool internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const;

		private:
			/*
			 *	main mesh generator has to generate wall to have balconies there
			 *	it should take an int variable:
			 *		"balconyCount"
			 *	it should add mesh nodes
			 *		"balcony" with variables:
			 *			"index" (int) set from 0 to balconyCount-1
			 *			"swapDoors" (bool, optional) to swap left and right door
			 *	also should take int variables:
			 *		"fakeBalconyPreCount"
			 *		"fakeBalconyPostCount"
			 *	that would add fake balcony mesh nodes (or just create fake balconies on its own, then there should be no fake balconies mesh generator provided)
			 *		"fake balcony" no other variables required
			 */
			Framework::MeshGenParam<Framework::LibraryName> mainMeshGenerator;
			/*
			 *	balcony mesh generator uses variables:
			 *		"availableSpaceForBalcony" (vector2) this is space available for balcony, doors included
			 *	has to provide mesh nodes:
			 *		"left door" (pointing outside!)
			 *		"right door" (pointing outside!)
			 *		"wall" (to place at the wall, forward being from the wall, dir that wall faces, if none, balcony placement/identity is assumed)
			 *		"available space centre" (to set vr space for doors etc, if none, balcony placement/identity is assumed)
			 *	mesh generator should be aware of place it has to fit door within provided space (that is within playable area),
			 *	although we can rotate it any way we want as long as we keep it within playable area
			 *
			 *	fake balconies should create doors on their own
			 */
			Framework::MeshGenParam<Framework::LibraryName> balconyMeshGenerator;
			Framework::MeshGenParam<Framework::LibraryName> fakeBalconyMeshGenerator;

			// fake balconies are any other balconies that
			Random::Number<int> fakeBalconyPreCount;
			Random::Number<int> fakeBalconyPostCount;

			bool dontModifyBalconyCount = false;
			Random::Number<int> balconyCount; // might be overridden by an _inRegion's variables

			// just general connectors
			// if not looped, one connector should have "in" tag, and the other should have "out" tag
			// this way all doors will be connected properly to balconies
			Array<PieceCombiner::Connector<Framework::RegionGenerator>> connectors;

			bool balconyConnectorsLooped = false;
			bool balconyConnectorsClockwise = true;
			bool maySkipFirstLastConnectors = false;
			Optional<DirFourClockwise::Type> openWorldBreakerLocalDir; // if not looped, we may provide a local dir against which we organise order of outer connectors/doors
			struct BalconyConnectors
			{
				Array<DirFourClockwise::Type> localDirIs;
				Array<DirFourClockwise::Type> localDirNot;
				float priorityStartsAt = 1000.0f;
				// connectors used to connect balconies with passages
				// will choose just one, the first one that has the right parameters met
				Array<PieceCombiner::Connector<Framework::RegionGenerator>> next;
				Array<PieceCombiner::Connector<Framework::RegionGenerator>> prev;
				Array<PieceCombiner::Connector<Framework::RegionGenerator>> first;
				Array<PieceCombiner::Connector<Framework::RegionGenerator>> last;
			};
			Array<BalconyConnectors> balconyConnectors;
			bool ignoreAllConnectorTags = false;
			Array<Name> ignoreConnectorTags;
			Name setAnchorVariable;
			Name getAnchorPOI;
			Name roomCentrePOI;

			BalconyConnectors const& choose_balcony_connectors(Optional<DirFourClockwise::Type> const& _dir, Random::Generator& _randomGenerator) const;

			friend class Balconies;
		};

		class Balconies
		{
		public:
			Balconies(BalconiesInfo const * _info);
			~Balconies();

			bool generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext);

		private:
			BalconiesInfo const * info = nullptr;
		};

	};

};
