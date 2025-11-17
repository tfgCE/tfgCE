#include "combinedMesh3dInstanceSet.h"
#include "mesh3d.h"
#include "..\system\video\indexFormatUtils.h"
#include "..\system\video\vertexFormatUtils.h"
#include "..\system\video\materialInstance.h"
#include "..\containers\arrayStack.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
//#define LOG_COMBINED
#endif

//

using namespace Meshes;

//

CombinedMesh3DInstanceSet::SourceMesh3DInstanceRedirector::~SourceMesh3DInstanceRedirector()
{
	partRedirectors.clear();
}

//

CombinedMesh3DInstanceSet::CombinedMesh::~CombinedMesh()
{
	mesh->close();
	delete mesh;
}

//

CombinedMesh3DInstanceSet::CombinedMesh3DInstanceSet()
{
}

CombinedMesh3DInstanceSet::~CombinedMesh3DInstanceSet()
{
	clear();
}

void CombinedMesh3DInstanceSet::clear()
{
	instanceSet.clear();

	for_every_ptr(instanceRedirector, instanceRedirectors)
	{
		delete instanceRedirector;
	}
	instanceRedirectors.clear();

	for_every_ptr(combinedMesh, combinedMeshes)
	{
		delete combinedMesh;
	}
	combinedMeshes.clear();
}

void CombinedMesh3DInstanceSet::set_instance_placement(int _meshIndex, Transform const & _placement)
{
	// each part of original mesh might be placed in different mesh within instance set, and we want to modify different bone within that mesh that
	SourceMesh3DInstanceRedirector const * instanceRedirector = instanceRedirectors[_meshIndex];
	if (instanceRedirector->wasCombined)
	{
		for_every(partRedirector, instanceRedirector->partRedirectors)
		{
			instanceSet.access_instance(partRedirector->meshIndex)->access_render_bone_matrices()[partRedirector->boneIndex] = _placement.to_matrix();
		}
	}
	else
	{
		instanceSet.set_placement(instanceRedirector->meshIndex, _placement);
	}
}

void CombinedMesh3DInstanceSet::combine(Mesh3DInstanceSet const & _sourceSet)
{
	clear();

	int sourceInstanceIndex = 0;
	while (Mesh3DInstance const * sourceInstance = _sourceSet.get_instance(sourceInstanceIndex))
	{
		SourceMesh3DInstanceRedirector* instanceRedirector = new SourceMesh3DInstanceRedirector();
		instanceRedirectors.push_back(instanceRedirector);
		instanceRedirector->wasCombined = false;

		// handle only meshes that are Mesh3D and can be combined
		if (sourceInstance->can_be_combined())
		{
			if (Mesh3D const * sourceMesh3d = fast_cast<Mesh3D>(sourceInstance->get_mesh()))
			{
				if (sourceMesh3d->can_be_combined())
				{
					// combined!
					instanceRedirector->wasCombined = true;
					// move all parts to existing meshes or create new ones
					int sourcePartIndex = 0;
					while (Mesh3DPart const * sourcePart = sourceMesh3d->get_part(sourcePartIndex))
					{
						if (sourcePart->is_empty())
						{
							++sourcePartIndex;
							continue;
						}
						SourceMesh3DInstancePartRedirector partRedirector; // ready redirector
						// find existing combined mesh that matches source vertex format and source material instance (setup)
						CombinedMesh* addToCombinedMesh = nullptr;
						{
							int combinedMeshIndex = 0;
							for_every_ptr(combinedMesh, combinedMeshes)
							{
								if (::System::VertexFormatUtils::do_match(combinedMesh->firstSourceMeshPart->vertexFormat, sourcePart->vertexFormat) &&
									::System::IndexFormatUtils::do_match(combinedMesh->firstSourceMeshPart->indexFormat, sourcePart->indexFormat) &&
									::System::MaterialInstance::do_match(*combinedMesh->sourceMaterialInstance, *sourceInstance->get_material_instance(sourcePartIndex)) &&
									::Mesh3D::do_immovable_match(sourceInstance->can_be_combined_as_immovable(), combinedMesh->immovable))
								{
									addToCombinedMesh = combinedMesh;
									partRedirector.meshIndex = combinedMeshIndex;
									partRedirector.partIndex = 0;
									break;
								}
								++combinedMeshIndex;
							}
						}

						// if not found, create new combined mesh basing on data from that part
						if (addToCombinedMesh == nullptr)
						{
							// add combined mesh
							CombinedMesh* combinedMesh = new CombinedMesh();
							combinedMeshes.push_back(combinedMesh);
							combinedMesh->mesh = new Mesh3D();
							combinedMesh->firstSourceMeshPart = sourcePart;
							combinedMesh->sourceMaterialInstance = sourceInstance->get_material_instance(sourcePartIndex);
							combinedMesh->immovable = sourceMesh3d->can_be_combined_as_immovable();
							// create part
							// this is really important part
							// we setup usage, add single part, setup vertex format, everything here
							combinedMesh->mesh->set_usage(combinedMesh->immovable? Usage::Static : Usage::SkinnedToSingleBone);
							RefCountObjectPtr<Mesh3DPart> part(new Mesh3DPart(false, false)); // can't be used to be combined nor updated dynamicaly
							combinedMesh->mesh->add_part(part.get());
							// copy setup
							part->primitiveType = sourcePart->primitiveType;
							part->vertexFormat = sourcePart->vertexFormat;
							part->indexFormat = sourcePart->indexFormat;
							if (!combinedMesh->immovable)
							{
								// setup skinned to single bone
								part->vertexFormat.with_skinning_to_single_bone_data();
							}
							part->vertexFormat.calculate_stride_and_offsets();
							part->indexFormat.calculate_stride();
							// this data will be available eventually
							part->dataAvailableToBuildBufferObjects = true; 
							// we will be adding data here
							addToCombinedMesh = combinedMesh;
							// and add instance at default placement, we will work with bones
							Mesh3DInstance* meshInstance = instanceSet.add(combinedMesh->mesh);
							// and setup material - one part, one material
							meshInstance->requires_at_least_materials(1);
							meshInstance->access_material_instance(0)->hard_copy_from(*combinedMesh->sourceMaterialInstance, combinedMesh->immovable ? ::System::MaterialShaderUsage::Static : ::System::MaterialShaderUsage::SkinnedToSingleBone);
							// setup redirector to point to last mesh
							partRedirector.meshIndex = combinedMeshes.get_size() - 1;
							partRedirector.partIndex = 0;
						}

						// add to part
						Mesh3DPart* addToPart = addToCombinedMesh->mesh->access_part(partRedirector.partIndex);
						Mesh3DInstance* asInstance = instanceSet.access_instance(partRedirector.meshIndex);

						// add bone to instance and set it with source instance placement (redirector will be fully setup after this action)
						Array<Matrix44> & bones = asInstance->access_render_bone_matrices();
						partRedirector.boneIndex = bones.get_size();
						Transform relativePlacement = asInstance->get_placement().to_local(sourceInstance->get_placement());
						bones.push_back(relativePlacement.to_matrix());

						// add from sourcePart to addToPart
						int newAddedVertexDataSize;
						void* newVerticesGoHere;
						addToPart->add_from_part(sourcePart, nullptr, &newAddedVertexDataSize, &newVerticesGoHere);

						if (addToCombinedMesh->immovable)
						{
							// apply relative placement to locations and normals
							::System::VertexFormatUtils::apply_transform_to_data(addToPart->vertexFormat, newVerticesGoHere, newAddedVertexDataSize, relativePlacement);
						}
						else
						{
							// define single bone data
							int singleBoneIndexDataSize = ::System::DataFormatValueType::get_size(addToPart->vertexFormat.get_skinning_index_type());
							ARRAY_PREALLOC_SIZE(int8, singleBoneIndexData, singleBoneIndexDataSize);
							::System::DataFormatValueType::fill_with_uint(addToPart->vertexFormat.get_skinning_index_type(), partRedirector.boneIndex, singleBoneIndexData.get_data());

							// add single bone data to copied vertices
							::System::VertexFormatUtils::fill_data_at_offset_with(addToPart->vertexFormat, newVerticesGoHere, newAddedVertexDataSize, addToPart->vertexFormat.get_skinning_index_offset(), singleBoneIndexData.get_data(), singleBoneIndexDataSize);
						}

						// add redirector
						instanceRedirector->partRedirectors.push_back(partRedirector);
						++sourcePartIndex;
					}
				}
			}
		}

		if (!instanceRedirector->wasCombined)
		{
			// not combined - just point at existing mesh - in worst case we will double source mesh3dInstanceSet
			instanceRedirector->meshIndex = instanceSet.get_instances_num();
			if (auto* mi3d = instanceSet.add(sourceInstance->get_mesh(), sourceInstance->get_placement()))
			{
				mi3d->hard_copy_from(*sourceInstance);
			}
		}

		++sourceInstanceIndex;
	}

#ifdef LOG_COMBINED
	output("combined %i into %i", _sourceSet.get_instances_num(), instanceSet.get_instances_num());
#endif
}
