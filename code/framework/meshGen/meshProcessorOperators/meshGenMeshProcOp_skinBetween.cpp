#include "meshGenMeshProcOp_skinBetween.h"

#include "..\meshGenGenerationContext.h"

#include "..\..\..\core\debug\debugRenderer.h"

#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\types\vectorPacked.h"
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

REGISTER_FOR_FAST_CAST(SkinBetween);

bool SkinBetween::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	debugDraw = _node->get_bool_attribute_or_from_child_presence(TXT("debugDraw"), debugDraw);
	
	curve = _node->get_name_attribute_or_from_child(TXT("curve"), curve);

	return result;
}

bool SkinBetween::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	struct BoneSkin
	{
		int index;
		float weight;

		BoneSkin() {}
		BoneSkin(int _index, float _weight) : index(_index), weight(_weight) {}

		static int compare(void const* _a, void const* _b)
		{
			BoneSkin const & a = *plain_cast<BoneSkin>(_a);
			BoneSkin const & b = *plain_cast<BoneSkin>(_b);
			if (a.weight > b.weight) return A_BEFORE_B;
			if (a.weight < b.weight) return B_BEFORE_A;
			return A_AS_B;
		}
	};

	struct VertexToSkin
	{
		Vector3 loc;
		int index;
		bool skinningRead = false;
		ARRAY_STATIC(BoneSkin, skinning, 10); // if has skinning from some other part
	};
	for_every(pIdx, _actOnMeshPartIndices)
	{
		::Meshes::Mesh3DPart* part = _context.get_parts()[*pIdx].get();
		auto& vertexData = part->access_vertex_data();
		auto& vertexFormat = part->get_vertex_format();
		if (vertexFormat.has_skinning_data())
		{
			int vStride = vertexFormat.get_stride();
			int skinningOffset = vertexFormat.has_skinning_data() ? (vertexFormat.get_skinning_element_count() > 1 ? vertexFormat.get_skinning_indices_offset() : vertexFormat.get_skinning_index_offset()) : NONE;
			int skinningElementCount = vertexFormat.has_skinning_data() ? vertexFormat.get_skinning_element_count() : 0;
			if (skinningOffset != NONE)
			{
				Array<VertexToSkin> vertices;
				{
					int numberOfVertices = part->get_number_of_vertices();
					int8* currentVertexData = vertexData.get_data();
					vertices.set_size(numberOfVertices);
					for (int vrtIdx = 0; vrtIdx < numberOfVertices; ++vrtIdx, currentVertexData += vStride)
					{
						auto& v = vertices[vrtIdx];
						v.loc = System::VertexFormatUtils::restore_location(currentVertexData, vertexFormat);
						// read other mesh parts to find skinning
						for_every_ref(oPart, _context.get_parts())
						{
							if (_actOnMeshPartIndices.does_contain(for_everys_index(oPart)))
							{
								continue;
							}
							auto& vertexData = oPart->access_vertex_data();
							auto& vertexFormat = oPart->get_vertex_format();
							if (vertexFormat.has_skinning_data())
							{
								int vStride = vertexFormat.get_stride();
								int skinningOffset = vertexFormat.has_skinning_data() ? (vertexFormat.get_skinning_element_count() > 1 ? vertexFormat.get_skinning_indices_offset() : vertexFormat.get_skinning_index_offset()) : NONE;
								if (skinningOffset != NONE)
								{
									int numberOfVertices = oPart->get_number_of_vertices();
									int8* currentVertexData = vertexData.get_data();
									for (int vrtIdx = 0; vrtIdx < numberOfVertices; ++vrtIdx, currentVertexData += vStride)
									{
										Vector3 vLoc = System::VertexFormatUtils::restore_location(currentVertexData, vertexFormat);
										if (vLoc == v.loc)
										{
											v.skinning.clear();
											for_count(int, i, vertexFormat.get_skinning_element_count())
											{
												uint index;
												float weight;

												if (System::VertexFormatUtils::get_skinning_info(vertexFormat, currentVertexData, i, OUT_ index, OUT_ weight))
												{
													v.skinning.push_back(BoneSkin(index, weight));
												}
											}
											v.skinningRead = true;
											break;
										}
									}
								}
							}
							if (!v.skinning.is_empty())
							{
								break;
							}
						}
					}
				}

				// go through all vertices and get weighted skinning
				for_every(v, vertices)
				{
					if (!v->skinningRead)
					{
						for_every(ov, vertices)
						{
							if (ov == v || !ov->skinningRead) continue;
							float distance = (ov->loc - v->loc).length();
							float distanceWeight = 1.0f / distance;
							for_every(os, ov->skinning)
							{
								if (os->weight == 0.0f) continue;
								bool found = false;
								for_every(s, v->skinning)
								{
									if (s->index == os->index)
									{
										s->weight = max(s->weight, os->weight * distanceWeight);
										found = true;
										break;
									}
								}
								if (!found)
								{
									v->skinning.push_back(BoneSkin(os->index, os->weight * distanceWeight));
								}
							}
						}
					}
				}
				// normalise and write back
				{
					int8* currentVertexData = vertexData.get_data();

					for_every(v, vertices)
					{
						// normalise
						for_count(int, n, 2)
						{
							float weightTotal = 0.0f;
							for_every(s, v->skinning)
							{
								weightTotal += s->weight;
							}
							float weightMul = weightTotal > 0.0f ? 1.0f / weightTotal : 1.0f;
							for_every(s, v->skinning)
							{
								s->weight *= weightMul;
							}
							if (n == 0)
							{
								if (curve.is_valid())
								{
									for_every(s, v->skinning)
									{
										s->weight = BlendCurve::apply(curve, s->weight);
									}
								}
								else
								{
									break;
								}
							}
						}
						// fill empty
						while (v->skinning.get_size() < skinningElementCount)
						{
							v->skinning.push_back(BoneSkin(0, 0.0f));
						}
						sort(v->skinning);
						// write back
						for_every(s, v->skinning)
						{
							System::VertexFormatUtils::set_skinning_info(vertexFormat, currentVertexData, for_everys_index(s), OUT_ s->index, OUT_ s->weight);
						}

						currentVertexData += vStride;
					}
				}
#ifdef AN_DEVELOPMENT
				if (debugDraw)
				{
					if (auto* drf = _context.debugRendererFrame)
					{
						for_every(v, vertices)
						{
							Colour c = v->skinningRead ? Colour::red : Colour::green;
							drf->add_sphere(true, true, c, 0.1f, Sphere(v->loc, 0.005f));
							drf->add_arrow(true, c, v->loc + Vector3(0.0f, 0.0f, 0.001f), v->loc);
							for_every(s, v->skinning)
							{
								if (s->weight > 0.0f &&
									_context.get_generated_bones().is_index_valid(s->index))
								{
									Vector3 boneLoc = _context.get_generated_bones()[s->index].placement.get_translation();
									drf->add_arrow(true, c.with_alpha(s->weight * 0.5f), v->loc, v->loc + (boneLoc - v->loc).normal() * 0.005f);
								}
							}
						}
					}
				}
#endif
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
