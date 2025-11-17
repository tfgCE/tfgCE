#pragma once

#include "meshGenElement.h"
#include "meshGenValueDef.h"

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
		class ElementSocket
		: public Element
		{
			FAST_CAST_DECLARE(ElementSocket);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			virtual ~ElementSocket();

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			ValueDef<Name> socketName;
			ValueDef<Name> parentBoneName;
			ValueDef<Name> placementFromMeshNode;
			bool dropMeshNode = false;
			ValueDef<Vector3> location;
			ValueDef<Vector3> relativeLocation;
			ValueDef<Transform> placement;
			ValueDef<Transform> relativePlacement;
		};
	};
};
