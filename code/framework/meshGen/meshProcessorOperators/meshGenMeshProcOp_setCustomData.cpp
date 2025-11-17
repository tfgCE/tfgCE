#include "meshGenMeshProcOp_setCustomData.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenValueDefImpl.inl"

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

REGISTER_FOR_FAST_CAST(SetCustomData);

bool SetCustomData::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= name.load_from_xml(_node, TXT("name"));
	result &= asFloat.load_from_xml(_node, TXT("float"));
	result &= asInt.load_from_xml(_node, TXT("int"));
	result &= asVector3.load_from_xml(_node, TXT("vector3"));
	result &= asVector4.load_from_xml(_node, TXT("vector4"));

	result &= verticesInRange.load_from_xml(_node, TXT("verticesInRange"));

	return result;
}

bool SetCustomData::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	Name cdName = this->name.get_with_default(_context, Name::invalid());

	Optional<Range3> verticesInRange;
	if (this->verticesInRange.is_set()) verticesInRange = this->verticesInRange.get(_context);

	System::VertexFormatUtils::FillCustomDataParams params;
	if (asFloat.is_set()) params.with_float(asFloat.get(_context));
	if (asInt.is_set()) params.with_int(asInt.get(_context));
	if (asVector3.is_set()) params.with_vector3(asVector3.get(_context));
	if (asVector4.is_set()) params.with_vector4(asVector4.get(_context));

	for_every(pIdx, _actOnMeshPartIndices)
	{
		::Meshes::Mesh3DPart* part = _context.get_parts()[*pIdx].get();
		auto& vertexData = part->access_vertex_data();
		auto& vertexFormat = part->get_vertex_format();
		if (auto* cd = vertexFormat.get_custom_data(cdName))
		{
			int cdOffset = vertexFormat.get_custom_data_offset(cdName);
			if (cdOffset != NONE)
			{
				auto numberOfVertices = part->get_number_of_vertices();
				int stride = vertexFormat.get_stride();
				int8* currentVertexData = vertexData.get_data();
				int locOffset = vertexFormat.get_location_offset();
				for (int vrtIdx = 0; vrtIdx < numberOfVertices; ++vrtIdx, currentVertexData += stride)
				{
					if (verticesInRange.is_set())
					{
						Vector3 vLoc = *(Vector3*)(currentVertexData + locOffset);
						if (!verticesInRange.get().does_contain(vLoc))
						{
							continue;
						}
					}
					System::VertexFormatUtils::fill_custom_data(vertexFormat, currentVertexData, stride, *cd, params);
				}
			}
		}
	}

	return result;
}
