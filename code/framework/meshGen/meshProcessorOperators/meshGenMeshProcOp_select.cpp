#include "meshGenMeshProcOp_select.h"

#include "..\meshGenGenerationContext.h"
#include "..\modifiers\meshGenModifierUtils.h"

#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
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

REGISTER_FOR_FAST_CAST(Select);

bool Select::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	u.load_from_xml(_node, TXT("u"));
	uRadius.load_from_xml(_node, TXT("uRadius"));
	range.load_from_xml(_node, TXT("range"));

	for_every(node, _node->children_named(TXT("bone")))
	{
		BoneInfo bone;
		if (bone.bone.load_from_xml(node, TXT("name")))
		{
			bone.andAllBelow = node->get_bool_attribute_or_from_child_presence(TXT("andAllBelow"), bone.andAllBelow);
			bones.push_back(bone);
		}
		else
		{
			result = false;
		}
	}
	result &= onNotSelected.load_from_xml_child_node(_node, _lc, TXT("onNotSelected"));
	result &= onSelected.load_from_xml_child_node(_node, _lc, TXT("onSelected"));

	return result;
}

bool Select::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= onNotSelected.prepare_for_game(_library, _pfgContext);
	result &= onSelected.prepare_for_game(_library, _pfgContext);

	return result;
}

bool Select::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	Array<int> notSelectedMPIndices;
	Array<int> selectedMPIndices;

	Optional<float> u;
	float uRadius = 0.0f;
	Optional<Range3> range;
	Array<int> boneIndices;
	if (this->u.is_set())
	{
		u = this->u.get(_context);
	}
	if (this->uRadius.is_set())
	{
		uRadius = this->uRadius.get(_context);
	}
	if (this->range.is_set())
	{
		range = this->range.get(_context);
	}
	if (!this->bones.is_empty())
	{
		for_every(bone, this->bones)
		{
			Name boneName = bone->bone.get(_context);
			int boneIdx = _context.find_bone_index(boneName);
			if (boneIdx != NONE)
			{
				boneIndices.push_back(boneIdx);
				if (bone->andAllBelow)
				{
					_context.get_children_bone(boneIdx, OUT_ boneIndices, true);
				}
			}
			else
			{
				error_generating_mesh(_instance, TXT("invalid bone"));
			}
		}
	}

	for_every(pIdx, _actOnMeshPartIndices)
	{
		::Meshes::Mesh3DPart* processPart = _context.get_parts()[*pIdx].get();

		if (!u.is_set() && ! range.is_set() && bones.is_empty())
		{
			selectedMPIndices.push_back(*pIdx);
		}
		else
		{
			RefCountObjectPtr<::Meshes::Mesh3DPart> newPart;
			for_count(int, forSelect, 2)
			{
				an_assert(!forSelect || newPart.get());
				::Meshes::Mesh3DPart* part = forSelect ? newPart.get() : processPart;
				if (part->get_primitive_type() == Meshes::Primitive::Triangle)
				{
					auto& vertexData = part->access_vertex_data();
					auto& vertexFormat = part->get_vertex_format();
					int vStride = vertexFormat.get_stride();
					{
						auto& indexData = part->access_element_data();
						auto& indexFormat = part->get_index_format();
						bool indexed = indexFormat.get_element_size() != ::System::IndexElementSize::None;
						Array<int8> newData;
						int addedTriangles = 0;
						{
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
								bool inRange = false;
								int boneCount = 0;

								for_count(int, iIdx, 3)
								{
									int vIdx = i[iIdx];

									if (u.is_set() && vertexFormat.get_texture_uv() != System::VertexFormatTextureUV::None)
									{
										float vu = ::System::VertexFormatUtils::restore_as_float(vertexData.begin() + vIdx * vStride + vertexFormat.get_texture_uv_offset(), vertexFormat.get_texture_uv_type());
										if (abs(vu - u.get()) <= uRadius)
										{
											++uCount;
										}
									}
									if (!boneIndices.is_empty() && vertexFormat.has_skinning_data())
									{
										int skinningIndex = 0;
										while (true)
										{
											uint boneIdx;
											float weight;
											if (::System::VertexFormatUtils::get_skinning_info(vertexFormat, vertexData.begin(), skinningIndex, OUT_ boneIdx, OUT_ weight))
											{
												if (weight > 0.0f)
												{
													if (boneIndices.does_contain((int)boneIdx))
													{
														++ boneCount;
														break; // continue with the next vertex
													}
												}
												++skinningIndex;
											}
											else
											{
												break;
											}
										}
									}
								}
								if (range.is_set())
								{
									Vector3 loc0 = ::System::VertexFormatUtils::restore_location(vertexData.begin(), i[0], vertexFormat);
									Vector3 loc1 = ::System::VertexFormatUtils::restore_location(vertexData.begin(), i[1], vertexFormat);
									Vector3 loc2 = ::System::VertexFormatUtils::restore_location(vertexData.begin(), i[2], vertexFormat);
									if (range.get().intersects_triangle(loc0, loc1, loc2))
									{
										inRange = true;
									}
								}

								bool allUMatch = (uCount == 3);
								bool inRangeMatch = inRange;
								bool boneMatch = boneCount > 0;

								bool add = allUMatch || inRangeMatch || boneMatch;
								if (!forSelect)
								{
									add = !add;
								}

								if (add)
								{
									++addedTriangles;
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
						}

						if (addedTriangles == 0)
						{
							an_assert(forSelect == 0, TXT("if we haven't add any triangles, we should be modifying existing part only"));
							// we haven't added triangles here, then the next part will be all the same but with wholly selected
							selectedMPIndices.push_back(*pIdx);
							break; // don't process it twice
						}
						else if (addedTriangles == part->get_number_of_tris())
						{
							if (forSelect == 0)
							{
								// no triangles will go to the next part
								notSelectedMPIndices.push_back(*pIdx);
								break;
							}
						}
						else
						{
							if (forSelect == 0)
							{
								// first create a copy, so we will be processing existing data, not updated
								newPart = processPart->create_copy();
							}

							if (indexed)
							{
								auto vertexData = part->get_vertex_data();
								part->update_data(vertexData.get_data(), newData.get_data(), part->get_primitive_type(), part->get_number_of_vertices(), (newData.get_size() / indexFormat.get_stride()), vertexFormat, indexFormat);
								part->prune_vertices();
								if (part->get_number_of_elements() == 0)
								{
									part->be_empty_part(true);
								}
							}
							else
							{
								part->update_data(newData.get_data(), part->get_primitive_type(), (newData.get_size() / vertexFormat.get_stride()) / 3, vertexFormat);
								part->prune_vertices();
								if (part->get_number_of_vertices() == 0)
								{
									part->be_empty_part(true);
								}
							}

							if (forSelect)
							{
								if (!part->is_empty())
								{
									an_assert(part == newPart.get());
									int newIdx = _context.store_part_just_as(newPart.get(), processPart);
									selectedMPIndices.push_back(newIdx);
									_newMeshPartIndices.push_back(newIdx);
								}
							}
							else
							{
								notSelectedMPIndices.push_back(*pIdx);
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
		}
	}

	result &= onNotSelected.process(_context, _instance, notSelectedMPIndices, _newMeshPartIndices);
	result &= onSelected.process(_context, _instance, selectedMPIndices, _newMeshPartIndices);

	return result;
}
