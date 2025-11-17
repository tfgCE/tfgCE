#pragma once

#include "..\meshGenElementMeshProcessor.h"
#include "..\meshGenValueDef.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace MeshProcessorOperators
		{
			class ScaleVerticesOnPlane
			: public ElementMeshProcessorOperator
			{
				FAST_CAST_DECLARE(ScaleVerticesOnPlane);
				FAST_CAST_BASE(ElementMeshProcessorOperator);
				FAST_CAST_END();

				typedef ElementMeshProcessorOperator base;
			public:
				implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
				implement_ bool process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const;

			private:
				ValueDef<Vector3> cutDir = ValueDef<Vector3>(Vector3::zero);
				ValueDef<Vector3> cutPoint = ValueDef<Vector3>(Vector3::zero);
				ValueDef<float> scale = 1.0f;
				ValueDef<float> margin = 0.001f;
				ValueDef<bool> adjustNormals = false;
			};
		};
	};
};
