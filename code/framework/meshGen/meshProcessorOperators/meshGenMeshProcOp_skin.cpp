#include "meshGenMeshProcOp_skin.h"

#include "..\meshGenGenerationContext.h"

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

REGISTER_FOR_FAST_CAST(Skin);

bool Skin::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= toBone.load_from_xml(_node, TXT("toBone"));
	result &= replaceBone.load_from_xml(_node, TXT("replaceBone"));
	result &= withBone.load_from_xml(_node, TXT("withBone"));

	return result;
}

bool Skin::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	int toBoneIdx = NONE;
	int replaceBoneIdx = NONE;
	int withBoneIdx = NONE;
	struct ReadBone
	{
		GenerationContext& context;
		ElementInstance& instance;
		ReadBone(GenerationContext& _context, ElementInstance& _instance): context(_context), instance(_instance) {}
		bool read(OUT_ int& _boneIdx, ValueDef<Name> const& _bone)
		{
			if (_bone.is_set())
			{
				Name boneName = _bone.get(context);
				_boneIdx = context.find_bone_index(boneName);
				if (_boneIdx == NONE)
				{
					error_generating_mesh(instance, TXT("could not find bone \"%S\""), boneName.to_char());
					return false;
				}
			}
			return true;
		}
	} readBone(_context, _instance);
	readBone.read(OUT_ toBoneIdx, toBone);
	readBone.read(OUT_ replaceBoneIdx, replaceBone);
	readBone.read(OUT_ withBoneIdx, withBone);

	if (replaceBoneIdx != NONE && withBoneIdx == NONE)
	{
		withBoneIdx = toBoneIdx;
	}

	if (toBoneIdx != NONE ||
		(replaceBoneIdx != NONE && withBoneIdx != NONE))
	{
		bool replacing = replaceBoneIdx != NONE && withBoneIdx != NONE;
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
					int numberOfVertices = part->get_number_of_vertices();
					int8* currentVertexData = vertexData.get_data();
					for (int vrtIdx = 0; vrtIdx < numberOfVertices; ++vrtIdx, currentVertexData += vStride)
					{
						struct BoneSkin
						{
							int index;
							float weight;

							BoneSkin() {}
							BoneSkin(int _index, float _weight) : index(_index), weight(_weight) {}
						};

						an_assert(skinningElementCount + 2 <= 10);
						ARRAY_STATIC(BoneSkin, skinning, 10);
						if (replacing)
						{
							for_count(int, i, vertexFormat.get_skinning_element_count())
							{
								uint index;
								float weight;

								if (System::VertexFormatUtils::get_skinning_info(vertexFormat, currentVertexData, i, OUT_ index, OUT_ weight))
								{
									if (index == replaceBoneIdx)
									{
										index = withBoneIdx;
									}
									bool found = false;
									for_every(s, skinning)
									{
										if (s->index == index)
										{
											s->weight += weight;
											found = true;
											break;
										}
									}
									if (!found)
									{
										skinning.push_back(BoneSkin(index, weight));
									}
								}
							}
						}
						else
						{
							skinning.push_back(BoneSkin(toBoneIdx, 1.0f));
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
				}
			}
			else
			{
				error_generating_mesh(_instance, TXT("other primitives not supported right now"));
				result = false;
			}
		}
	}

	return result;
}
