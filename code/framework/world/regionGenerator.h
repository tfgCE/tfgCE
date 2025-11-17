#pragma once

#include "..\..\core\pieceCombiner\pieceCombiner.h"
#include "..\library\libraryStored.h"

namespace Framework
{
	class Library;
	class Door;
	class DoorType;
	class RoomType;
	class RegionType;
	class Region;
	class SubWorld;

	/**
	 *	Inner region connects with outer region through:
	 *		connectors of inner region through "outer"
	 *		connectors of outer region through "name"
	 */
	class RegionGenerator
	{
	public:
		static void setup_generator(PieceCombiner::Generator<Framework::RegionGenerator>& _generator, Region * _region, Library* _library, Random::Generator & _useRG);
		// _generateRooms - if working with vr, it is advised to not generate rooms via generator but call _ready_for_game on first/starting/closest to player room as it will both generate rooms and add vr corridors if required
		static bool generate_and_add_to_sub_world(SubWorld* _inSubWorld, Region* _inRegion, RegionType const * _regionType, Optional<Random::Generator> const& _rgForRegion, Random::Generator* _randomGenerator,
			std::function<void(PieceCombiner::Generator<Framework::RegionGenerator> & _generator, Framework::Region* _forRegion)> _setup_generator,
			std::function<void(Region* _region)> _setup_region, OUT_ Region** _regionPtr = nullptr, bool _generateRooms = true);

		static bool apply_renamer_to(PieceCombiner::Piece<RegionGenerator> & _piece, LibraryStoredRenamer const & _renamer, Library* _library = nullptr);
		static bool apply_renamer_to(PieceCombiner::Connector<RegionGenerator> & _connector, LibraryStoredRenamer const & _renamer, Library* _library = nullptr);

	public:
		struct LoadingContext
		{
			LibraryLoadingContext& libraryLoadingContext;
			LoadingContext(LibraryLoadingContext& _libraryLoadingContext) : libraryLoadingContext(_libraryLoadingContext) {}

			LoadingContext* temp_ptr() { return this; }
		};

		struct PreparingContext
		{
			Library* library;
			PreparingContext(Library* _library) : library(_library) {}

			PreparingContext* temp_ptr() { return this; }
		};

		struct GenerationContext
		{
			Tags generationTags; // to choose the right set from RegionType
		};

		struct PieceDef
		{
			String desc;
			UsedLibraryStored<RoomType> roomType;
			UsedLibraryStored<RegionType> regionType;
			
			String get_desc() const;

			bool load_from_xml(IO::XML::Node const * _node, LoadingContext * _context = nullptr);
			bool prepare(PieceCombiner::Piece<RegionGenerator> * _piece, PreparingContext * _context = nullptr);
			bool apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library = nullptr);
		};

		struct ConnectorDef
		{
			UsedLibraryStored<DoorType> doorType;
			Tags doorInRoomTags;

			String get_desc() const;

			bool load_from_xml(IO::XML::Node const * _node, LoadingContext * _context = nullptr);
			bool prepare(PieceCombiner::Piece<RegionGenerator> * _piece, PieceCombiner::Connector<RegionGenerator> * _connector, PreparingContext * _context = nullptr);
			bool apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library = nullptr);

			bool is_valid_cycle_connector() const { return doorType.get() != nullptr; } // if can create cycle/completion
			bool are_sides_symmetrical() const { return true; } // if symmetrical side A can join with side A - rule of thumb is to always join B to A
			bool can_join_to(ConnectorDef const & _other) const { return doorType == _other.doorType; } // if can link with it - some can be two sided
		};

		struct PieceInstanceData
		{
			Optional<Random::Generator> useRGForCreatedPiece;

			PieceInstanceData() {}
			PieceInstanceData(Random::Generator const& _rg) : useRGForCreatedPiece(_rg) {}

			String get_desc() const { return String::empty(); }
		};

		struct ConnectorInstanceData
		{
			Door* door; // to have connector representing door in world - this happens in generate_and_add_to_sub_world
			// (or might be in future - when connector is created)

			ConnectorInstanceData() : door(nullptr) {}
			ConnectorInstanceData(Door* _door) : door(_door) {}
		};

		struct Utils
		{
			static bool update_and_validate(PieceCombiner::Generator<RegionGenerator>& _generator) { return true; }
		};

	};

	struct RegionGeneratorContextOwner
	: public WheresMyPoint::IOwner
	{
		FAST_CAST_DECLARE(RegionGeneratorContextOwner);
		FAST_CAST_BASE(IOwner);
		FAST_CAST_END();

	public:
		Region* region = nullptr;
		PieceCombiner::Generator<Framework::RegionGenerator>* generator = nullptr;

		RegionGeneratorContextOwner(Region* _region, PieceCombiner::Generator<Framework::RegionGenerator>* _generator) : region(_region), generator(_generator) {}

	public: // WheresMyPoint::IOwner
		override_ bool store_value_for_wheres_my_point(Name const& _byName, WheresMyPoint::Value const& _value);
		override_ bool restore_value_for_wheres_my_point(Name const& _byName, OUT_ WheresMyPoint::Value& _value) const;
		override_ bool store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to);
		override_ bool rename_value_forwheres_my_point(Name const& _from, Name const& _to);
		override_ bool store_global_value_for_wheres_my_point(Name const& _byName, WheresMyPoint::Value const& _value);
		override_ bool restore_global_value_for_wheres_my_point(Name const& _byName, OUT_ WheresMyPoint::Value& _value) const;

		override_ WheresMyPoint::IOwner* get_wmp_owner();
	};
};
