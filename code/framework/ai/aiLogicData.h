#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\types\name.h"
#include "..\..\core\types\optional.h"
#include "..\..\core\fastCast.h"

namespace Framework
{
	class Library;
	struct LibraryLoadingContext;
	struct LibraryPrepareContext;

	namespace AI
	{
		class Logic;

		class LogicData
		{
			FAST_CAST_DECLARE(LogicData);
			FAST_CAST_END();
		public:
			LogicData();
			virtual ~LogicData();

		public:
			virtual bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
			virtual bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext) { return true; }

		private:
			Optional<Range> autoRareAdvanceIfNotVisible;

			friend class Logic;
		};
	};
};
