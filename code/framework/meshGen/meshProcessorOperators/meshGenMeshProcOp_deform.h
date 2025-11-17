#pragma once

#include "..\meshGenElementMeshProcessor.h"
#include "..\meshGenValueDef.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace MeshProcessorOperators
		{
			class Deform
			: public ElementMeshProcessorOperator
			{
				FAST_CAST_DECLARE(Deform);
				FAST_CAST_BASE(ElementMeshProcessorOperator);
				FAST_CAST_END();

				typedef ElementMeshProcessorOperator base;
			public:
				implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
				implement_ bool process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const;

			private:
				ValueDef<float> voxelSize = 0.1f; // uses pseudo random values based on coordinates taken from voxels
				ValueDef<float> deformDist;
				ValueDef<Vector3> deformVector; // deformation takes place only in this direction
				ValueDef<Vector3> deformNormal; // deformation takes place not in this direction
			
				// when these three are combined, we determine how much do we deform in the direction of a single plane
				ValueDef<Vector3> deformPoint;
				ValueDef<Vector3> deformDir;
				ValueDef<float> deformDistanceFull; // might be skipped, treated as 0 then
				ValueDef<float> deformDistanceNone; // opposite of full, useful if combined with point
			};
		};
	};
};
