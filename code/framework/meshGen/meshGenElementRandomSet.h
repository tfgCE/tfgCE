#pragma once

#include "meshGenElement.h"
#include "meshGenValueDef.h"

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
		class ElementRandomSet
		: public Element
		{
			FAST_CAST_DECLARE(ElementRandomSet);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			virtual ~ElementRandomSet();

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
#ifdef AN_DEVELOPMENT
			override_ void for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const;
#endif

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
#ifdef AN_DEVELOPMENT
			bool debugIterations = false;
#endif
			Array<RefCountObjectPtr<Element>> elements; // per each in set
			ValueDef<Range2> range;
			ValueDef<int> count;
			ValueDef<int> iterationCount;
			ValueDef<float> pushAwayStrength;
			ValueDef<float> pushAwayDistance;
			ValueDef<float> keepTogetherStrength;
			ValueDef<float> pullCloserStrength;
			ValueDef<float> pullCloserDistance;
			Name randomSetValueParam; // it will be using
			Name randomSetIndexParam; // it will be using
		};
	};
};
