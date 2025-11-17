#pragma once

#include "..\meshGenElementMeshProcessor.h"
#include "..\meshGenValueDef.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace MeshProcessorOperators
		{
			class SetCustomData
			: public ElementMeshProcessorOperator
			{
				FAST_CAST_DECLARE(SetCustomData);
				FAST_CAST_BASE(ElementMeshProcessorOperator);
				FAST_CAST_END();

				typedef ElementMeshProcessorOperator base;
			public:
				implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
				implement_ bool process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const;

			private:
				ValueDef<Name> name;

				// read without "as"
				ValueDef<float> asFloat;
				ValueDef<int> asInt;
				ValueDef<Vector3> asVector3;
				ValueDef<Vector4> asVector4;

				ValueDef<Range3> verticesInRange; // by default we work only on mesh parts (whole!) this is to alow setting only on some
			};
		};
	};
};
