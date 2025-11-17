#include "meshGenMeshProcOp_numberCustomData.h"

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

REGISTER_FOR_FAST_CAST(NumberCustomData);

bool NumberCustomData::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= name.load_from_xml(_node, TXT("name"));

	u.load_from_xml(_node, TXT("u"));
	uRadius.load_from_xml(_node, TXT("uRadius"));

	result &= verticesInRange.load_from_xml(_node, TXT("verticesInRange"));

	return result;
}

bool NumberCustomData::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	Name cdName = this->name.get_with_default(_context, Name::invalid());

	Optional<float> u;
	float uRadius = 0.0f;
	Optional<Range3> verticesInRange;
	if (this->u.is_set()) u = this->u.get(_context);
	if (this->uRadius.is_set()) uRadius = this->uRadius.get(_context);
	if (this->verticesInRange.is_set()) verticesInRange = this->verticesInRange.get(_context);

	struct Patch
	{
		struct Triangle
		{
			int vertexIndices[3];
			Vector3 locations[3];
		};
		Array<Triangle> triangles;
	};
	List<Patch> patches;

	for_every(pIdx, _actOnMeshPartIndices)
	{
		::Meshes::Mesh3DPart* part = _context.get_parts()[*pIdx].get();
		auto& vertexFormat = part->get_vertex_format();
		if (auto* cd = vertexFormat.get_custom_data(cdName))
		{
			int cdOffset = vertexFormat.get_custom_data_offset(cdName);
			if (cdOffset != NONE)
			{
				int stride = vertexFormat.get_stride();
				int locOffset = vertexFormat.get_location_offset();
				part->for_every_triangle_indices_and_u(
					[&patches, part, stride, locOffset, u, uRadius]
					(int _aIdx, int _bIdx, int _cIdx, float _u)
					{
						if (u.is_set() && abs(u.get() - _u) > uRadius)
						{
							// skip
							return;
						}
						Vector3 aLoc, bLoc, cLoc;
						{
							char const* currentVertexData = part->get_vertex_data().get_data() + _aIdx * stride;
							aLoc = *(Vector3*)(currentVertexData + locOffset);
						}
						{
							char const* currentVertexData = part->get_vertex_data().get_data() + _bIdx * stride;
							bLoc = *(Vector3*)(currentVertexData + locOffset);
						}
						{
							char const* currentVertexData = part->get_vertex_data().get_data() + _cIdx * stride;
							cLoc = *(Vector3*)(currentVertexData + locOffset);
						}
						Patch* found = nullptr;
						for_every(patch, patches)
						{
							for_every(tri, patch->triangles)
							{
								// check in opposite directions
								if ((tri->locations[0] == aLoc && tri->locations[1] == cLoc) ||
									(tri->locations[0] == bLoc && tri->locations[1] == aLoc) ||
									(tri->locations[0] == cLoc && tri->locations[1] == bLoc) ||
									(tri->locations[1] == aLoc && tri->locations[2] == cLoc) ||
									(tri->locations[1] == bLoc && tri->locations[2] == aLoc) ||
									(tri->locations[1] == cLoc && tri->locations[2] == bLoc) ||
									(tri->locations[2] == aLoc && tri->locations[0] == cLoc) ||
									(tri->locations[2] == bLoc && tri->locations[0] == aLoc) ||
									(tri->locations[2] == cLoc && tri->locations[0] == bLoc))
								{
									found = patch;
									break;
								}
							}
						}
						if (!found)
						{
							patches.push_back(Patch());
							found = &patches.get_last();
						}
						{
							found->triangles.grow_size(1);
							auto& t = found->triangles.get_last();
							t.locations[0] = aLoc;
							t.locations[1] = bLoc;
							t.locations[2] = cLoc;
							t.vertexIndices[0] = _aIdx;
							t.vertexIndices[1] = _bIdx;
							t.vertexIndices[2] = _cIdx;
						}
					});

				// make sure they are grouped together
				while (true)
				{
					bool changedSomething = false;
					for_every(patch, patches)
					{
						for_every(oPatch, patches)
						{
							if (oPatch == patch)
							{
								// only up to this one
								break;
							}

							for (int i = 0; i < oPatch->triangles.get_size(); ++ i)
							{
								auto& t = oPatch->triangles[i];
								Vector3 aLoc = t.locations[0];
								Vector3 bLoc = t.locations[1];
								Vector3 cLoc = t.locations[2];

								Patch* found = nullptr;
								for_every(tri, patch->triangles)
								{
									// check in opposite directions
									if ((tri->locations[0] == aLoc && tri->locations[1] == cLoc) ||
										(tri->locations[0] == bLoc && tri->locations[1] == aLoc) ||
										(tri->locations[0] == cLoc && tri->locations[1] == bLoc) ||
										(tri->locations[1] == aLoc && tri->locations[2] == cLoc) ||
										(tri->locations[1] == bLoc && tri->locations[2] == aLoc) ||
										(tri->locations[1] == cLoc && tri->locations[2] == bLoc) ||
										(tri->locations[2] == aLoc && tri->locations[0] == cLoc) ||
										(tri->locations[2] == bLoc && tri->locations[0] == aLoc) ||
										(tri->locations[2] == cLoc && tri->locations[0] == bLoc))
									{
										found = patch;
										break;
									}
								}

								if (found)
								{
									found->triangles.push_back(t);
									oPatch->triangles.remove_fast_at(i);
									--i;
									changedSomething = true;
								}
							}
						}
					}

					if (!changedSomething)
					{
						break;
					}
				}

				for_every(patch, patches)
				{
					System::VertexFormatUtils::FillCustomDataParams params;
					params.with_int(for_everys_index(patch));
					for_every(t, patch->triangles)
					{
						for_count(int, i, 3)
						{
							char * currentVertexData = part->access_vertex_data().get_data() + t->vertexIndices[i] * stride;
							System::VertexFormatUtils::fill_custom_data(vertexFormat, currentVertexData, stride, *cd, params);
						}
					}
				}
			}
		}
	}

	return result;
}
