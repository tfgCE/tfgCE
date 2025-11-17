#pragma once

#include "..\..\core\pieceCombiner\pieceCombiner.h"
#include "..\library\libraryStored.h"

namespace Framework
{
	class Library;
	class DoorType;
	class RoomType;
	class RoomPart;
	class DoorInRoom;
	class Region;

	namespace RoomGenerators
	{
		class RoomPieceCombinerInfo;
	};

	namespace RoomPartConnector
	{
		enum Type
		{
			Normal, // places have to match
			Outward, // bi-directional, points outwards (rotated 180' round z)
		};

		Type parse(String const & _string);
	};
	
	/**
	 *	Room generation starts with adding doors as room parts - it might be default room part or one overriden (chosen by "outer" of override_ and "name" of connector)
	 */
	class RoomPieceCombinerGenerator
	{
	public:
		static void setup_generator(PieceCombiner::Generator<Framework::RoomPieceCombinerGenerator>& _generator, RoomGenerators::RoomPieceCombinerInfo const * _roomPieceCombinerGeneratorInfo, Library* _library);
		static bool place_pieces(PieceCombiner::Generator<RoomPieceCombinerGenerator>& _generator, bool _forValidation = false);
		static bool apply_renamer_to(PieceCombiner::Piece<RoomPieceCombinerGenerator> & _piece, LibraryStoredRenamer const & _renamer, Library* _library = nullptr);
		static bool apply_renamer_to(PieceCombiner::Connector<RoomPieceCombinerGenerator> & _connector, LibraryStoredRenamer const & _renamer, Library* _library = nullptr);

	public:
		struct LoadingContext
		{
			LibraryLoadingContext& libraryLoadingContext;
			LoadingContext(LibraryLoadingContext& _libraryLoadingContext) : libraryLoadingContext(_libraryLoadingContext) {}
		};

		struct PreparingContext
		{
			Library* library;
			PreparingContext(Library* _library) : library(_library) {}
		};

		struct Hull
		{
			ConvexHull hull;
			Tags blocks;
			Tags blockedBy;

			bool load_from_xml(IO::XML::Node const * _node, LoadingContext * _context = nullptr);
		};

		struct GenerationContext
		{
			Region* inRegion;
			GenerationContext() : inRegion(nullptr) {}
		};

		struct PieceDef
		{
			UsedLibraryStored<RoomPart> roomPart;
			Array<Hull> hulls;
			bool canBeUsedForPlacementInRoom;
			Transform initialPlacement; // to offset - this is most useful for environments as player may watch them "from outside"

			PieceDef();

			String get_desc() const;

			bool load_from_xml(IO::XML::Node const * _node, LoadingContext * _context = nullptr);
			bool prepare(PieceCombiner::Piece<RoomPieceCombinerGenerator> * _piece, PreparingContext * _context = nullptr);
			bool apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library = nullptr);
		};

		struct ConnectorDef
		: public PieceCombiner::DefaultCustomGenerator::ConnectorDef
		{
			RoomPartConnector::Type type;
			Name socketName; // socket/bone name, if invalid, placement will be used, if valid, placement will be updated
			Transform placement; // relative to piece (identity by default)

			ConnectorDef();

			String get_desc() const;

			bool load_from_xml(IO::XML::Node const * _node, LoadingContext * _context = nullptr);
			bool prepare(PieceCombiner::Piece<RoomPieceCombinerGenerator> * _piece, PieceCombiner::Connector<RoomPieceCombinerGenerator> * _connector, PreparingContext * _context = nullptr);
			bool apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library = nullptr);

			static void get_placement(ConnectorDef const & _a, OUT_ Transform & _aPlacement);
			static void get_placements(ConnectorDef const & _a, OUT_ Transform & _aPlacement, ConnectorDef const & _b, OUT_ Transform & _bPlacement); // get placements taking into account types of connectors
		};

		struct PieceInstanceData
		{
		public:
			DoorInRoom* doorInRoom; // to connect it to proper door inside room - this is done when adding piece at the generation setup (before generation!)
			
			PieceInstanceData();
			PieceInstanceData(DoorInRoom* _dir);
			
			void clear_placement() { placementGroup = 0; }
			bool is_placed() const { return placementGroup != 0; }
			bool is_placed_in(int _placementGroup) const { return placementGroup == _placementGroup; }
			int get_placement_group() const { return placementGroup; }
			Transform const & get_placement() const { return placement; }
			void place_at(Transform const & _placement, int _placementGroup) { placement = _placement; an_assert(_placementGroup!=0); placementGroup = _placementGroup; }
			
			String get_desc() const { return String(TXT("placement:")) + placement.get_translation().to_string() + String(TXT(" rotation:")) + placement.get_orientation().to_rotator().to_string(); }

		private:
			int placementGroup;
			Transform placement;
		};

		struct ConnectorInstanceData
		{
		public:
			ConnectorInstanceData();

			void place_at(Transform const & _placement);

			bool has_provided_placement() const { return hasProvidedPlacement; }
			Transform const & get_placement() const { return placement; }

		private:
			bool hasProvidedPlacement;
			Transform placement;
		};

		struct Utils
		{
		public:
			static bool update_and_validate(PieceCombiner::Generator<RoomPieceCombinerGenerator>& _generator);

		private:
			static bool check_if_dont_intersect(PieceCombiner::Generator<RoomPieceCombinerGenerator>& _generator); // requires pieces to be placed

			static bool check_if_pieces_intersect(PieceCombiner::PieceInstance<RoomPieceCombinerGenerator> const & _a, PieceCombiner::PieceInstance<RoomPieceCombinerGenerator> const & _b);
		};

	private:
		static bool process_placement_for(PieceCombiner::Generator<RoomPieceCombinerGenerator>& _generator, PieceCombiner::PieceInstance<RoomPieceCombinerGenerator>* _piece, bool _forValidation);
	};
};
