#pragma once

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\random\random.h"

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
		class Towers;

		struct TowersPrepareVariablesContext
		{
		public:
			bool halfTower = false;
			Vector2 maxSize = Vector2::zero;
			Vector2 maxTileSize = Vector2::zero;
			float tileSize = 0.0f;
			float towerDiameter = 0.0f;
			float towerRadius = 0.0f;
			float innerTowerRadius = 0.0f;

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
		 *	Towers are placed in a ring.
		 *	Exits:
		 *		in (1+)
		 *		out (1)
		 *			These are placed on towers
		 *
		 *
		 *	Note:
		 *
		 *			POSSIBLE LAYOUTS
		 *
		 *	----	Looped, with or without connector tags
		 *			counter clockwise!
		 *
		 *							    t3
		 *							   /  \
		 *							  /    \
		 *						passage    passage
		 *						   /		  \
		 *						  /			   \
		 *			       in---t0			    t2---out
		 *						  \			   /
		 *						   \		  /
		 *						passage    passage
		 *							  \    /
		 *							   \  /
		 *							    t1
		 *
		 *	Main difference between towers and balconies is that towers may have 2 or 3 exits
		 *	and exits to other regions are placed on towers, while for balconies they are hidden
		 *	within the passages.
		 *
		 */
		class TowersInfo
		: public Base
		{
			FAST_CAST_DECLARE(TowersInfo);
			FAST_CAST_BASE(Base);
			FAST_CAST_END();

			typedef Base base;
		public:
			TowersInfo();
			virtual ~TowersInfo();

		public: // RoomGeneratorInfo
			implement_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			implement_ bool apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library = nullptr);
			implement_ Framework::RoomGeneratorInfoPtr create_copy() const;

			implement_ RefCountObjectPtr<PieceCombiner::Piece<Framework::RegionGenerator>> generate_piece_for_region_generator(REF_ PieceCombiner::Generator<Framework::RegionGenerator>& _generator, Framework::Region* _region, REF_ Random::Generator & _randomGenerator) const;
			implement_ bool internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const;

		private:
			/*
			 *	main mesh generator has to generate layout for towers
			 *	it should take variable(s):
			 *		int "towersCount"
			 *	it should add mesh nodes
			 *		"tower" with variables:
			 *			"index" (int) set from 0 to towersCount-1
			 */
			Framework::MeshGenParam<Framework::LibraryName> mainMeshGenerator;
			Framework::MeshGenParam<Framework::LibraryName> towerMeshGenerator;

			Random::Number<int> towersCount; // might be overriden by an _inRegion's variables
			Random::Number<int> towersBetweenDoorsCount; // might be overriden by an _inRegion's variables (gets outer connectors and adds additional towers between)

			Array<PieceCombiner::Connector<Framework::RegionGenerator>> connectors; // just general connectors (should be enough to connect to outer connectors)

			float towerConnectorsPriorityStartsAt = 1000.0f;
			// connectors used to connect towers, they work as templates for actual towers
			PieceCombiner::Connector<Framework::RegionGenerator> towerConnectorA;
			PieceCombiner::Connector<Framework::RegionGenerator> towerConnectorB;

			Name setAnchorVariable;
			Name getAnchorPOI;
			Name roomCentrePOI;

			friend class Towers;
		};

		class Towers
		{
		public:
			Towers(TowersInfo const * _info);
			~Towers();

			bool generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext);

		private:
			TowersInfo const * info = nullptr;
		};

	};

};
