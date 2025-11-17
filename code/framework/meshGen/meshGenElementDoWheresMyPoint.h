#pragma once

#include "meshGenElement.h"
#include "meshGenValueDef.h"

#include "..\library\usedLibraryStored.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	class MeshGenerator;

	namespace MeshGeneration
	{
		class ElementDoWheresMyPoint
		: public Element
		{
			FAST_CAST_DECLARE(ElementDoWheresMyPoint);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			ElementDoWheresMyPoint();
			virtual ~ElementDoWheresMyPoint();

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

			override_ bool should_use_random_generator_stack() const { return false /* to allow overriding random generator */; }

		private:
			WheresMyPoint::ToolSet wheresMyPoint;

			bool forPreviewGameOnly = false;
		};
	};
};
