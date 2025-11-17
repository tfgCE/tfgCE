#include "meshGenMeshProcOp_weld.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenValueDefImpl.inl"

#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"

//

using namespace Framework;
using namespace MeshGeneration;
using namespace MeshProcessorOperators;

//

REGISTER_FOR_FAST_CAST(Weld);

bool Weld::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	withinBox.load_from_xml_child_node(_node, TXT("withinBox"));
	withinSphere.load_from_xml_child_node(_node, TXT("withinSphere"));
	dist.load_from_xml(_node, TXT("dist"));

	if (!dist.is_set())
	{
		error_loading_xml(_node, TXT("not all parameters for weld were defined"));
		result = false;
	}

	return result;
}

bool Weld::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	if (dist.is_set())
	{
		float weldDist = dist.get(_context);
		Optional<Range3> withinBox;
		Optional<Sphere> withinSphere;
		if (this->withinBox.is_set())
		{
			withinBox = this->withinBox.get(_context);
		}
		if (this->withinSphere.is_set())
		{
			withinSphere = this->withinSphere.get(_context);
		}
		for_every(pIdx, _actOnMeshPartIndices)
		{
			::Meshes::Mesh3DPart* part = _context.get_parts()[*pIdx].get();
			auto& _vertexFormat = part->get_vertex_format();
			if (_vertexFormat.get_location() == ::System::VertexFormatLocation::XYZ)
			{
				int _numberOfVertices = part->get_number_of_vertices();
				ARRAY_PREALLOC_SIZE(bool, wasWeld, _numberOfVertices);
				ARRAY_PREALLOC_SIZE(int, weldIndices, _numberOfVertices);
				wasWeld.set_size(_numberOfVertices);
				for_every(w, wasWeld)
				{
					*w = false;
				}
				auto& _vertexData = part->access_vertex_data();
				auto& _vertexFormat = part->get_vertex_format();
				int stride = _vertexFormat.get_stride();
				for (int mainVrtIdx = 0; mainVrtIdx < _numberOfVertices; ++mainVrtIdx)
				{
					weldIndices.clear();
					weldIndices.push_back(mainVrtIdx);
					Vector3 centre = *(Vector3*)(_vertexData.get_data() + mainVrtIdx * stride + _vertexFormat.get_location_offset());
					if ((withinBox.is_set() && !withinBox.get().does_contain(centre)) ||
						(withinSphere.is_set() && !withinSphere.get().does_contain(centre)))
					{
						continue;
					}
					Vector3 centreSum = centre;
					float centreWeight = 1.0f;
					while (true)
					{
						bool addedAnything = false;
						for (int checkVrtIdx = mainVrtIdx + 1; checkVrtIdx < _numberOfVertices; ++checkVrtIdx)
						{
							if (wasWeld[checkVrtIdx] || weldIndices.does_contain(checkVrtIdx))
							{
								continue;
							}
							Vector3 point = *(Vector3*)(_vertexData.get_data() + checkVrtIdx * stride + _vertexFormat.get_location_offset());
							if ((point - centre).length() <= weldDist)
							{
								centreSum += point;
								centreWeight += 1.0f;
								centre = centreSum / centreWeight;
								weldIndices.push_back(checkVrtIdx);
								addedAnything = true;
							}
						}
						if (!addedAnything)
						{
							break;
						}
					}

					// weld if more than one
					if (weldIndices.get_size() > 1)
					{
						for_every(wi, weldIndices)
						{
							Vector3* loc = (Vector3*)(_vertexData.get_data() + (*wi) * stride + _vertexFormat.get_location_offset());
							*loc = centre;
							wasWeld[*wi] = true;
						}
					}
				}
			}
		}
	}

	return result;
}
