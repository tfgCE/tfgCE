#include "meshGenMeshProcOp_dropVerticesOnPlane.h"

#include "..\meshGenGenerationContext.h"

#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\system\video\vertexFormatUtils.h"

//

using namespace Framework;
using namespace MeshGeneration;
using namespace MeshProcessorOperators;

//

REGISTER_FOR_FAST_CAST(DropVerticesOnPlane);

bool DropVerticesOnPlane::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	cutDir.load_from_xml_child_node(_node, TXT("dir"));
	cutPoint.load_from_xml_child_node(_node, TXT("point"));

	if (cutDir.is_value_set())
	{
		if (cutDir.get_value().is_zero())
		{
			cutDir.set(Vector3::xAxis);
		}
	}

	adjustNormals.load_from_xml(_node, TXT("adjustNormals"));

	return result;
}

bool DropVerticesOnPlane::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	Plane cutPlane = Plane(cutDir.get(_context).normal(), cutPoint.get(_context));
	bool adjustNormals = this->adjustNormals.get(_context);

	for_every(pIdx, _actOnMeshPartIndices)
	{
		::Meshes::Mesh3DPart* part = _context.get_parts()[*pIdx].get();

		auto& vertexFormat = part->get_vertex_format();
		if (vertexFormat.get_location() == ::System::VertexFormatLocation::XYZ)
		{
			int numberOfVertices = part->get_number_of_vertices();
			auto* vertexData = part->access_vertex_data().get_data();
			for (int vIdx = 0; vIdx < numberOfVertices; ++vIdx)
			{
				Vector3 loc = ::System::VertexFormatUtils::restore_location(vertexData, vIdx, vertexFormat);
				float inFront = cutPlane.get_in_front(loc);
				if (inFront < 0.0f)
				{
					loc = loc - inFront * cutPlane.get_normal();
					if (adjustNormals && vertexFormat.get_normal() == ::System::VertexFormatNormal::Yes)
					{
						::System::VertexFormatUtils::store_normal(vertexData, vIdx, vertexFormat, -cutPlane.get_normal());
					}
				}
				::System::VertexFormatUtils::store_location(vertexData, vIdx, vertexFormat, loc);
			}
		}
	}

	return result;
}
