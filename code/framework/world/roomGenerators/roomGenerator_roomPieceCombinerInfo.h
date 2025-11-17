#pragma once

#include "..\..\library\libraryStored.h"
#include "..\..\library\libraryStoredReplacer.h"

#include "..\roomGeneratorInfo.h"
#include "..\roomPieceCombinerGenerator.h"

namespace Framework
{
	class RoomPartInstance;

	namespace RoomGenerators
	{
		class RoomPieceCombinerInfo
		: public RoomGeneratorInfo
		{
			FAST_CAST_DECLARE(RoomPieceCombinerInfo);
			FAST_CAST_BASE(RoomGeneratorInfo);
			FAST_CAST_END();

			typedef RoomGeneratorInfo base;
		public:
			struct Set
			{
				float chance;
				Array<UsedLibraryStored<RoomPart>> includeInstanceRoomParts;
				bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
				bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
				bool apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library = nullptr);
			};

			struct DoorOverride
			{
				Name outerName;
				UsedLibraryStored<RoomPart> roomPart;
				bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
				bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
				bool apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library = nullptr);
			};

			RoomPieceCombinerInfo();
			virtual ~RoomPieceCombinerInfo();

		public: // RoomGeneratorInfo
			implement_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			implement_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
			implement_ bool apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library = nullptr);
			implement_ RoomGeneratorInfoPtr create_copy() const;

			implement_ bool generate(Room* _room, bool _subGenerator, REF_ RoomGeneratingContext& _roomGeneratingContext) const;

		public:
			PieceCombiner::GenerationRules<RoomPieceCombinerGenerator> const * get_generation_rules() const { return &generationRules; }
			Array<UsedLibraryStored<RoomPart>> const & get_included_room_parts() const { return includedRoomParts; }
			Array<RefCountObjectPtr<TagCondition>> const & get_included_room_part_tag_conditions() const { return includedRoomPartsTagConditions; }
			Array<RefCountObjectPtr<PieceCombiner::Piece<RoomPieceCombinerGenerator>>> const & get_room_pieces() const { return roomPieces; }

			Set const * choose_set(Random::Generator & _generator) const;
			RoomPart const * get_door_override(Name const & _outerName) const;

			PieceCombiner::Connector<RoomPieceCombinerGenerator> const * find_outer_connector(Name const & _outerName) const;

		private:
			PieceCombiner::GenerationRules<RoomPieceCombinerGenerator> generationRules;
			Array<RefCountObjectPtr<PieceCombiner::Piece<RoomPieceCombinerGenerator>>> roomPieces;
			Array<RefCountObjectPtr<PieceCombiner::Connector<RoomPieceCombinerGenerator>>> outerRoomConnectors; // how outside connectors (for example, as room part) are understood as normal (outer) connectors here
			Array<UsedLibraryStored<RoomPart>> includedRoomParts; // included room types
			Array<RefCountObjectPtr<TagCondition>> includedRoomPartsTagConditions; // shared between template and created from template
			List<Set> obligatorySets; // sets that are always included
			List<Set> sets;
			Array<DoorOverride> doorOverrides; // if we connect to room through a specific door, we may use room part of our choice to handle that kind of door (connector) we use name of connector that was used for this room as region piece

			bool load_room_generation_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);

			struct GeneratedRoomPart
			{
				PieceCombiner::PieceInstance<Framework::RoomPieceCombinerGenerator>* piece; // void to make it less stressful for compilation (including headers etc)
				RoomPartInstance* roomPartInstance;

				GeneratedRoomPart() {}
				GeneratedRoomPart(PieceCombiner::PieceInstance<Framework::RoomPieceCombinerGenerator>* _piece, RoomPartInstance* _roomPartInstance) : piece(_piece), roomPartInstance(_roomPartInstance) {}
			};
			bool generate_sub_for(Room* _room, GeneratedRoomPart* _generatedRoomPart, int _randomOffset) const;
			bool process_successful_generator(Room* _room, REF_ PieceCombiner::Generator<Framework::RoomPieceCombinerGenerator> & _generator) const;

		};
	};
};
