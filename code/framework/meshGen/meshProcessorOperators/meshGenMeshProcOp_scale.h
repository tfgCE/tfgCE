#pragma once

#include "..\meshGenElementMeshProcessor.h"
#include "..\meshGenValueDef.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace MeshProcessorOperators
		{
			class Scale
			: public ElementMeshProcessorOperator
			{
				FAST_CAST_DECLARE(Scale);
				FAST_CAST_BASE(ElementMeshProcessorOperator);
				FAST_CAST_END();

				typedef ElementMeshProcessorOperator base;
			public:
				implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
				implement_ bool process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const;

			private:
				ValueDef<float> scale;
				ValueDef<Vector3> scaleVector;
				ValueDef<Vector3> scaleRefPoint;

				// when these three are combined, we determine how much do we scale in the direction of a single plane
				ValueDef<Vector3> scalePoint;
				ValueDef<Vector3> scaleDir;
				ValueDef<float> scaleDistanceFull; // might be skipped, treated as 0 then
			};
		};
	};
};
