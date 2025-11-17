#pragma once

#include "..\meshGenElementMeshProcessor.h"
#include "..\meshGenValueDef.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace MeshProcessorOperators
		{
			class RemoveSurfaces
			: public ElementMeshProcessorOperator
			{
				FAST_CAST_DECLARE(RemoveSurfaces);
				FAST_CAST_BASE(ElementMeshProcessorOperator);
				FAST_CAST_END();

				typedef ElementMeshProcessorOperator base;
			public:
				implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
				implement_ bool process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const;

			private:
				ValueDef<float> u; // best to use with 0 (otherwise we may get floating point inaccuracy)
				ValueDef<float> uRadius = 0.0001f; // because of floating point inaccuracy
			};
		};
	};
};
