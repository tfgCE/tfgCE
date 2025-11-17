#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\types\name.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	class Library;
	struct LibraryLoadingContext;
	struct LibraryPrepareContext;

	namespace AI
	{
		class Social;

		class SocialData
		{
		public:
			bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		private:
			bool neverAgainstAllies = false;
			bool sociopath = false;
			bool endearing = false; // friend of everyone
			Name faction;

			friend class Social;
		};
	};
};
