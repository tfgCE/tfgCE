#pragma once

#include "meshGenElement.h"

#include "..\..\core\wheresMyPoint\iWMPTool.h"

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
		class ElementLoop
		: public Element
		{
			FAST_CAST_DECLARE(ElementLoop);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			virtual ~ElementLoop();

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
#ifdef AN_DEVELOPMENT
			override_ void for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const;
#endif

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			WheresMyPoint::ToolSet startWMP; // initialise loop, starting values
			WheresMyPoint::ToolSet whileWMP; // performed before loop / optional (this or next has to be present)
			Array<RefCountObjectPtr<Element>> loopElements; // perform loop
			WheresMyPoint::ToolSet nextWMP; // performed after loop / optional  (this or while has to be present)
		};
	};
};
