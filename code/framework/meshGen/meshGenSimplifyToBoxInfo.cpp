#include "meshGenSimplifyToBoxInfo.h"

#include "meshGenGenerationContext.h"

#include "meshGenUtils.h"

#include "..\library\library.h"

#include "..\..\core\mesh\builders\mesh3dBuilder_IPU.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;

//

DEFINE_STATIC_NAME(autoU);
DEFINE_STATIC_NAME(materialIndex);

//

bool SimplifyToBoxInfo::load_from_child_node_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _childNodeName)
{
	bool result = true;

	for_every(node, _node->children_named(_childNodeName))
	{
		forLOD.load_from_xml(node, TXT("forLOD"));
		result &= u.load_from_xml(node, TXT("u"));
		result &= skinToBone.load_from_xml(node, TXT("skinToBone"));
		result &= scale.load_from_xml(node, TXT("scale"));
	}
	
	return result;
}

bool SimplifyToBoxInfo::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	forLOD.load_from_xml(_node, TXT("forLOD"));
	result &= u.load_from_xml(_node, TXT("u"));
	result &= skinToBone.load_from_xml(_node, TXT("skinToBone"));
	result &= scale.load_from_xml(_node, TXT("scale"));

	return result;
}

bool SimplifyToBoxInfo::process(GenerationContext& _context, ElementInstance& _instance, int _meshPartsSoFar, bool _requiresDefinedForLOD) const
{
	bool result = true;

	if ((forLOD.is_empty() && !_requiresDefinedForLOD) ||
		(!forLOD.is_empty() && forLOD.does_contain(_context.get_for_lod())))
	{
		Range3 range = Range3::empty;
		Optional<int> skinToBoneIdx;
		Optional<int> materialIndex;
		Optional<Vector3> useScale;
		if (skinToBone.is_set())
		{
			int foundBone = _context.find_bone_index(skinToBone.get(_context));
			if (foundBone != NONE)
			{
				skinToBoneIdx = foundBone;
			}
		}
		if (scale.is_set())
		{
			useScale = scale.get(_context);
		}
		for_range(int, i, _meshPartsSoFar, _context.get_parts().get_size() - 1)
		{
			auto const& part = _context.get_parts()[i].get();
			part->for_every_triangle_and_simple_skinning([&range, &skinToBoneIdx](Vector3 const& _a, Vector3 const& _b, Vector3 const& _c, int _skinToBoneIdx)
			{
				range.include(_a);
				range.include(_b);
				range.include(_c);
				if (! skinToBoneIdx.is_set() && _skinToBoneIdx != NONE)
				{
					skinToBoneIdx = _skinToBoneIdx;
				}
			});
			if (! materialIndex.is_set())
			{
				materialIndex = _context.get_material_index_for_part(part);
			}
			part->clear(); // empty
		}

		if (! range.is_empty())
		{
			if (useScale.is_set())
			{
				Vector3 centre = range.centre();
				range.x.min = centre.x + (range.x.min - centre.x) * useScale.get().x;
				range.x.max = centre.x + (range.x.max - centre.x) * useScale.get().x;
				range.y.min = centre.y + (range.y.min - centre.y) * useScale.get().y;
				range.y.max = centre.y + (range.y.max - centre.y) * useScale.get().y;
				range.z.min = centre.z + (range.z.min - centre.z) * useScale.get().z;
				range.z.max = centre.z + (range.z.max - centre.z) * useScale.get().z;
			}

			Array<int8> elementsData;
			Array<int8> vertexData;

			::Meshes::Builders::IPU ipu;

			MeshGeneration::load_ipu_parameters(ipu, _context);

			float useU = 0.0f;
			if (u.is_set())
			{
				useU = u.get(_context);
			}

			{
				ipu.add_point(range.get_at(Vector3(0.0f, 0.0f, 0.0f)));
				ipu.add_point(range.get_at(Vector3(0.0f, 1.0f, 0.0f)));
				ipu.add_point(range.get_at(Vector3(1.0f, 1.0f, 0.0f)));
				ipu.add_point(range.get_at(Vector3(1.0f, 0.0f, 0.0f)));

				ipu.add_point(range.get_at(Vector3(0.0f, 0.0f, 1.0f)));
				ipu.add_point(range.get_at(Vector3(0.0f, 1.0f, 1.0f)));
				ipu.add_point(range.get_at(Vector3(1.0f, 1.0f, 1.0f)));
				ipu.add_point(range.get_at(Vector3(1.0f, 0.0f, 1.0f)));
			}

			{
				// bottom, top
				ipu.add_quad(useU, 0, 3, 2, 1);
				ipu.add_quad(useU, 4, 5, 6, 7);

				// left, right
				ipu.add_quad(useU, 0, 1, 5, 4);
				ipu.add_quad(useU, 3, 7, 6, 2);

				// front, back
				ipu.add_quad(useU, 0, 4, 7, 3);
				ipu.add_quad(useU, 2, 6, 5, 1);
			}

			//

			// prepare data for vertex and index formats, so further modifications may be applied and mesh can be imported
			ipu.prepare_data(_context.get_vertex_format(), _context.get_index_format(), vertexData, elementsData);

			int vertexCount = vertexData.get_size() / _context.get_vertex_format().get_stride();
			int elementsCount = elementsData.get_size() / _context.get_index_format().get_stride();

			//

			if (vertexCount)
			{
				// create part
				Meshes::Mesh3DPart* part = _context.get_generated_mesh()->load_part_data(vertexData.get_data(), elementsData.get_data(), Meshes::Primitive::Triangle, vertexCount, elementsCount, _context.get_vertex_format(), _context.get_index_format());

#ifdef AN_DEVELOPMENT
				part->debugInfo = String::printf(TXT("simplified to box @ %S"), _instance.get_element()->get_location_info().to_char());
#endif

				if (skinToBoneIdx.is_set())
				{
					int8* currentVertexData = part->access_vertex_data().get_data();
					int stride = _context.get_vertex_format().get_stride();
					int skinningOffset = _context.get_vertex_format().get_skinning_element_count() > 1 ? _context.get_vertex_format().get_skinning_indices_offset() : _context.get_vertex_format().get_skinning_index_offset();
					int bonesRequired = _context.get_vertex_format().get_skinning_element_count();
					if (bonesRequired > 4)
					{
						error_generating_mesh(_instance, TXT("no more than 4 bones supported!"));
						todo_important(TXT("add support for more bones"));
						bonesRequired = 4;
					}
					for (int vrtIdx = 0; vrtIdx < part->get_number_of_vertices(); ++vrtIdx, currentVertexData += stride)
					{
						if (_context.get_vertex_format().get_skinning_index_type() == ::System::DataFormatValueType::Uint8)
						{
							{
								uint8* out = (uint8*)(currentVertexData + skinningOffset);
								for (int idx = 0; idx < bonesRequired; ++idx, ++out)
								{
									*out = idx == 0? skinToBoneIdx.get() : 0;
								}
							}
						}
						else if (_context.get_vertex_format().get_skinning_index_type() == ::System::DataFormatValueType::Uint16)
						{
							{
								uint16* out = (uint16*)(currentVertexData + skinningOffset);
								for (int idx = 0; idx < bonesRequired; ++idx, ++out)
								{
									*out = idx == 0 ? skinToBoneIdx.get() : 0;
								}
							}
						}
						else if (_context.get_vertex_format().get_skinning_index_type() == ::System::DataFormatValueType::Uint32)
						{
							{
								uint32* out = (uint32*)(currentVertexData + skinningOffset);
								for (int idx = 0; idx < bonesRequired; ++idx, ++out)
								{
									*out = idx == 0 ? skinToBoneIdx.get() : 0;
								}
							}
						}
						else
						{
							todo_important(TXT("implement_ other skinning index type"));
						}

						if (bonesRequired > 1) // no need to provide weight for single bone
						{
							float* out = (float*)(currentVertexData + _context.get_vertex_format().get_skinning_weights_offset());
							for (int idx = 0; idx < bonesRequired; ++idx, ++out)
							{
								*out = idx == 0 ? 1.0f : 0.0f;
							}
						}
					}
				}

				_context.store_part(part, _instance, materialIndex);

				apply_standard_parameters(part->access_vertex_data(), part->get_number_of_vertices(), _context, _instance);
			}
		}
	}

	return result;
}
