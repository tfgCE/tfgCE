#pragma once

#include "meshGenElement.h"

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
		class ElementNamedCheckpoint
		: public Element
		{
			FAST_CAST_DECLARE(ElementNamedCheckpoint);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			virtual ~ElementNamedCheckpoint();

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
#ifdef AN_DEVELOPMENT
			override_ void for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const;
#endif

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			Name start;
			Name end; // even if not started
		};
	};
};
