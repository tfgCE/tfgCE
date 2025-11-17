#pragma once

#include "..\meshGenElementMeshProcessor.h"
#include "..\meshGenValueDef.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace MeshProcessorOperators
		{
			/**
			 *	Finds separate patches (triangles have to share edge (just location)
			 */
			class NumberCustomData
			: public ElementMeshProcessorOperator
			{
				FAST_CAST_DECLARE(NumberCustomData);
				FAST_CAST_BASE(ElementMeshProcessorOperator);
				FAST_CAST_END();

				typedef ElementMeshProcessorOperator base;
			public:
				implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
				implement_ bool process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const;

			private:
				ValueDef<Name> name; // always as int

				ValueDef<float> u; // best to use with 0 (otherwise we may get floating point inaccuracy)
				ValueDef<float> uRadius = 0.0001f; // because of floating point inaccuracy

				ValueDef<Range3> verticesInRange; // by default we work only on mesh parts (whole!) this is to alow setting only on some
			};
		};
	};
};
