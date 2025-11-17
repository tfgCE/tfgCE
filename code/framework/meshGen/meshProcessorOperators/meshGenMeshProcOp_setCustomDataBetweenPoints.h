#pragma once

#include "..\meshGenElementMeshProcessor.h"
#include "..\meshGenValueDef.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace MeshProcessorOperators
		{
			class SetCustomDataBetweenPoints
			: public ElementMeshProcessorOperator
			{
				FAST_CAST_DECLARE(SetCustomDataBetweenPoints);
				FAST_CAST_BASE(ElementMeshProcessorOperator);
				FAST_CAST_END();

				typedef ElementMeshProcessorOperator base;
			public:
				implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
				implement_ bool process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const;

			private:
				ValueDef<Name> name;

				struct Point
				{
					ValueDef<Vector3> at;

					// read without "as"
					ValueDef<float> asFloat;
					ValueDef<int> asInt;
					ValueDef<Vector3> asVector3;
					ValueDef<Vector4> asVector4;

					bool load_from_xml(IO::XML::Node const* _node);
				};

				Point pointA;
				Point pointB;
			};
		};
	};
};
