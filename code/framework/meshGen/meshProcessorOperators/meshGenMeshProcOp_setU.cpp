#include "meshGenMeshProcOp_setU.h"

#include "..\meshGenGenerationContext.h"

#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\system\video\vertexFormatUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;
using namespace MeshProcessorOperators;

//

REGISTER_FOR_FAST_CAST(SetU);

bool SetU::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= replaceU.load_from_xml(_node, TXT("replaceU"));
	result &= replaceURadius.load_from_xml(_node, TXT("replaceURadius"));
	result &= u.load_from_xml(_node, TXT("u"));

	return result;
}

bool SetU::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	Optional<float> replaceU;
	Optional<float> replaceURadius;
	Optional<float> u;
	if (this->replaceU.is_set()) replaceU = this->replaceU.get(_context);
	if (this->replaceURadius.is_set()) replaceURadius = this->replaceURadius.get(_context);
	if (this->u.is_set()) u = this->u.get(_context);

	if (u.is_set())
	{
		for_every(pIdx, _actOnMeshPartIndices)
		{
			::Meshes::Mesh3DPart* part = _context.get_parts()[*pIdx].get();
			auto& vertexData = part->access_vertex_data();
			auto& vertexFormat = part->get_vertex_format();
			if (vertexFormat.get_texture_uv() != ::System::VertexFormatTextureUV::None)
			{
				int vStride = vertexFormat.get_stride();
				int numberOfVertices = part->get_number_of_vertices();
				int8* currentVertexData = vertexData.get_data();
				for (int vIdx = 0; vIdx < numberOfVertices; ++vIdx, currentVertexData += vStride)
				{
					if (replaceU.is_set())
					{
						float vu = ::System::VertexFormatUtils::restore_as_float(currentVertexData + vertexFormat.get_texture_uv_offset(), vertexFormat.get_texture_uv_type());
						if (abs(vu - replaceU.get()) > replaceURadius.get(0.0f))
						{
							// skip
							continue;
						}
					}
					::System::VertexFormatUtils::store(currentVertexData + vertexFormat.get_texture_uv_offset(), vertexFormat.get_texture_uv_type(), u.get());
				}
			}
		}
	}

	return result;
}
