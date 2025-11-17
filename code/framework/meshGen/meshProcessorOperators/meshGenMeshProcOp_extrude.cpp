#include "meshGenMeshProcOp_extrude.h"

#include "..\meshGenGenerationContext.h"

#include "..\modifiers\meshGenModifierUtils.h"

#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\types\vectorPacked.h"
#include "..\..\..\core\system\video\indexFormatUtils.h"
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

REGISTER_FOR_FAST_CAST(Extrude);

bool Extrude::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	moveBy.load_from_xml_child_node(_node, TXT("moveBy"));
	moveBy.load_from_xml_child_node(_node, TXT("by"));
	point.load_from_xml_child_node(_node, TXT("point"));
	pointScale.load_from_xml(_node, TXT("pointScale"));
	for_every(node, _node->children_named(TXT("point")))
	{
		pointScale.load_from_xml_child_node(node, TXT("scale"));
	}

	error_loading_xml_on_assert(moveBy.is_set() ^ point.is_set(), result, _node, TXT("moveBy or point has to be set"));
	error_loading_xml_on_assert(point.is_set() == pointScale.is_set(), result, _node, TXT("point has to be followed with pointScale"));

	edgeU.load_from_xml(_node, TXT("edgeU"));

	result &= onExtruded.load_from_xml_child_node(_node, _lc, TXT("onExtruded"));
	result &= onEdge.load_from_xml_child_node(_node, _lc, TXT("onEdge"));

	return result;
}

bool Extrude::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= onExtruded.prepare_for_game(_library, _pfgContext);
	result &= onEdge.prepare_for_game(_library, _pfgContext);

	return result;
}

bool Extrude::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	struct Edge
	{
		Vector3 a;
		Vector3 b;
		int aIdx;
		int bIdx;
		float u = 0.0f; // only the first is used
		int ab = 0;
		int ba = 0;
	};
	Array<Edge> edges;

	Optional<Vector3> moveBy; if (this->moveBy.is_set()) moveBy = this->moveBy.get(_context);
	Optional<Vector3> point; if (this->point.is_set()) point = this->point.get(_context);
	Optional<float> pointScale; if (this->pointScale.is_set()) pointScale = this->pointScale.get(_context);
	Optional<float> edgeU; if (this->edgeU.is_set()) edgeU = this->edgeU.get(_context);

	Array<int> newlyCreatedMPIndices;
	Array<int> extrudedMPIndices;

	for_every(pIdx, _actOnMeshPartIndices)
	{
		::Meshes::Mesh3DPart* part = _context.get_parts()[*pIdx].get();
		if (part->is_empty())
		{
			continue;
		}
		auto& vFormat = part->get_vertex_format();
		auto& iFormat = part->get_index_format();
		if (part->get_primitive_type() != ::Meshes::Primitive::Triangle)
		{
			error_generating_mesh(_instance, TXT("non-triangles not supported"));
			result = false;
		}
		else if (iFormat.get_element_size() == ::System::IndexElementSize::None)
		{
			error_generating_mesh(_instance, TXT("non-indexed not supported"));
			result = false;
		}
		else
		{
			int vStride = vFormat.get_stride();
			int locOffset = vFormat.get_location_offset();
			// for each triangle get each edge and register it in our edges look up
			// increase how many times it is referenced (ab+ba) and its "u" value
			edges.clear();
			part->for_every_triangle_indices_and_u([part, &edges, locOffset, vStride](int _aIdx, int _bIdx, int _cIdx, float _u)
				{
					Vector3 aLoc = cast_to_value<Vector3>(part->get_vertex_data().get_data() + vStride * _aIdx + locOffset);
					Vector3 bLoc = cast_to_value<Vector3>(part->get_vertex_data().get_data() + vStride * _bIdx + locOffset);
					Vector3 cLoc = cast_to_value<Vector3>(part->get_vertex_data().get_data() + vStride * _cIdx + locOffset);

					if (aLoc != bLoc && aLoc != cLoc && bLoc != cLoc)
					{
						Vector3 v[] = { aLoc, bLoc, cLoc };
						int i[] = { _aIdx, _bIdx, _cIdx };
						for_count(int, eIdx, 3)
						{
							Vector3 a = v[eIdx];
							Vector3 b = v[(eIdx + 1) % 3];
							int aIdx = i[eIdx];
							int bIdx = i[(eIdx + 1) % 3];

							bool found = false;
							for_every(e, edges)
							{
								if (e->a == a &&
									e->b == b)
								{
									++ e->ab;
									found = true;
									break;
								}
								if (e->b == a &&
									e->a == b)
								{
									++ e->ba;
									found = true;
									break;
								}
							}

							if (!found)
							{
								Edge e;
								e.a = a;
								e.b = b;
								e.aIdx = aIdx;
								e.bIdx = bIdx;
								e.ab = 1;
								e.u = _u;
								edges.push_back(e);
							}
						}
					}
				});

			// now we have full edges look up, we have their locations stored, we keep it

			// first, let's extrude things
			for_count(int, vIdx, part->get_number_of_vertices())
			{
				Vector3& loc = cast_to_value<Vector3>(part->access_vertex_data().get_data() + vStride * vIdx + locOffset);
				if (moveBy.is_set())
				{
					loc += moveBy.get();
				}
				if (point.is_set() && pointScale.is_set())
				{
					loc = point.get() + (loc - point.get()) * pointScale.get();
				}
			}
			extrudedMPIndices.push_back(*pIdx); // we will run more mesh processor operators on this one

			// now create a part for edges
			{
				Array<int8> vertexData;
				Array<int8> elementData;
				vertexData.make_space_for(part->get_vertex_data().get_size());
				elementData.make_space_for(part->get_element_data().get_size());
				int vCount = 0;
				int eCount = 0;

				// go through edges and with all having ab+ba = 1 create an edge (calculate the normal too)
				todo_future(TXT("all vertices are unique here, if extrude is used a lot, it is a good place to optimise"));

				int iStride = iFormat.get_stride();
				for_every(e, edges)
				{
					if (e->ab + e->ba == 1) // used exactly once
					{
						an_assert(e->ab == 1 && e->ba == 0, TXT("shouldn't be any other way"));
						/**
						 * 
						 *				a-----b ORG
						 *				|	  |
						 *				a-----b EXT
						 *				 \	 /	 extruded
						 *				  \ /	 triangle
						 *				   c
						 * 
						 */
						Vector3 aOrg = e->a;
						Vector3 bOrg = e->b;
						Vector3 aExt = cast_to_value<Vector3>(part->get_vertex_data().get_data() + vStride * e->aIdx + locOffset);
						Vector3 bExt = cast_to_value<Vector3>(part->get_vertex_data().get_data() + vStride * e->bIdx + locOffset);

						int v0 = vCount;
						/* aOrg, v0 + 0 */	vertexData.grow_size(vStride);	memory_copy(&vertexData[vCount * vStride], part->get_vertex_data().get_data() + vStride * e->aIdx, vStride); ++ vCount;
						/* bOrg, v0 + 1 */	vertexData.grow_size(vStride);	memory_copy(&vertexData[vCount * vStride], part->get_vertex_data().get_data() + vStride * e->bIdx, vStride); ++ vCount;
						/* aExt, v0 + 2 */	vertexData.grow_size(vStride);	memory_copy(&vertexData[vCount * vStride], part->get_vertex_data().get_data() + vStride * e->aIdx, vStride); ++ vCount;
						/* bExt, v0 + 3 */	vertexData.grow_size(vStride);	memory_copy(&vertexData[vCount * vStride], part->get_vertex_data().get_data() + vStride * e->bIdx, vStride); ++ vCount;
						
						// use ORG location (it is EXT/extruded now there)
						::System::VertexFormatUtils::store_location(vertexData.begin(), (v0 + 0), vFormat, aOrg);
						::System::VertexFormatUtils::store_location(vertexData.begin(), (v0 + 1), vFormat, bOrg);

						if (edgeU.is_set() && vFormat.get_texture_uv() != ::System::VertexFormatTextureUV::None)
						{
							for_count(int, v, 4)
							{
								::System::VertexFormatUtils::store(vertexData.begin() + (v0 + v) * vStride + vFormat.get_texture_uv_offset(), vFormat.get_texture_uv_type(), edgeU.get());
							}
						}

						if (vFormat.get_normal() == ::System::VertexFormatNormal::Yes)
						{
							Vector3 aOrgNormal = Vector3::cross(aExt - aOrg, bOrg - aOrg).normal();
							Vector3 bOrgNormal = Vector3::cross(aOrg - bOrg, bExt - bOrg).normal();
							Vector3 aExtNormal = Vector3::cross(bExt - aExt, aOrg - aExt).normal();
							Vector3 bExtNormal = Vector3::cross(bOrg - bExt, aExt - bExt).normal();
							::System::VertexFormatUtils::store_normal(vertexData.begin(), (v0 + 0), vFormat, aOrgNormal);
							::System::VertexFormatUtils::store_normal(vertexData.begin(), (v0 + 1), vFormat, bOrgNormal);
							::System::VertexFormatUtils::store_normal(vertexData.begin(), (v0 + 2), vFormat, aExtNormal);
							::System::VertexFormatUtils::store_normal(vertexData.begin(), (v0 + 3), vFormat, bExtNormal);
						}

						elementData.grow_size(2/*triangles*/ * 3 * iStride);
						// aOrg -> bOrg -> aExt
						System::IndexFormatUtils::set_value(iFormat, elementData.begin(), eCount, v0 + 0); ++eCount;
						System::IndexFormatUtils::set_value(iFormat, elementData.begin(), eCount, v0 + 1); ++eCount;
						System::IndexFormatUtils::set_value(iFormat, elementData.begin(), eCount, v0 + 2); ++eCount;
						// aExt -> bOrg -> bExt
						System::IndexFormatUtils::set_value(iFormat, elementData.begin(), eCount, v0 + 2); ++eCount;
						System::IndexFormatUtils::set_value(iFormat, elementData.begin(), eCount, v0 + 1); ++eCount;
						System::IndexFormatUtils::set_value(iFormat, elementData.begin(), eCount, v0 + 3); ++eCount;
					}
				}

				if (vCount > 0 && eCount > 0)
				{
					RefCountObjectPtr<::Meshes::Mesh3DPart> copyForClosure(part->create_copy());
					an_assert(copyForClosure->get_vertex_format().is_ok_to_be_used());
					int newIdx = _context.store_part_just_as(copyForClosure.get(), part);
					newlyCreatedMPIndices.push_back(newIdx);

					copyForClosure->load_data(vertexData.get_data(), elementData.get_data(), part->get_primitive_type(), vCount, eCount, vFormat, iFormat);
				}
			}
		}
	}

	_newMeshPartIndices.add_from(newlyCreatedMPIndices);

	result &= onExtruded.process(_context, _instance, extrudedMPIndices, _newMeshPartIndices);
	result &= onEdge.process(_context, _instance, newlyCreatedMPIndices, _newMeshPartIndices);
	
	return result;
}
