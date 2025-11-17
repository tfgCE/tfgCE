#pragma once

#include "..\..\core\containers\array.h"
#include "..\..\core\memory\refCountObject.h"

#include "..\..\core\pieceCombiner\pieceCombiner.h"
#include "..\..\core\tags\tagCondition.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Random
{
	class Generator;
};

namespace Framework
{
	class Library;
	class Region;
	class RegionGenerator;
	class RoomGeneratorInfo;
	struct LibraryLoadingContext;
	struct LibraryPrepareContext;
	struct LibraryStoredRenamer;

	typedef RefCountObjectPtr<RoomGeneratorInfo> RoomGeneratorInfoPtr;

	struct RoomGeneratorInfoOption
	{
		float get_probability_coef() const { return probabilityCoef; }
		RoomGeneratorInfo const * get_room_generator_info() const { return roomGeneratorInfo.get(); }

		bool is_ok_to_be_used() const;

		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _defaultGeneratorType);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		RoomGeneratorInfoOption create_copy() const;
		bool apply_renamer(LibraryStoredRenamer const & _renamer, Library * _library);

	private:
		float probabilityCoef = 1.0f;
		RoomGeneratorInfoPtr roomGeneratorInfo;
		TagCondition gameTagsCondition;
	};

	struct RoomGeneratorInfoSet
	{
		bool load_from_xml(IO::XML::Node const * _containerNode, tchar const * _subContainerName, tchar const * _singleNodeName, LibraryLoadingContext & _lc, tchar const * _defaultGeneratorType = nullptr);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		void clear();
		bool is_empty() const { return infos.is_empty(); }
		RoomGeneratorInfo const * get(Random::Generator & _rg) const;
		RoomGeneratorInfo const * get(int _idx) const;
		int get_count() const { return infos.get_size(); }

		RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>> generate_piece_for_region_generator(REF_ PieceCombiner::Generator<Framework::RegionGenerator>& _generator, Region* _region, REF_ Random::Generator & _randomGenerator) const;

		RoomGeneratorInfoSet create_copy() const;
		bool apply_renamer(LibraryStoredRenamer const & _renamer, Library * _library);

	private:
		Array<RoomGeneratorInfoOption> infos;
	};
};

