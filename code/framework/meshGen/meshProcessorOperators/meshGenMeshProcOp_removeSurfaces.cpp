#include "meshGenMeshProcOp_removeSurfaces.h"

#include "..\meshGenGenerationContext.h"

#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\types\vectorPacked.h"
#include "..\..\..\core\system\video\indexFormatUtils.h"
#include "..\..\..\core\system\video\vertexFormatUtils.h"

//

using namespace Framework;
using namespace MeshGeneration;
using namespace MeshProcessorOperators;

//

REGISTER_FOR_FAST_CAST(RemoveSurfaces);

bool RemoveSurfaces::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	u.load_from_xml(_node, TXT("u"));
	uRadius.load_from_xml(_node, TXT("uRadius"));

	return result;
}

bool RemoveSurfaces::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	Optional<float> u;
	float uRadius = 0.0f;
	if (this->u.is_set())
	{
		u = this->u.get(_context);
	}
	if (this->uRadius.is_set())
	{
		uRadius = this->uRadius.get(_context);
	}

	for_every(pIdx, _actOnMeshPartIndices)
	{
		::Meshes::Mesh3DPart* part = _context.get_parts()[*pIdx].get();
		if (part->get_primitive_type() == Meshes::Primitive::Triangle)
		{
			auto& vertexData = part->access_vertex_data();
			auto& vertexFormat = part->get_vertex_format();
			if (vertexFormat.get_texture_uv() != ::System::VertexFormatTextureUV::None)
			{
				int vStride = vertexFormat.get_stride();
				{
					auto& indexData = part->access_element_data();
					auto& indexFormat = part->get_index_format();
					bool indexed = indexFormat.get_element_size() != ::System::IndexElementSize::None;
					Array<int8> newData;
					newData.make_space_for(indexed ? part->access_element_data().get_size() : part->access_vertex_data().get_size());
					for (int tIdx = 0; tIdx < part->get_number_of_tris(); ++tIdx)
					{
						int i[3];
						if (indexed)
						{
							i[0] = System::IndexFormatUtils::get_value(indexFormat, indexData.begin(), tIdx * 3 + 0);
							i[1] = System::IndexFormatUtils::get_value(indexFormat, indexData.begin(), tIdx * 3 + 1);
							i[2] = System::IndexFormatUtils::get_value(indexFormat, indexData.begin(), tIdx * 3 + 2);
						}
						else
						{
							i[0] = tIdx * 3 + 0;
							i[1] = tIdx * 3 + 0;
							i[2] = tIdx * 3 + 0;
						}

						int uCount = 0;
						for_count(int, iIdx, 3)
						{
							int vIdx = i[iIdx];

							float vu = ::System::VertexFormatUtils::restore_as_float(vertexData.begin() + vIdx * vStride + vertexFormat.get_texture_uv_offset(), vertexFormat.get_texture_uv_type());
							if (u.is_set() && abs(vu - u.get()) <= uRadius)
							{
								++uCount;
							}
						}

						bool add = true;
						{
							if (uCount == 3) add = false;
						}

						if (add)
						{
							int8* srcAt = nullptr;
							int amount = 0;
							if (indexed)
							{
								srcAt = &indexData[(tIdx * 3 + 0) * indexFormat.get_stride()];
								amount = 3 * indexFormat.get_stride();
							}
							else
							{
								srcAt = &vertexData[(tIdx * 3 + 0) * vertexFormat.get_stride()];
								amount = 3 * vertexFormat.get_stride();
							}
							while (amount > 0)
							{
								newData.push_back(*srcAt);
								++srcAt;
								--amount;
							}
						}
					}

					if (newData.get_size() != (indexed ? indexData.get_size() : vertexData.get_size()))
					{
						if (indexed)
						{
							auto vertexData = part->get_vertex_data();
							part->update_data(vertexData.get_data(), newData.get_data(), part->get_primitive_type(), part->get_number_of_vertices(), (newData.get_size() / indexFormat.get_stride()), vertexFormat, indexFormat);
							part->prune_vertices();
						}
						else
						{
							part->update_data(newData.get_data(), part->get_primitive_type(), (newData.get_size() / vertexFormat.get_stride()) / 3, vertexFormat);
							part->prune_vertices();
						}
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
