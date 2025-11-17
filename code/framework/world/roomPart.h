#pragma once

#include "doorHole.h"
#include "pointOfInterest.h"

#include "..\library\libraryStored.h"

#include "roomPieceCombinerGenerator.h"

namespace Framework
{
	class DoorType;
	class Mesh;

	/**
	 *	?
	 */
	class RoomPart
	: public LibraryStored
	{
		FAST_CAST_DECLARE(RoomPart);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		RoomPart(Library * _library, LibraryName const & _name);

		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ void prepare_to_unload();

		PieceCombiner::Piece<RoomPieceCombinerGenerator>* get_as_room_piece() { return roomPiece.get(); }
		PieceCombiner::Piece<RoomPieceCombinerGenerator> const * get_as_room_piece() const { return roomPiece.get(); }

		RoomGenerators::RoomPieceCombinerInfo const* get_sub_room_piece_combiner_generator_info() const { return subRoomPieceCombinerGeneratorInfo; }

	protected: friend class DoorType;
		~RoomPart();

	private:
		struct Element
		{
			UsedLibraryStored<Mesh> mesh;
			Transform placement;
			Array<PointOfInterestPtr> pois;

			Element();
			bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		};

		RefCountObjectPtr<PieceCombiner::Piece<RoomPieceCombinerGenerator>> roomPiece;
		Array<Element> elements;
		Array<PointOfInterestPtr> pois;

		RoomGenerators::RoomPieceCombinerInfo* subRoomPieceCombinerGeneratorInfo; // this allows room part to generate additional parts, this is different to outer/external in pieceCombiner as it is completely different generation (it's more similar to region outer connector)

		friend class RoomPartInstance;
	};
};
