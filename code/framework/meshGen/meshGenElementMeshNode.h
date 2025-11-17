#pragma once

#include "meshGenElement.h"
#include "meshGenValueDef.h"

#include "..\..\core\tags\tag.h"

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
		class ElementMeshNode
		: public Element
		{
			FAST_CAST_DECLARE(ElementMeshNode);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			ElementMeshNode();
			virtual ~ElementMeshNode();

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;
			override_ bool post_process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			ValueDef<Name> meshNodeName;
			Tags meshNodeTags;
			SimpleVariableStorage meshNodeVariables; /* to copy from existing, just add empty entry */
		};
	};
};
