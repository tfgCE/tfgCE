#pragma once

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\other\simpleVariableStorage.h"
#include "..\..\..\core\random\random.h"

#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\meshGen\meshGenParam.h"
#include "..\..\..\framework\world\roomGeneratorInfo.h"

#include "..\roomGenerationInfo.h"

#include "..\roomGeneratorBase.h"

namespace Framework
{
	class Scenery;
	class SceneryType;
	class DoorInRoom;
	class Game;
	class MeshGenerator;
	class Room;
};

#ifdef AN_DEVELOPMENT
//#define AN_PLATFORM_DEBUG_INFO
#endif

namespace TeaForGodEmperor
{
	namespace RoomGenerators
	{
		class PlatformsLinear;

		namespace PlatformsLinearDoorSetupThrough
		{
			enum Type
			{
				DoorInRooms,
				CreatePOIs,
			};
		};

		/**
		*	Platforms linear, directional.
		*	Going from near to far.
		*	Platforms + carts are created within allowed rectangular vr space.
		*	Each platform has at least one cart assigned to it.
		*	Carts are moving along axes (X, Y). But may change altitude - this should allow to have climbing platforms - stairs etc.
		*
		*	Room should already have doors assigned although they are not required to be placed in vr, no vr anchor.
		*
		*	Placements are done within tileCount (that is modified to allow for more variety)
		*/
		struct PlatformsLinearSetup
		{
			// some meshes might be shared by all sceneries - they are generated in the same way and only individual instance is different (for voxels)
#ifdef AN_DEVELOPMENT
			Framework::MeshGenParam<bool> shareCartMeshes = true; // to speed up
#else
			Framework::MeshGenParam<bool> shareCartMeshes = false;
#endif
			Framework::MeshGenParam<bool> shareCartPointMeshes = true;

			bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext& _lc);
			void fill_with(SimpleVariableStorage const & _variables);
		};

		struct PlatformsLinearExtraPlatform
		{
			// these are platforms that are predefined with given parameters
			// if outside, they will be connected towards the actual grid
			Framework::MeshGenParam<VectorInt2> at; // 0 is on the left end of the grid, 1 is at the actual grid, if negative, starts from the end (-1 is the last one!)
			Framework::MeshGenParam<bool> matchCartPointLeft; // only neighbours, best to use for platforms that have single cart point
			Framework::MeshGenParam<bool> matchCartPointRight;
			Framework::MeshGenParam<bool> matchCartPointForth;
			Framework::MeshGenParam<bool> matchCartPointBack;

			// will override whatever is in general setup
			Framework::MeshGenParam<RangeInt> railingsLimitLeft;
			Framework::MeshGenParam<RangeInt> railingsLimitRight;
			Framework::MeshGenParam<RangeInt> railingsLimitForth;
			Framework::MeshGenParam<RangeInt> railingsLimitBack;
			Framework::MeshGenParam<bool> blockCartSideLeft;
			Framework::MeshGenParam<bool> blockCartSideRight;
			Framework::MeshGenParam<bool> blockCartSideForth;
			Framework::MeshGenParam<bool> blockCartSideBack;

			struct UseOrSpawn
			{
				Framework::MeshGenParam<Framework::LibraryName> name; // will get some information about the platform (size, placement
				Framework::MeshGenParam<Vector3> atPt; // relative to platform size, default 0.5,0.5,0.0
				Framework::MeshGenParam<Vector3> forwardDir; // default: y-axis
				Framework::MeshGenParam<Vector3> upDir; // default: z-axis
				Tags tags; // for scenery types
			};
			Array<UseOrSpawn> useMeshGenerators;
			Array<UseOrSpawn> spawnSceneryTypes;

			bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
			void fill_with(SimpleVariableStorage const& _variables);
		};

		struct PlatformsLinearPlatformsSetup
		{
			Random::Generator random; // only for working

			// these are only ones that can be randomised on the fly
			Framework::MeshGenParam<RangeInt> lines; // this is without starting and ending line
			Framework::MeshGenParam<RangeInt> width;

			Framework::MeshGenParam<bool> exitPlatformsCentred; // exits platforms are centred (all!)
			Framework::MeshGenParam<bool> centralPlatformInTheCentre; // when stretching, central platform will be the central point of the map

			Framework::MeshGenParam<bool> stretchToMatchSizeX; // if width is smaller than provided, will make it wider, default : false
			Framework::MeshGenParam<bool> stretchToMatchSizeY; // if size is smaller than provided, will make it wider, default : true
			
			Framework::MeshGenParam<bool> allowSimplePath; // if false, won't remove stuff

			Framework::MeshGenParam<bool> fullOuterRing; // all grid elements on outer ring will be active
			Framework::MeshGenParam<float> platformChance;
			Framework::MeshGenParam<float> connectChance;

			// others are restrict values that if to be randomised, have to be done with wheresMyPoint
			Framework::MeshGenParam<float> useVerticals;
			Framework::MeshGenParam<float> chanceOfUsingTwoPlatformsForFourCarts; // 4 carts mean either clockwise or counter-clockwise or two platforms
			Framework::MeshGenParam<bool> ignoreMinimumCartsSeparation;
			Framework::MeshGenParam<Vector3> platformSeparation; // y is the same as x! always! if you don' set it, game will set it
			Framework::MeshGenParam<float> minDoorPlatformSeparation;
			Framework::MeshGenParam<float> overrideRequestedCellSize;
			Framework::MeshGenParam<float> overrideRequestedCellWidth; // if overrides size, to have separate width
			Framework::MeshGenParam<Range> cartLineSeparation;
			Framework::MeshGenParam<float> platformHeight;
			Framework::MeshGenParam<float> maxSteepAngle;
			Framework::MeshGenParam<Vector2> maxPlayAreaSize;
			Framework::MeshGenParam<Vector2> maxPlayAreaTileSize;
			Framework::MeshGenParam<float> maxTileSize;

			Framework::MeshGenParam<RangeInt> railingsLimitLeft;
			Framework::MeshGenParam<RangeInt> railingsLimitRight;
			Framework::MeshGenParam<RangeInt> railingsLimitForth;
			Framework::MeshGenParam<RangeInt> railingsLimitBack;

			Framework::MeshGenParam<RangeInt> railingsLimitEntrances;
			
			Framework::MeshGenParam<bool> outerGridRailings; // if false, platforms on the limits of grid won't have railings

			Framework::MeshGenParam<float> alongYSlopeChance;
			Framework::MeshGenParam<float> alongXSlopeChance;
			Framework::MeshGenParam<float> forceLowInTheMiddleChance;
			Framework::MeshGenParam<float> forceHighOutsideTheMiddleChance;
			Framework::MeshGenParam<float> allHighChance;
			Framework::MeshGenParam<float> noRailingsChance;

			Array<PlatformsLinearExtraPlatform> extraPlatforms;

			bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			void randomise_missing(Random::Generator const & _random); // will override_ anything! use first, then use fill_with
			void fill_with(SimpleVariableStorage const & _variables);

			void log(LogInfoContext & _context) const;
		};

		struct PlatformsLinearAppearanceSetup
		{
			Framework::MeshGenParam<float> cartHeight = 2.0f;
			Framework::MeshGenParam<float> railingHeight = 1.0f;
			Framework::MeshGenParam<float> cartRailingThickness = 0.05f;
			Framework::MeshGenParam<float> platformRailingThickness = 0.05f;
			Framework::MeshGenParam<float> cartPointThickness = 0.05f;
			Framework::MeshGenParam<float> cartDoorThickness = 0.1f;

			bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext& _lc);
			void fill_with(SimpleVariableStorage const & _variables);

			void setup_parameters(REF_ SimpleVariableStorage & _parameters) const;
			void setup_variables(Framework::Scenery* _scenery) const;
		};

		struct PlatformsLinearVerticalAdjustment
		{
			// (a * x^2 + b * x) * h (see below)
			Framework::MeshGenParam<Range> xA = Range(0.0f);
			Framework::MeshGenParam<Range> yA = Range(0.0f);
			Framework::MeshGenParam<Range> xB = Range(0.0f);
			Framework::MeshGenParam<Range> yB = Range(0.0f);
			Framework::MeshGenParam<Range> hLimit; // limit to this range (limits hPt), if hPt not set, choose randomly from limit
			Framework::MeshGenParam<Range> hPt; // use max size of x, y

			bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext& _lc);
			void fill_with(SimpleVariableStorage const & _variables);
			void choose(Random::Generator const & _random);

			void log(LogInfoContext & _context);
		};

		struct PlatformsLinearDoorSetupThroughCreatePOIs
		{
			Framework::MeshGenParam<int> inCount = 1;
			Framework::MeshGenParam<int> outCount = 1;
			Framework::MeshGenParam<int> leftCount = 0;
			Framework::MeshGenParam<int> rightCount = 0;
			Framework::MeshGenParam<Name> inPOIName;
			Framework::MeshGenParam<Name> outPOIName;
			Framework::MeshGenParam<Name> leftPOIName;
			Framework::MeshGenParam<Name> rightPOIName;
			// will use vr anchor and vr anchor id

			bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
			void fill_with(SimpleVariableStorage const& _variables);
		};

		class PlatformsLinearInfo
		: public Base
		{
			FAST_CAST_DECLARE(PlatformsLinearInfo);
			FAST_CAST_BASE(Base);
			FAST_CAST_END();

			typedef Base base;
		public:
			PlatformsLinearInfo();
			virtual ~PlatformsLinearInfo();

		public: // RoomGeneratorInfo
			implement_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			implement_ bool apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library = nullptr);
			implement_ Framework::RoomGeneratorInfoPtr create_copy() const;

			implement_ bool internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const;

		public:
			PlatformsLinearSetup const & get_setup() const { return setup; }
			PlatformsLinearPlatformsSetup const & get_platforms_setup() const { return platformsSetup; }
			PlatformsLinearAppearanceSetup const & get_appearance_setup() const { return appearanceSetup; }
			PlatformsLinearVerticalAdjustment const& get_vertical_adjustment() const { return verticalAdjustment; }

		private:
			// those might be still overriden by generation params
			Framework::MeshGenParam<Framework::LibraryName> platformSceneryType;
			Framework::MeshGenParam<Framework::LibraryName> atDoorSceneryType;

			Framework::MeshGenParam<Framework::LibraryName> cartPointSceneryType;
			Framework::MeshGenParam<Framework::LibraryName> cartPointVerticalSceneryType;
			Framework::MeshGenParam<Framework::LibraryName> cartPointHorizontalSceneryType;
			Framework::MeshGenParam<Framework::LibraryName> cartSceneryType;
			Framework::MeshGenParam<Framework::LibraryName> cartVerticalSceneryType;
			Framework::MeshGenParam<Framework::LibraryName> cartHorizontalSceneryType;

			Framework::MeshGenParam<Framework::LibraryName> cartLaneSceneryType;
			Framework::MeshGenParam<Framework::LibraryName> cartLaneVerticalSceneryType;
			Framework::MeshGenParam<Framework::LibraryName> cartLaneHorizontalSceneryType;
			Framework::MeshGenParam<Framework::LibraryName> railingNoRailing;
			Framework::MeshGenParam<Framework::LibraryName> railingLowTransparent;
			Framework::MeshGenParam<Framework::LibraryName> railingLowSolid;
			Framework::MeshGenParam<Framework::LibraryName> railingMedSolid;
			Framework::MeshGenParam<Framework::LibraryName> railingHighSolid;
			Framework::MeshGenParam<bool> allowRailingTransparent;

			PlatformsLinearSetup setup;
			PlatformsLinearPlatformsSetup platformsSetup;
			PlatformsLinearAppearanceSetup appearanceSetup;
			PlatformsLinearVerticalAdjustment verticalAdjustment;

			Name setAnchorVariable;
			Name setSpaceOccupiedVariable; // Range3
			Name setCentralTopPlatformAtVariable; // Vector3
			Name setCentralTopPlatformSizeVariable; // Vector3
			Framework::MeshGenParam<Vector3> increaseSpaceOccupied;
			Framework::MeshGenParam<Range> anchorBelow;
			Framework::MeshGenParam<Range> anchorAbove;

			PlatformsLinearDoorSetupThrough::Type doorSetupThrough = PlatformsLinearDoorSetupThrough::DoorInRooms;

			PlatformsLinearDoorSetupThroughCreatePOIs doorSetupThroughCreatePOIs;

			friend class PlatformsLinear;
		};

		class PlatformsLinear
		{
		public:
			PlatformsLinear(PlatformsLinearInfo const * _info);
			virtual ~PlatformsLinear();

			//void use_setup(PlatformsLinearSetup const & _setup) { setup = _setup; }
			//void use_setup(PlatformsLinearPlatformsSetup const & _setup) { platformsSetup = _setup; }
			//void use_setup(PlatformsLinearAppearanceSetup const & _setup) { appearanceSetup = _setup; }
			RoomGenerationInfo const & get_room_generation_info() const { return roomGenerationInfo; }

		public:
			bool generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext);

		protected:
			PlatformsLinearInfo const * info = nullptr;
			SimpleVariableStorage roomVariables;
			PlatformsLinearSetup setup; // working copy
			PlatformsLinearPlatformsSetup platformsSetup; // working copy
			PlatformsLinearAppearanceSetup appearanceSetup; // working copy
			PlatformsLinearVerticalAdjustment verticalAdjustment; // working copy


			enum CartAxis
			{
				CA_BackForth,
				CA_LeftRight,
				CA_VerticalX,
				CA_VerticalY,
			};
			enum PlatformDir
			{
				PD_Left = 1,
				PD_Right = 2,
				PD_Back = 4,
				PD_Forth = 8,
				PD_DownX = 16,
				PD_UpX = 32,
				PD_DownY = 64,
				PD_UpY = 128,
			};
			enum PlatformType
			{
				PT_Filler,
				PT_KeyPoint,
				PT_In,
				PT_Out,
				PT_Left,
				PT_Right,
				PT_Extra,
			};
			enum RailingSize
			{
				RS_None = 0,
				RS_Low = 1,
				RS_Med = 2,
				RS_High = 3
			};
			struct Platform;
			struct CartPoint
			{
				CartAxis axis;
				int dir; // BF: back or forth, LR: left or right
				int side; // on which side is the cart: BF: left or right, LR: near or far
				VectorInt2 tilePlacement; // vr / local placement of cart
				//
				int cartIdx = NONE;
				Transform placement; // vr / local - forward is towards platform - this is relative to platform scenerys centre
				void calculate_placement(PlatformsLinear const * _generator, Platform const * _platform);
				void set(PlatformDir _pd);
				VectorInt2 get_entry_tile_placement() const;
				RangeInt2 get_entry_tiles(PlatformsLinear const * _generator) const;
#ifdef AN_DEVELOPMENT_OR_PROFILER
				tchar get_dir_as_char() const;
#endif
			};
			struct Platform
			{
				bool placed = false;
				PlatformType type = PT_Filler;
#ifdef AN_PLATFORM_DEBUG_INFO
				String debugInfo;
#endif
				RangeInt2 tilesAvailable = RangeInt2::empty; // empty = no limitations
				int matchCartPoint = 0; // use PD
				int blockCartSide = 0; // use PD

				Optional<VectorInt2> atGrid; // location at the grid, if not set, it was added to fit other platforms
				Vector3 at = Vector3::zero; // this is thing filled at the end, this is not the platform's centre! it is against what platform is used
				Range2 platform = Range2::empty; // created using "at" and sides - if side is not set, whole available space might be used
				Vector3 move = Vector3::zero;
				int idFromIn = NONE;

				// limits for railings in each dir RS_None 0 to RS_High 3
				RangeInt railingsLeft = RangeInt::empty; 
				RangeInt railingsRight = RangeInt::empty;
				RangeInt railingsBack = RangeInt::empty;
				RangeInt railingsForth = RangeInt::empty;

				Array<CartPoint> cartPoints;

				Vector3 get_room_placement() const { return at + get_centre_offset(); }
				Vector3 get_centre_offset() const { return Vector3(platform.x.centre(), platform.y.centre(), 0.0f); }
				void calculate_platform(PlatformsLinear const * _generator, Random::Generator & _random);
				CartPoint * find_cart_point_unconnected(CartAxis _axis, int _dir);
				Vector3 calculate_half_size(float _platformHeight) const; // use it with room_placement, not "at"
				bool can_have_another_vertical(int _dir) const;
				bool has_multiple_in_one_dir() const;
				bool has_multiple_in_one_dir(CartPoint const * _cps, int _count) const;
				bool can_add_to_have_unique_dirs(CartPoint const & _cp) const;

#ifdef AN_DEVELOPMENT_OR_PROFILER
				Array<String> const get_layout(PlatformsLinear const * _generator) const;
#endif
			};
			struct ElevatorCartPointRef
			{
				int platformIdx = NONE;
				int cartPointIdx = NONE;
			};
			// key point might be just one platform or multiple platforms
			struct KeyPointInfo
			{
				ElevatorCartPointRef left;
				ElevatorCartPointRef right;
				ElevatorCartPointRef back;
				ElevatorCartPointRef forth;

				bool is_empty();
				void set(PlatformDir _dir, int _platformIdx, int _cartPointIdx);
				bool remove(Random::Generator & _random, OUT_ PlatformDir & _dir, OUT_ ElevatorCartPointRef & _ref);
			};

			struct Cart
			{
				// information about cart, to combine two platforms
				CartAxis axis;
				ElevatorCartPointRef minus;
				ElevatorCartPointRef plus;
				Vector2 vrPlacement; // vr / local placement of cart

				Vector3 calculate_lane_vector(PlatformsLinear * _platformsLinear, Array<Framework::Scenery*> const & _platformScenerys, OUT_ bool & _reversePlatform) const;
			};

			struct PathGrid
			{
				bool active = false;
				bool connectsRight = false;
				bool connectsForth = false;
				bool connected = false;
				bool kpiGenerated = false;
				bool checkedConnection = false;
				Optional<int> platformIdx;
				KeyPointInfo kpi;
			};

			Random::Generator random;

			VectorInt2 gridCentreAt = VectorInt2::zero;
			RangeInt2 grid = RangeInt2::empty;

			Array<Platform> platforms;
			Array<Cart> carts;

			// indices
			Array<int> platformsIn;
			Array<int> platformsOut;
			Array<int> platformsLeft;
			Array<int> platformsRight;
			struct ExtraPlatformInfo
			{
				int platformIdx;
				PlatformsLinearExtraPlatform const* extraPlatform;
			};
			Array<ExtraPlatformInfo> platformsExtra;

			struct DoorInfo
			{
				Framework::DoorInRoom* door = nullptr;
				Name createPOI;
				Transform placement = Transform::identity;

				DoorInfo() {}
				explicit DoorInfo(Framework::DoorInRoom* _door): door(_door) {}

			};

			Array<DoorInfo> inDoors;
			Array<DoorInfo> outDoors;
			Array<DoorInfo> leftDoors;
			Array<DoorInfo> rightDoors;

			PlatformsLinearDoorSetupThrough::Type doorSetupThrough = PlatformsLinearDoorSetupThrough::DoorInRooms;

			PlatformsLinearDoorSetupThroughCreatePOIs doorSetupThroughCreatePOIs;

		protected:
			virtual void collect_doors(Framework::Room* _room);
			virtual void place_doors_in_local_space_initially();

			virtual void create_door_platforms();

			virtual void create_platforms_and_connect();

			virtual void place_initial_platforms(ArrayStack<int> & _platformsToPlace);

			virtual void all_platforms_centred_xy();
			virtual void all_platforms_above_zero();
			virtual void align_door_platforms();
			virtual bool separate_door_platforms(int _againstPIdx);

			virtual void add_platforms_to_move(ArrayStack<int> & _platformsToMove, CartAxis _axis, int _pIdx, int _skipPIdx);

			virtual bool adjust_platforms_vertically(); // return true if did anything

			virtual void place_doors(Framework::Game* _game, Framework::Room* _room);

			virtual void calculate_door_placement(DoorInfo* door, Platform const& platform, Vector3 const& _side, OUT_ Transform& _placement, OUT_ Transform& _vrPlacement);

		protected:
			virtual void clear();

			Cart const * find_cart(int _platformIdx, int _cartPointIdx) const;
			CartPoint * find_cart_point(ElevatorCartPointRef const & _ref);
			int find_cart_dir(int _platformIdx, int _cartPointIdx) const;
			int find_platform_on_other_end_of_cart_line(int _platformIdx, int _cartPointIdx) const;
			int find_platform_in_dir(int _fromPlatform, Vector3 const & _dir) const;

			// sub functions

			void connect_platforms(int _aIdx, int _bIdx, CartAxis _aUsingAxis, int _aDir, CartAxis _bUsingAxis, int _bDir, int _addExtraPlatforms = 0);
			bool add_cart_between(int _aIdx, int _bIdx, Optional<CartAxis> const & _usingAxis = NP, int _a2bDir = 0);

			void place_platforms();
			void move_platforms(Framework::Room const* _room, bool _final = false);

			void choose_railings_strategy_and_setup_limits();

			bool put_everything_into_room(Framework::Game* _game, Framework::Room* _room, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext);

			bool validate_placement_and_side_for(CartPoint& _cp, Platform const & _platform);
			bool find_placement_and_side_for(CartPoint& _cp, Platform const & _platform, Optional<VectorInt2> const & _preferredTilePlacement = NP, int _preferredSide = 0, Optional<RangeInt2> const & _amongTilePlacements = NP);
			RangeInt2 calc_range_for(CartPoint const & _cp);
			static PlatformDir opposite(PlatformDir _dir);
			static CartAxis axis_of(PlatformDir _dir);
			static int dir_of(PlatformDir _dir);
			static bool is_entry_x(CartAxis _ca);
			static CartAxis change_verticality(CartAxis _ca);
			static CartAxis change_perpendicular(CartAxis _ca);
			static bool is_vertical(CartAxis _ca);

			void gather_platforms_to_align(ArrayStack<int> & _platformsToMove, CartAxis _alongAxis, CartAxis _alongAxis2, int _skipPIdx = NONE) const;

			void try_to_move(int _pIdx, Vector3 const & _by, bool _immediateMovement = false, int _skipPIdx = NONE);
			void gather_platforms_to_move(ArrayStack<int> & _platformsToMove, CartAxis _alongAxis, CartAxis _alongAxis2, int _skipPIdx = NONE) const;

			void try_to_move_all_in_dir(int _pIdx, Vector3 const & _by);
			void gather_platforms_to_move_all_in_dir(ArrayStack<int> & _platformsToMove, int _pIdx, Vector3 const & _by) const;

			void generate_railings_to(Framework::Room* _room, Framework::Scenery* _platformScenery, Platform* _platform);
			void generate_one_side_railings_to(Framework::Room* _room, Framework::Scenery* _platformScenery, Platform* _platform, bool _x, int _side);

			PlatformsLinear::KeyPointInfo create_key_point(int _platformDirs, VectorInt2 const & _atGrid, Optional<int> const & _usePlatformIdx, bool _allowMatchCartPoint);

			bool check_if_valid() const;

			void find_central_top_platform(Framework::Room const* _room, OUT_ Vector3 * _at, OUT_ Vector3 * _size) const;
			Range calculate_door_size(Framework::Room const * _room) const;

#ifdef AN_DEVELOPMENT_OR_PROFILER
			static tchar get_dir_as_char(CartAxis _axis, int _dir);
			static tchar get_dir_as_char(PlatformDir _dir);
#endif

		private:
			RoomGenerationInfo roomGenerationInfo;
		};
	};

};
