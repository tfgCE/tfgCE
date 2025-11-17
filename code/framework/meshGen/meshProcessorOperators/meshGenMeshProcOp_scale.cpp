#include "meshGenMeshProcOp_scale.h"

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

REGISTER_FOR_FAST_CAST(Scale);

bool Scale::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	scale.load_from_xml(_node, TXT("scale"));
	scaleVector.load_from_xml(_node, TXT("scaleVector"));

	scaleRefPoint.load_from_xml(_node, TXT("refPoint"));

	scalePoint.load_from_xml(_node, TXT("point"));
	scaleDir.load_from_xml(_node, TXT("dir"));
	scaleDistanceFull.load_from_xml(_node, TXT("distanceFull"));
	scaleDistanceFull.load_from_xml(_node, TXT("fullAtDistance"));

	return result;
}

bool Scale::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	if (this->scale.is_set() ||
		this->scaleVector.is_set())
	{
		Vector3 scale = Vector3::one;
		if (this->scale.is_set())
		{
			scale *= this->scale.get(_context);
		}
		if (this->scaleVector.is_set())
		{
			scale *= this->scaleVector.get(_context);
		}
		Optional<Vector3> scaleRefPoint;
		if (this->scaleRefPoint.is_set())
		{
			scaleRefPoint = this->scaleRefPoint.get(_context);
		}
		Optional<Vector3> scalePoint;
		Optional<Vector3> scaleDir;
		Optional<float> scaleDistanceFull;
		// all or none
		if (this->scalePoint.is_set() &&
			this->scaleDir.is_set())
		{
			scalePoint = this->scalePoint.get(_context);
			scaleDir = this->scaleDir.get(_context).normal();
			if (this->scaleDistanceFull.is_set())
			{
				scaleDistanceFull = this->scaleDistanceFull.get(_context);
			}
			else
			{
				scaleDistanceFull = 0.0f;
			}
		}
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

					float useScale = 1.0f;

					if (scalePoint.is_set())
					{
						float alongDir = Vector3::dot(loc - scalePoint.get(), scaleDir.get());
						alongDir = scaleDistanceFull.get() > 0.0f? clamp(alongDir / scaleDistanceFull.get(), 0.0f, 1.0f) : (alongDir > 0.0f? 1.0f : 0.0f);
						useScale *= alongDir;
					}
					if (useScale > 0.0f)
					{
						if (scaleRefPoint.is_set())
						{
							loc -= scaleRefPoint.get();
						}
						loc = lerp(useScale, loc, loc * scale);
						if (scaleRefPoint.is_set())
						{
							loc += scaleRefPoint.get();
						}

						::System::VertexFormatUtils::store_location(vertexData, vIdx, vertexFormat, loc);
					}
				}
			}
		}
	}

	return result;
}
