#include "meshGenModifiers.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenUtils.h"
#include "..\meshGenElementModifier.h"
#include "..\meshGenValueDef.h"
#include "..\meshGenValueDefImpl.inl"

#include "..\..\world\pointOfInterest.h"

#include "..\..\..\core\collision\model.h"
#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\mesh\socketDefinitionSet.h"
#include "..\..\..\core\system\video\vertexFormatUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;
using namespace Modifiers;

//

/**
 *	Skins along (bone) chain
 */
class SkinAlongChainData
: public ElementModifierData
{
	FAST_CAST_DECLARE(SkinAlongChainData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

public: // we won't be exposed anywhere else, so we may be public here!
	int againstCheckpointsUp = 0; // to get extra checkpoints (if we know how much there are)

	bool modifyMesh = true;
	bool modifySockets = true;
	bool modifyCollisions = true;

	bool nonLinearSkin = false;

	ValueDef<Name> startBone;
	ValueDef<Name> endBone;
	ValueDef<Name> modifyBone;

	ValueDef<float> modifyWithinBoundsTRange; // extends range
	ValueDef<float> modifyWithinBoundsT0Range; // at 0
	ValueDef<float> modifyWithinBoundsT1Range; // at 1
};

//

bool Modifiers::skin_along_chain(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
{
	bool result = true;

	Checkpoint checkpoint(_context);

	if (_subject)
	{
		result &= _context.process(_subject, &_instigatorInstance);
	}
	else
	{
		int againstCheckpointsUp = 0;
		if (SkinAlongChainData const* data = fast_cast<SkinAlongChainData>(_data))
		{
			againstCheckpointsUp = data->againstCheckpointsUp;
		}
		checkpoint = _context.get_checkpoint(1 + againstCheckpointsUp); // of parent
	}

	Checkpoint now(_context);
	
	if (checkpoint != now)
	{
		if (SkinAlongChainData const * data = fast_cast<SkinAlongChainData>(_data))
		{
			{
				Name modifyBone = data->modifyBone.get_with_default(_context, Name::invalid());
				Name startBone = data->startBone.get_with_default(_context, Name::invalid());
				Name endBone = data->endBone.get_with_default(_context, Name::invalid());
				float modifyWithinBoundsTRange = data->modifyWithinBoundsTRange.get_with_default(_context, 0.0f);
				float modifyWithinBoundsT0Range = data->modifyWithinBoundsT0Range.get_with_default(_context, 0.0f);
				float modifyWithinBoundsT1Range = data->modifyWithinBoundsT1Range.get_with_default(_context, 0.0f);

				if (startBone.is_valid() && endBone.is_valid())
				{
					int startBoneIdx = _context.find_bone_index(startBone);
					int endBoneIdx = _context.find_bone_index(endBone);
					int modifyBoneIdx = modifyBone.is_valid()? _context.find_bone_index(modifyBone) : NONE;

					int aBoneIdx = endBoneIdx;
					int bBoneIdx = NONE;
					while (aBoneIdx != NONE && aBoneIdx != startBoneIdx)
					{
						bBoneIdx = aBoneIdx;
						aBoneIdx = _context.get_bone_parent(aBoneIdx);
						if (aBoneIdx != NONE && bBoneIdx != NONE)
						{
							Name aBone = _context.get_bone_name(aBoneIdx);
							Name bBone = _context.get_bone_name(bBoneIdx);
							Vector3 aPoint = _context.get_bone_placement_MS(aBoneIdx).get_translation();
							Vector3 bPoint = _context.get_bone_placement_MS(bBoneIdx).get_translation();

							Segment a2b = Segment(aPoint, bPoint);
							if (data->modifyMesh && checkpoint.partsSoFarCount != now.partsSoFarCount)
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
													float t = a2b.get_t_not_clamped(*location);
													float minT = 0.0f;
													float maxT = 1.0f;
													if (aBoneIdx == startBoneIdx) minT -= modifyWithinBoundsTRange + modifyWithinBoundsT0Range;
													if (bBoneIdx == endBoneIdx) maxT += modifyWithinBoundsTRange + modifyWithinBoundsT1Range;
													if (t >= minT && t <= maxT)
													{
														t = clamp(t, 0.0f, 1.0f);
														if (data->nonLinearSkin)
														{
															t = cos_deg(180.0f * (1.0f - t)) * 0.5f + 0.5f;
														}

														if (modifyBoneIdx != NONE)
														{
															float weightAvailable = 0.0f;
															ARRAY_STATIC(int, sAvailable, 16);
															for_count(int, s, skinningElementCount)
															{
																uint index;
																float weight;
																System::VertexFormatUtils::get_skinning_info(vertexFormat, currentVertexData, s, OUT_ index, OUT_ weight);
																if (index == modifyBoneIdx || weight == 0.0f)
																{
																	sAvailable.push_back(s);
																	weightAvailable += weight;
																}
															}
															if (weightAvailable > 0.0f)
															{
																uint firstIndex = aBoneIdx;
																uint secondIndex = bBoneIdx;
																float firstWeight = (1.0f - t) * weightAvailable;
																float secondWeight = t * weightAvailable;
																if (t > 0.5f)
																{
																	swap(firstIndex, secondIndex);
																	swap(firstWeight, secondWeight);
																}
																int s = 0;
																if (s < sAvailable.get_size())
																{
																	System::VertexFormatUtils::set_skinning_info(vertexFormat, currentVertexData, sAvailable[s], firstIndex, firstWeight);
																	++s;
																}
																if (s < sAvailable.get_size())
																{
																	System::VertexFormatUtils::set_skinning_info(vertexFormat, currentVertexData, sAvailable[s], secondIndex, secondWeight);
																	++s;
																}
																while (s < sAvailable.get_size())
																{
																	// clear rest
																	System::VertexFormatUtils::set_skinning_info(vertexFormat, currentVertexData, sAvailable[s], 0, 0.0f);
																	++s;
																}
															}
														}
														else
														{
															uint firstIndex = aBoneIdx;
															uint secondIndex = bBoneIdx;
															float firstWeight = 1.0f - t;
															float secondWeight = t;
															if (t > 0.5f)
															{
																swap(firstIndex, secondIndex);
																swap(firstWeight, secondWeight);
															}
															int s = 0;
															if (s < skinningElementCount)
															{
																System::VertexFormatUtils::set_skinning_info(vertexFormat, currentVertexData, s, firstIndex, firstWeight);
																++s;
															}
															if (s < skinningElementCount)
															{
																System::VertexFormatUtils::set_skinning_info(vertexFormat, currentVertexData, s, secondIndex, secondWeight);
																++s;
															}
															while (s < skinningElementCount)
															{
																// clear rest
																System::VertexFormatUtils::set_skinning_info(vertexFormat, currentVertexData, s, 0, 0.0f);
																++s;
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
							if (data->modifyCollisions && checkpoint.movementCollisionPartsSoFarCount != now.movementCollisionPartsSoFarCount)
							{
								::Collision::Model* const* pPart = &_context.get_movement_collision_parts()[checkpoint.movementCollisionPartsSoFarCount];
								for (int i = checkpoint.movementCollisionPartsSoFarCount; i < now.movementCollisionPartsSoFarCount; ++i, ++pPart)
								{
									::Collision::Model* part = *pPart;
									part->reskin([modifyBone, aBone, bBone, a2b, aBoneIdx, bBoneIdx, startBoneIdx, endBoneIdx, modifyWithinBoundsTRange, modifyWithinBoundsT0Range, modifyWithinBoundsT1Range](Name const& _bone, Optional<Vector3> const& _loc)
									{
										if (!modifyBone.is_valid() || _bone == modifyBone)
										{
											if (_loc.is_set())
											{
												float t = a2b.get_t_not_clamped(_loc.get());
												float minT = 0.0f;
												float maxT = 1.0f;
												if (aBoneIdx == startBoneIdx) minT -= modifyWithinBoundsTRange + modifyWithinBoundsT0Range;
												if (bBoneIdx == endBoneIdx) maxT += modifyWithinBoundsTRange + modifyWithinBoundsT1Range;
												if (t >= minT && t <= maxT)
												{
													return t < 0.5f ? aBone : bBone;
												}
											}
										}
										return _bone;
									});
								}
							}
							if (data->modifyCollisions && checkpoint.preciseCollisionPartsSoFarCount != now.preciseCollisionPartsSoFarCount)
							{
								::Collision::Model* const* pPart = &_context.get_precise_collision_parts()[checkpoint.preciseCollisionPartsSoFarCount];
								for (int i = checkpoint.preciseCollisionPartsSoFarCount; i < now.preciseCollisionPartsSoFarCount; ++i, ++pPart)
								{
									::Collision::Model* part = *pPart;
									part->reskin([modifyBone, aBone, bBone, a2b, aBoneIdx, bBoneIdx, startBoneIdx, endBoneIdx, modifyWithinBoundsTRange, modifyWithinBoundsT0Range, modifyWithinBoundsT1Range](Name const& _bone, Optional<Vector3> const& _loc)
									{
										if (!modifyBone.is_valid() || _bone == modifyBone)
										{
											if (_loc.is_set())
											{
												float t = a2b.get_t_not_clamped(_loc.get());
												float minT = 0.0f;
												float maxT = 1.0f;
												if (aBoneIdx == startBoneIdx) minT -= modifyWithinBoundsTRange + modifyWithinBoundsT0Range;
												if (bBoneIdx == endBoneIdx) maxT += modifyWithinBoundsTRange + modifyWithinBoundsT1Range;
												if (t >= minT && t <= maxT)
												{
													return t < 0.5f ? aBone : bBone;
												}
											}
										}
										return _bone;
									});
								}
							}
							if (data->modifySockets && checkpoint.socketsGenerationIdSoFar != now.socketsGenerationIdSoFar)
							{
								for_every(socket, _context.access_sockets().access_sockets())
								{
									if (socket->get_generation_id() > checkpoint.socketsGenerationIdSoFar)
									{
										if (!modifyBone.is_valid() || socket->get_bone_name() == modifyBone)
										{
											if (socket->get_placement_MS().is_set())
											{
												float t = a2b.get_t_not_clamped(socket->get_placement_MS().get().get_translation());
												if (t >= 0.0f && t <= 1.0f)
												{
													socket->set_bone_name(t < 0.5f ? aBone : bBone);
												}
											}
										}
									}
								}
							}
							// for others it does not make sense
						}
					}
				}
			}
		}
	}
	
	return result;
}

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_skin_along_chain_data()
{
	return new SkinAlongChainData();
}

//

REGISTER_FOR_FAST_CAST(SkinAlongChainData);

bool SkinAlongChainData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	againstCheckpointsUp = _node->get_int_attribute_or_from_child(TXT("againstCheckpointsUp"), againstCheckpointsUp);

	modifyMesh = _node->get_bool_attribute_or_from_child_presence(TXT("modifyMesh"), modifyMesh);
	modifySockets = _node->get_bool_attribute_or_from_child_presence(TXT("modifySockets"), modifySockets);
	modifyCollisions = _node->get_bool_attribute_or_from_child_presence(TXT("modifyCollisions"), modifyCollisions);
	nonLinearSkin = _node->get_bool_attribute_or_from_child_presence(TXT("nonLinear"), nonLinearSkin);
	nonLinearSkin = _node->get_bool_attribute_or_from_child_presence(TXT("nonLinearSkin"), nonLinearSkin);
	
	startBone.load_from_xml_child_node(_node, TXT("startBone"));
	endBone.load_from_xml_child_node(_node, TXT("endBone"));
	modifyBone.load_from_xml_child_node(_node, TXT("modifyBone"));

	modifyWithinBoundsTRange.load_from_xml_child_node(_node, TXT("modifyWithinBoundsTRange"));
	modifyWithinBoundsT0Range.load_from_xml_child_node(_node, TXT("modifyWithinBoundsT0Range"));
	modifyWithinBoundsT1Range.load_from_xml_child_node(_node, TXT("modifyWithinBoundsT1Range"));

	if (!startBone.is_set() ||
		!endBone.is_set())
	{
		error_loading_xml(_node, TXT("not all parameters for skin along were defined"));
		result = false;
	}

	return result;
}
