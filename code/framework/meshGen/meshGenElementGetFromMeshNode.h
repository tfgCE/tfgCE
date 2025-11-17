#pragma once

#include "meshGenElement.h"
#include "meshGenValueDef.h"

#include "..\..\core\tags\tagCondition.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	namespace MeshGeneration
	{
		/*
		 *	Gathers all "forEach" to create sets and for each set process elements
		 *	It may either process all or just look for best
		 *
		 *		for each
		 *			mesh node plug (process all)
		 *				+
		 *			mesh node socket (process one/best)
		 *		
		 *	will scan all plugs and for each plug it will go through all sockets but element will be processed once
		 *	it's up to user to store best socket info (or even if it was actually found!, note to clear this info when processing each plug)
		 *
		 *	should have parameters defined
		 */
		class ElementGetFromMeshNode
		: public Element
		{
			FAST_CAST_DECLARE(ElementGetFromMeshNode);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			ElementGetFromMeshNode();
			virtual ~ElementGetFromMeshNode();

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			ValueDef<Name> name;
			TagCondition tagged;
			bool drop;
			struct GetParameter
			{
				TypeId type;
				Name name;
				Name storeAs;
			};
			Name placementAs;
			Name locationAs;
			Array<GetParameter> getParameters; // will be provided up, after reverting stack
		};
	};
};
