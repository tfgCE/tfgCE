#pragma once

#include "..\meshGenElementMeshProcessor.h"
#include "..\meshGenValueDef.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace MeshProcessorOperators
		{
			class Split
			: public ElementMeshProcessorOperator
			{
				FAST_CAST_DECLARE(Split);
				FAST_CAST_BASE(ElementMeshProcessorOperator);
				FAST_CAST_END();

				typedef ElementMeshProcessorOperator base;
			public:
				implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
				implement_ bool process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const;

			private:
				ValueDef<Vector3> splitDir = ValueDef<Vector3>(Vector3::zero);
				ValueDef<Vector3> splitPoint = ValueDef<Vector3>(Vector3::zero);

				ElementMeshProcessorOperatorSet onFront;
				ElementMeshProcessorOperatorSet onBack;
			};
		};
	};
};
