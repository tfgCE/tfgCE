#pragma once

#include "..\meshGenElementMeshProcessor.h"
#include "..\meshGenValueDef.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace MeshProcessorOperators
		{
			class Select
			: public ElementMeshProcessorOperator
			{
				FAST_CAST_DECLARE(Select);
				FAST_CAST_BASE(ElementMeshProcessorOperator);
				FAST_CAST_END();

				typedef ElementMeshProcessorOperator base;
			public:
				implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
				implement_ bool process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const;

			private:
				ValueDef<float> u; // best to use with 0 (otherwise we may get floating point inaccuracy)
				ValueDef<float> uRadius = 0.0001f; // because of floating point inaccuracy

				ValueDef<Range3> range; // all triangles touching/intersecting the range

				struct BoneInfo
				{
					ValueDef<Name> bone;
					bool andAllBelow = false;
				};
				Array<BoneInfo> bones;

				ElementMeshProcessorOperatorSet onNotSelected;
				ElementMeshProcessorOperatorSet onSelected;
			};
		};
	};
};
