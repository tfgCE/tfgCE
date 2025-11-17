#include "meshGenModifiers.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenUtils.h"
#include "..\meshGenElementModifier.h"
#include "..\meshGenValueDef.h"
#include "..\meshGenValueDefImpl.inl"

#include "..\..\world\pointOfInterest.h"

#include "..\..\..\core\collision\model.h"
#include "..\..\..\core\containers\arrayStatic.h"
#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\mesh\socketDefinitionSet.h"
#include "..\..\..\core\system\video\vertexFormatUtils.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace Modifiers;

/**
 *	Skins Range segment
 */
class SkinRangeData
: public ElementModifierData
{
	FAST_CAST_DECLARE(SkinRangeData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

public: // we won't be exposed anywhere else, so we may be public here!
	ValueDef<Vector3> point;
	ValueDef<Range> range;
	ValueDef<Name> bone;
	ValueDef<Name> modifyBone;
};

//

struct BoneSkin
{
	int index;
	float weight;

	BoneSkin() {}
	BoneSkin(int _index, float _weight) : index(_index), weight(_weight) {}
};

bool Modifiers::skin_range(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
{
	bool result = true;

	Checkpoint checkpoint(_context);

	if (_subject)
	{
		result &= _context.process(_subject, &_instigatorInstance);
	}
	else
	{
		checkpoint = _context.get_checkpoint(1); // of parent
	}

	Checkpoint now(_context);
	
	if (checkpoint != now)
	{
		if (SkinRangeData const * data = fast_cast<SkinRangeData>(_data))
		{
			if (data->point.is_set() && data->range.is_set())
			{
				Name modifyBone = data->modifyBone.get_with_default(_context, Name::invalid());
				Name bone = data->bone.get_with_default(_context, modifyBone);

				if (bone.is_valid())
				{
					int boneIdx = _context.find_bone_index(bone);
					int modifyBoneIdx = modifyBone.is_valid()? _context.find_bone_index(modifyBone) : NONE;
					if (boneIdx != NONE)
					{
						Vector3 point = data->point.get(_context);
						Range range = data->range.get(_context);
						if (range.min == range.max)
						{
							range.min = 0.0f;
						}
						
						if (checkpoint.partsSoFarCount != now.partsSoFarCount)
						{
							RefCountObjectPtr<::Meshes::Mesh3DPart> const* pPart = &_context.get_parts()[checkpoint.partsSoFarCount];
							for (int i = checkpoint.partsSoFarCount; i < now.partsSoFarCount; ++i, ++pPart)
							{
								::Meshes::Mesh3DPart* part = pPart->get();
								auto & vertexData = part->access_vertex_data();
								auto & vertexFormat = part->get_vertex_format();
								if (vertexFormat.has_skinning_data())
								{
									auto numberOfVertices = part->get_number_of_vertices();
									int stride = vertexFormat.get_stride();
									int locationOffset = vertexFormat.get_location_offset();
									int skinningOffset = vertexFormat.has_skinning_data() ? (vertexFormat.get_skinning_element_count() > 1 ? vertexFormat.get_skinning_indices_offset() : vertexFormat.get_skinning_index_offset()) : NONE;
									int skinningElementCount = vertexFormat.has_skinning_data() ? vertexFormat.get_skinning_element_count() : 0;
									if (skinningOffset != NONE)
									{
										int8* currentVertexData = vertexData.get_data();
										for (int vrtIdx = 0; vrtIdx < numberOfVertices; ++vrtIdx, currentVertexData += stride)
										{
											if (vertexFormat.get_location() == ::System::VertexFormatLocation::XYZ)
											{
												Vector3* location = (Vector3*)(currentVertexData + locationOffset);
												float distance = (*location - point).length();
												float t = clamp(1.0f - range.get_pt_from_value(distance), 0.0f, 1.0f);

												if (t > 0.0f)
												{
													an_assert(skinningElementCount + 2 <= 10);
													ARRAY_STATIC(BoneSkin, skinning, 10);
													for_count(int, s, skinningElementCount)
													{
														uint index;
														float weight;
														System::VertexFormatUtils::get_skinning_info(vertexFormat, currentVertexData, s, OUT_ index, OUT_ weight);
														skinning.push_back(BoneSkin(index, weight));
													}

													if (modifyBoneIdx != NONE)
													{
														bool done = false;
														float modifyBoneWeight = 0.0f;
														ARRAY_STATIC(int, modifyBoneIndices, 16);
														for_every(s, skinning)
														{
															if (s->index == modifyBoneIdx)
															{
																modifyBoneIndices.push_back(for_everys_index(s));
																modifyBoneWeight += s->weight;
																if (t >= 1.0f)
																{
																	s->index = boneIdx;
																	done = true;
																}
															}
														}
														if (!done && modifyBoneIndices.get_size() > 0)
														{
															// just add new one and lower weights for bones we modify
															skinning.push_back(BoneSkin(boneIdx, modifyBoneWeight * t));
															for_every(mbi, modifyBoneIndices)
															{
																skinning[*mbi].weight *= 1.0f - t;
															}
															if (skinning.get_size() > skinningElementCount)
															{
																// we have to remove one
																int lowestIdx = NONE;
																float lowest = 0.0f;
																float sum = 0.0f;
																for_every(s, skinning)
																{
																	sum += s->weight;
																	if (lowestIdx == NONE || lowest < s->weight)
																	{
																		lowestIdx = for_everys_index(s);
																		lowest = s->weight;
																	}
																}
																skinning.remove_at(lowestIdx);
																sum -= lowest;
																float invSum = sum != 0.0f ? 1.0f / sum : 0.0f;
																for_every(s, skinning)
																{
																	s->weight *= invSum;
																}
															}
															done = true;
														}
														while (skinning.get_size() < skinningElementCount)
														{
															skinning.push_back(BoneSkin(0, 0.0f));
														}
														{
															float sumWeight = 0.0f;
															for_every(s, skinning)
															{
																sumWeight += s->weight;
															}
															an_assert(abs(sumWeight - 1.0f) < 0.01f);
														}
														// write back
														for_every(s, skinning)
														{
															System::VertexFormatUtils::set_skinning_info(vertexFormat, currentVertexData, for_everys_index(s), OUT_ s->index, OUT_ s->weight);
														}
													}
													else
													{
														uint indexToSet = boneIdx;
														float weightToSet = t;
														for_count(int, s, skinningElementCount)
														{
															uint index;
															float weight;
															System::VertexFormatUtils::get_skinning_info(vertexFormat, currentVertexData, s, OUT_ index, OUT_ weight);
															if ((weight == 0.0f || t == 1.0f) && indexToSet != NONE)
															{
																System::VertexFormatUtils::set_skinning_info(vertexFormat, currentVertexData, s, indexToSet, weightToSet);
																indexToSet = NONE;
															}
															else
															{
																System::VertexFormatUtils::set_skinning_info(vertexFormat, currentVertexData, s, index, weight * (1.0f - t));
															}
														}
														if (indexToSet != NONE && t < 1.0f)
														{
															// choose one with lowest weight, replace it and scale others
															float otherWeight = 0.0;
															float minWeight = 0.0f;
															int minWeightIdx = NONE;
															for_count(int, s, skinningElementCount)
															{
																uint index;
																float weight;
																System::VertexFormatUtils::get_skinning_info(vertexFormat, currentVertexData, s, OUT_ index, OUT_ weight);
																if (minWeightIdx == NONE || minWeight > weight)
																{
																	minWeightIdx = index;
																	minWeight = weight;
																}
																otherWeight += weight;
															}

															otherWeight -= minWeight;
															float weightMul = 0.0f;
															an_assert(otherWeight != 0.0f)
															{
																float newOtherWeight = (1.0f - weightToSet);
																weightMul = newOtherWeight / otherWeight;
															}

															for_count(int, s, skinningElementCount)
															{
																if (s != minWeightIdx)
																{
																	uint index;
																	float weight;
																	System::VertexFormatUtils::get_skinning_info(vertexFormat, currentVertexData, s, OUT_ index, OUT_ weight);
																	System::VertexFormatUtils::set_skinning_info(vertexFormat, currentVertexData, s, index, weight * weightMul);
																}
															}
															System::VertexFormatUtils::set_skinning_info(vertexFormat, currentVertexData, minWeightIdx, indexToSet, weightToSet);
														}
													}
												}
												{
													float sumWeight = 0.0f;
													for_count(int, s, skinningElementCount)
													{
														uint index;
														float weight;
														System::VertexFormatUtils::get_skinning_info(vertexFormat, currentVertexData, s, OUT_ index, OUT_ weight);
														sumWeight += weight;
													}
													an_assert(abs(sumWeight - 1.0f) < 0.01f);
												}
											}
											else
											{
												todo_important(TXT("implement_"));
											}
										}
									}
								}
							}
						}
						if (checkpoint.movementCollisionPartsSoFarCount != now.movementCollisionPartsSoFarCount ||
							checkpoint.preciseCollisionPartsSoFarCount != now.preciseCollisionPartsSoFarCount)
						{
							error_generating_mesh(_instigatorInstance, TXT("do not add collision here (or implement_ it!), add collision outside of modifier block"));
							result = false;
						}
						if (checkpoint.socketsGenerationIdSoFar != now.socketsGenerationIdSoFar)
						{
							for_every(socket, _context.access_sockets().access_sockets())
							{
								if (socket->get_generation_id() > checkpoint.socketsGenerationIdSoFar)
								{
									if (!modifyBone.is_valid() || socket->get_bone_name() == modifyBone)
									{
										if (socket->get_placement_MS().is_set())
										{
											//float t = clamp(1.0f - range.get_pt_from_value((socket->get_placement_MS().get().get_translation() - point).length()), 0.0f, 1.0f);
											socket->set_bone_name(bone);
										}
									}
								}
							}
						}
						// for others it does not make sense
					}
					else
					{
						error_generating_mesh(_instigatorInstance, TXT("bone and range have to be set"));
						result = false;
					}
				}
				else
				{
					error_generating_mesh(_instigatorInstance, TXT("bone to point at bones directly or with modifyBone"));
					result = false;
				}
			}
		}
	}
	
	return result;
}

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_skin_range_data()
{
	return new SkinRangeData();
}

//

REGISTER_FOR_FAST_CAST(SkinRangeData);

bool SkinRangeData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	point.load_from_xml_child_node(_node, TXT("point"));
	range.load_from_xml_child_node(_node, TXT("range"));
	bone.load_from_xml_child_node(_node, TXT("bone"));
	modifyBone.load_from_xml_child_node(_node, TXT("modifyBone"));

	if (!point.is_set() ||
		!range.is_set())
	{
		error_loading_xml(_node, TXT("not all parameters for skin range were defined"));
		result = false;
	}

	return result;
}
