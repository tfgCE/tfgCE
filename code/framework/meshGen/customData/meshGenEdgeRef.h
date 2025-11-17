#pragma once

#include "..\..\library\customLibraryDatas.h"

#include "..\meshGenParam.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace CustomData
		{
			class Edge;

			struct EdgeRef
			{
			public:
				Edge* get(MeshGeneration::GenerationContext const & _context) const;
				bool load_from_xml(IO::XML::Node const * _node, tchar const * _useEdgeAttr, tchar const * _ownEdgeChild /* may be nullptr */, LibraryLoadingContext & _lc);
				bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

				MeshGenParam<LibraryName> const & get_use_edge() const { return useEdge; }
			private:
				RefCountObjectPtr<Edge> edge; // if "useEdge" is null, this is own edge
				MeshGenParam<LibraryName> useEdge;
			};
		};
	};
};
