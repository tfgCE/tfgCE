#pragma once

#include "..\meshGenElementMeshProcessor.h"
#include "..\meshGenValueDef.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace MeshProcessorOperators
		{
			class SetCustomDataDistanceBased
			: public ElementMeshProcessorOperator
			{
				FAST_CAST_DECLARE(SetCustomDataDistanceBased);
				FAST_CAST_BASE(ElementMeshProcessorOperator);
				FAST_CAST_END();

				typedef ElementMeshProcessorOperator base;
			public:
				implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
				implement_ bool process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const;

			private:
				ValueDef<Name> name;
				ValueDef<float> distanceMul;
				ValueDef<float> normaliseDistanceTo;

				// adds all on top of each other
				ValueDef<int> useCrossSectionVertexCount; // will group points together, calculate distance between averages to get the value
				ValueDef<Vector3> fromPoint;
			};
		};
	};
};
