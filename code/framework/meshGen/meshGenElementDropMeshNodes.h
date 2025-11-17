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
		class ElementDropMeshNodes
		: public Element
		{
			FAST_CAST_DECLARE(ElementDropMeshNodes);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			virtual ~ElementDropMeshNodes();

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			struct DropMeshNode;
			typedef RefCountObjectPtr<DropMeshNode> DropMeshNodePtr;

			struct DropMeshNode
			: public RefCountObject
			{
				Name name;
				Tags tags;
				WheresMyPoint::ToolSet toolSet; // store values and return true or false

				bool load_from_xml(IO::XML::Node const * _node);
			};
			Array<DropMeshNodePtr> dropMeshNodes;
		};
	};
};
