#pragma once

#include "..\roomGeneratorBase.h"

namespace TeaForGodEmperor
{
	namespace RoomGenerators
	{
		class UseMeshGenerator;

		/**
		 *	Empty room. Can be anything, but most of the time it is a stand alone room (vista, for airships), that's why it uses Base and is a room generator (not just room element)
		 */
		class Empty
		: public Base
		{
			FAST_CAST_DECLARE(Empty);
			FAST_CAST_BASE(Base);
			FAST_CAST_END();

			typedef Base base;
		public:
			Empty();
			virtual ~Empty();

		public: // RoomGeneratorInfo
			implement_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			implement_ Framework::RoomGeneratorInfoPtr create_copy() const;

			implement_ bool internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const;
		};

	};

};
