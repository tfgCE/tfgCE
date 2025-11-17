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
		class ElementInfo
		: public Element
		{
			FAST_CAST_DECLARE(ElementInfo);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			virtual ~ElementInfo();

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			ValueDef<Vector3> from;
			ValueDef<Vector3> to;
			WheresMyPoint::ToolSet fromWMP; // allows to modify or set or even add values
			WheresMyPoint::ToolSet toWMP;
			String info;
			Name infoValueParam;
			Optional<Colour> colour;
		};
	};
};
