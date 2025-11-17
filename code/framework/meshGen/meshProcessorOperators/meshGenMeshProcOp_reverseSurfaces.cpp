#include "meshGenMeshProcOp_reverseSurfaces.h"

#include "..\meshGenGenerationContext.h"

#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\types\vectorPacked.h"
#include "..\..\..\core\system\video\indexFormatUtils.h"

//

using namespace Framework;
using namespace MeshGeneration;
using namespace MeshProcessorOperators;

//

REGISTER_FOR_FAST_CAST(ReverseSurfaces);

bool ReverseSurfaces::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool ReverseSurfaces::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	for_every(pIdx, _actOnMeshPartIndices)
	{
		::Meshes::Mesh3DPart* part = _context.get_parts()[*pIdx].get();
		if (part->get_primitive_type() == Meshes::Primitive::Triangle)
		{
			auto& indexData = part->access_element_data();
			auto& indexFormat = part->get_index_format();
			for_count(int, eIdx, part->get_number_of_elements() / 3)
			{
				int i0 = System::IndexFormatUtils::get_value(indexFormat, indexData.begin(), eIdx * 3 + 0);
				int i2 = System::IndexFormatUtils::get_value(indexFormat, indexData.begin(), eIdx * 3 + 2);
				System::IndexFormatUtils::set_value(indexFormat, indexData.begin(), eIdx * 3 + 0, i2);
				System::IndexFormatUtils::set_value(indexFormat, indexData.begin(), eIdx * 3 + 2, i0);
			}
			auto& vertexData = part->access_vertex_data();
			auto& vertexFormat = part->get_vertex_format();
			if (vertexFormat.get_normal() == System::VertexFormatNormal::Yes)
			{
				int vStride = vertexFormat.get_stride();
				int8* v = plain_cast<int8>(vertexData.begin());
				int8* nPtr = v + vertexFormat.get_normal_offset();
				int vCount = vertexData.get_size() / vStride;
				for (int i = 0; i < vCount; ++i, nPtr += vStride)
				{
					Vector3 n;
					if (vertexFormat.is_normal_packed())
					{
						n = ((VectorPacked*)nPtr)->get_as_vertex_normal();
					}
					else
					{
						n = *((Vector3*)nPtr);
					}
					n = -n;
					//
					if (vertexFormat.is_normal_packed())
					{
						((VectorPacked*)nPtr)->set_as_vertex_normal(n);
					}
					else
					{
						*((Vector3*)nPtr) = n;
					}
				}
			}
		}
		else
		{
			error_generating_mesh(_instance, TXT("other primitives not supported right now"));
			result = false;
		}
	}

	return result;
}
