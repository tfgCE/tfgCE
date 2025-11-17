#include "meshGenModifiers.h"

#include "meshGenModifier_mirror.h"

#include "meshGenModifierUtils.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenUtils.h"
#include "..\meshGenElementModifier.h"
#include "..\meshGenValueDef.h"

#include "..\..\appearance\appearanceControllerData.h"

#include "..\..\world\pointOfInterest.h"

#include "..\..\..\core\collision\model.h"
#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\mesh\socketDefinitionSet.h"
#include "..\..\..\core\types\vectorPacked.h"
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

float cut_mirror_threshold() { return 0.001f; }

//

bool apply_mirror(GenerationContext & _context, Checkpoint const & checkpoint, Checkpoint const & now, MeshMirrorData const * _data)
{
	bool result = true;

	if (MeshMirrorData const * data = fast_cast<MeshMirrorData>(_data))
	{
		Vector3 mirrorDir = data->mirrorDir.get(_context).normal();
		Vector3 mirrorPoint = data->mirrorPoint.get(_context);
		Vector3 upDir = abs(mirrorDir.z) > 0.7f ? Vector3::xAxis : Vector3::zAxis;
		Vector3 rightDir = Vector3::cross(mirrorDir, upDir).normal();
		upDir = Vector3::cross(rightDir, mirrorDir).normal();
		Matrix44 sourceMat = matrix_from_axes_orthonormal_check(rightDir, mirrorDir, upDir, mirrorPoint);
		Matrix44 destMat = matrix_from_axes_orthonormal_check(rightDir, -mirrorDir, upDir, mirrorPoint);
		Matrix44 transformMat = destMat.to_world(sourceMat.inverted());
		ARRAY_PREALLOC_SIZE(Transform, notMirroredBonesMS, now.bonesSoFarCount);
		for_count(int, b, now.bonesSoFarCount)
		{
			notMirroredBonesMS.push_back(_context.get_bone_placement_MS(b));
		}

		if ((_data->flags & Modifiers::Utils::Flags::Bones) && checkpoint.bonesSoFarCount != now.bonesSoFarCount)
		{
			Bone* bone = &_context.access_generated_bones()[checkpoint.bonesSoFarCount];
			for (int i = checkpoint.bonesSoFarCount; i < now.bonesSoFarCount; ++i, ++bone)
			{
				apply_transform_to_bone(*bone, transformMat);
				bone->boneName = data->boneRenamer.apply(bone->boneName);
				bone->parentName = data->boneRenamer.apply(bone->parentName);
			}
		}
		if ((_data->flags & Modifiers::Utils::Flags::Mesh) && checkpoint.partsSoFarCount != now.partsSoFarCount)
		{
			// for meshes apply transform and reverse order of elements (triangles, quads)
			RefCountObjectPtr<::Meshes::Mesh3DPart> const* pPart = &_context.get_parts()[checkpoint.partsSoFarCount];
			ARRAY_PREALLOC_SIZE(uint, remapBones, _context.get_bones_count());
			for (int i = checkpoint.partsSoFarCount; i < now.partsSoFarCount; ++i, ++pPart)
			{
				::Meshes::Mesh3DPart* part = pPart->get();
				if (part->is_empty())
				{
					continue;
				}
				apply_transform_to_vertex_data(part->access_vertex_data(), part->get_number_of_vertices(), part->get_vertex_format(), transformMat);
				if (! data->boneRenamer.is_empty())
				{
					remapBones.clear(); // reusing
					for_count(int, b, _context.get_bones_count())
					{
						Name bone = _context.get_bone_name(b);
						bone = data->boneRenamer.apply(bone);
						remapBones.push_back((uint)_context.find_bone_index(bone));
					}
					int8* vertex = part->access_vertex_data().get_data();
					auto const & vFormat = part->get_vertex_format();
					if (vFormat.has_skinning_data())
					{
						an_assert(vFormat.is_ok_to_be_used());
						int stride = vFormat.get_stride();
						for_count(int, v, part->get_number_of_vertices())
						{
							for_count(int, i, vFormat.get_skinning_element_count())
							{
								uint skinningIndex;
								float skinningWeight;
								if (::System::VertexFormatUtils::get_skinning_info(vFormat, vertex, i, skinningIndex, skinningWeight))
								{
									if (remapBones.is_index_valid(skinningIndex))
									{
										skinningIndex = remapBones[skinningIndex];
									}
									::System::VertexFormatUtils::set_skinning_info(vFormat, vertex, i, skinningIndex, skinningWeight);
								}
							}
							vertex += stride;
						}
					}
				}
				if (part->get_index_format().get_element_size() != ::System::IndexElementSize::None)
				{
					Array<int8> & elementData = part->access_element_data();
					if (int triCount = part->get_number_of_tris())
					{
						int elementSize = ::System::IndexElementSize::to_memory_size(part->get_index_format().get_element_size());
						int8 * element = elementData.get_data();
						for (int i = 0; i < triCount; ++i)
						{
							memory_swap(element, element + elementSize * 2, elementSize);
							element += elementSize * 3;
						}
					}
					else if (part->get_number_of_vertices() != 0)
					{
						todo_important(TXT("implement_ mirroring for such case!"));
					}
				}
				else if (part->get_number_of_tris())
				{
					int8* vertex = part->access_vertex_data().get_data();
					auto const & vFormat = part->get_vertex_format();
					an_assert(vFormat.is_ok_to_be_used());
					int stride = vFormat.get_stride();
					for_count(int, t, part->get_number_of_tris())
					{
						int8* pVertex = vertex;
						vertex += stride;
						memory_swap(pVertex, vertex, stride);
						vertex += stride;
						vertex += stride;
					}
				}
			}
		}
		if ((_data->flags & Modifiers::Utils::Flags::MovementCollision) && checkpoint.movementCollisionPartsSoFarCount != now.movementCollisionPartsSoFarCount)
		{
			// for meshes apply transform and reverse order of elements (triangles, quads)
			::Collision::Model* const * pPart = &_context.get_movement_collision_parts()[checkpoint.movementCollisionPartsSoFarCount];
			for (int i = checkpoint.movementCollisionPartsSoFarCount; i < now.movementCollisionPartsSoFarCount; ++i, ++pPart)
			{
				::Collision::Model* part = *pPart;
				apply_transform_to_collision_model(part, transformMat);
				reverse_triangles_on_collision_model(part);
				part->reskin([data](Name const & _bone, Optional<Vector3> const & _loc) { return data->boneRenamer.apply(_bone); });
			}
		}
		if ((_data->flags & Modifiers::Utils::Flags::PreciseCollision) && checkpoint.preciseCollisionPartsSoFarCount != now.preciseCollisionPartsSoFarCount)
		{
			// for meshes apply transform and reverse order of elements (triangles, quads)
			::Collision::Model* const * pPart = &_context.get_precise_collision_parts()[checkpoint.preciseCollisionPartsSoFarCount];
			for (int i = checkpoint.preciseCollisionPartsSoFarCount; i < now.preciseCollisionPartsSoFarCount; ++i, ++pPart)
			{
				::Collision::Model* part = *pPart;
				apply_transform_to_collision_model(part, transformMat);
				reverse_triangles_on_collision_model(part);
				part->reskin([data](Name const& _bone, Optional<Vector3> const& _loc) { return data->boneRenamer.apply(_bone); });
			}
		}
		if ((_data->flags & Modifiers::Utils::Flags::Sockets) && checkpoint.socketsGenerationIdSoFar != now.socketsGenerationIdSoFar)
		{
			for_every(socket, _context.access_sockets().access_sockets())
			{
				if (socket->get_generation_id() > checkpoint.socketsGenerationIdSoFar)
				{
					Name remapedBone = data->boneRenamer.apply(socket->get_bone_name());
					if (socket->get_bone_name().is_valid() && socket->get_bone_name() == remapedBone && ! socket->get_placement_MS().is_set())
					{
						Transform relPlacement = socket->get_relative_placement().is_set() ? socket->get_relative_placement().get() : Transform::identity;
						socket->set_placement_MS(notMirroredBonesMS[_context.find_bone_index(socket->get_bone_name())].to_world(relPlacement)); // using not mirrored bone
						socket->apply_to_placement_MS(transformMat);
						socket->set_relative_placement(_context.get_bone_placement_MS(_context.find_bone_index(remapedBone)).to_local(socket->get_placement_MS().get())); // now with mirrored
						socket->clear_placement_MS();
					}
					else
					{
						socket->apply_to_placement_MS(transformMat);
					}
					socket->set_name(data->boneRenamer.apply(socket->get_name()));
					socket->set_bone_name(remapedBone);
				}
			}
		}
		if ((_data->flags & Modifiers::Utils::Flags::MeshNodes) && checkpoint.meshNodesSoFarCount != now.meshNodesSoFarCount)
		{
			MeshNodePtr * meshNode = &_context.access_mesh_nodes()[checkpoint.meshNodesSoFarCount];
			for (int i = checkpoint.meshNodesSoFarCount; i < now.meshNodesSoFarCount; ++i, ++meshNode)
			{
				meshNode->get()->apply(transformMat);
				meshNode->get()->name = data->boneRenamer.apply(meshNode->get()->name);
			}
		}
		if ((_data->flags & Modifiers::Utils::Flags::POIs) && checkpoint.poisSoFarCount != now.poisSoFarCount)
		{
			PointOfInterestPtr * poi = &_context.access_pois()[checkpoint.poisSoFarCount];
			for (int i = checkpoint.poisSoFarCount; i < now.poisSoFarCount; ++i, ++poi)
			{
				poi->get()->apply(transformMat);
				poi->get()->name = data->boneRenamer.apply(poi->get()->name);
				poi->get()->socketName = data->boneRenamer.apply(poi->get()->socketName);
			}
		}
		if ((_data->flags & Modifiers::Utils::Flags::SpaceBlockers) && checkpoint.spaceBlockersSoFarCount != now.spaceBlockersSoFarCount)
		{
			SpaceBlocker* sb= &_context.access_space_blockers().blockers[checkpoint.spaceBlockersSoFarCount];
			for (int i = checkpoint.spaceBlockersSoFarCount; i < now.spaceBlockersSoFarCount; ++i, ++sb)
			{
				sb->box.apply_transform(transformMat);
			}
		}
		if ((_data->flags & Modifiers::Utils::Flags::AppearanceControllers) && checkpoint.appearanceControllersSoFarCount != now.appearanceControllersSoFarCount)
		{
			AppearanceControllerDataPtr * pAC = &_context.access_appearance_controllers()[checkpoint.appearanceControllersSoFarCount];
			for (int i = checkpoint.appearanceControllersSoFarCount; i < now.appearanceControllersSoFarCount; ++i, ++pAC)
			{
				pAC->get()->apply_transform(transformMat);
				pAC->get()->reskin([data](Name const& _bone, Optional<Vector3> const& _loc) { return data->boneRenamer.apply(_bone); });
			}
		}
	}

	return result;
}

bool create_copy(GenerationContext & _context, Checkpoint const & checkpoint, Checkpoint const & now, Plane const & _mirror, MeshMirrorData const * _data)
{
	bool result = true;

	// use _mirror to check if it makes sense to create a copy - create ONLY IF IN FRONT, not at the plane
	float const threshold = Modifiers::Utils::mesh_threshold();

	if ((_data->flags & Modifiers::Utils::Flags::Mesh) && checkpoint.partsSoFarCount != now.partsSoFarCount)
	{
		// for meshes apply transform and reverse order of elements (triangles, quads)
		for (int i = checkpoint.partsSoFarCount; i < now.partsSoFarCount; ++i)
		{
			::Meshes::Mesh3DPart* part = _context.get_parts()[i].get();
			RefCountObjectPtr<::Meshes::Mesh3DPart> copy(part->create_copy());
			_context.store_part_just_as(copy.get(), part);
		}
	}
	if ((_data->flags & Modifiers::Utils::Flags::MovementCollision) && checkpoint.movementCollisionPartsSoFarCount != now.movementCollisionPartsSoFarCount)
	{
		for (int i = checkpoint.movementCollisionPartsSoFarCount; i < now.movementCollisionPartsSoFarCount; ++i)
		{
			auto* copy = _context.get_movement_collision_parts()[i]->create_copy();
			copy->remove_at_or_behind(_mirror, threshold);
			_context.store_movement_collision_part(copy);
		}
	}
	if ((_data->flags & Modifiers::Utils::Flags::PreciseCollision) && checkpoint.preciseCollisionPartsSoFarCount != now.preciseCollisionPartsSoFarCount)
	{
		for (int i = checkpoint.preciseCollisionPartsSoFarCount; i < now.preciseCollisionPartsSoFarCount; ++i)
		{
			auto* copy = _context.get_precise_collision_parts()[i]->create_copy();
			copy->remove_at_or_behind(_mirror, threshold);
			_context.store_precise_collision_part(copy);
		}
	}
	if ((_data->flags & Modifiers::Utils::Flags::Sockets) && checkpoint.socketsGenerationIdSoFar != now.socketsGenerationIdSoFar)
	{
		int socketsSoFar = _context.get_sockets().get_sockets().get_size();
		for (int i = 0; i < socketsSoFar; ++ i)
		{
			if (_context.get_sockets().get_sockets()[i].get_generation_id() > checkpoint.socketsGenerationIdSoFar)
			{
				bool shouldMirror = true;
				if (shouldMirror)
				{
					auto const & placementMS = _context.get_sockets().get_sockets()[i].get_placement_MS();
					if (placementMS.is_set())
					{
						shouldMirror &= (_mirror.get_in_front(placementMS.get().get_translation()) > threshold);
					}
				}
				if (shouldMirror)
				{
					Transform socketPlacement = _context.calculate_socket_placement_ms(i);
					shouldMirror &= (_mirror.get_in_front(socketPlacement.get_translation()) > threshold);
				}
				if (shouldMirror)
				{
					auto socket = _context.get_sockets().get_sockets()[i];
					socket.set_generation_id(_context.get_new_sockets_generation_id());
					_context.access_sockets().access_sockets().push_back(socket);
				}
			}
		}
		result = false;
	}
	if ((_data->flags & Modifiers::Utils::Flags::MeshNodes) && checkpoint.meshNodesSoFarCount != now.meshNodesSoFarCount)
	{
		for (int i = checkpoint.meshNodesSoFarCount; i < now.meshNodesSoFarCount; ++i)
		{
			if (_data->dontCutMeshNodes || _mirror.get_in_front(_context.get_mesh_nodes()[i]->placement.get_translation()) > threshold)
			{
				_context.access_mesh_nodes().push_back(MeshNodePtr(_context.get_mesh_nodes()[i]->create_copy()));
			}
		}
	}
	if ((_data->flags & Modifiers::Utils::Flags::POIs) && checkpoint.poisSoFarCount != now.poisSoFarCount)
	{
		for (int i = checkpoint.poisSoFarCount; i < now.poisSoFarCount; ++i)
		{
			if (_mirror.get_in_front(_context.get_pois()[i]->offset.get_translation()) > threshold)
			{
				_context.access_pois().push_back(PointOfInterestPtr(_context.get_pois()[i]->create_copy()));
			}
		}
	}
	if ((_data->flags & Modifiers::Utils::Flags::SpaceBlockers) && checkpoint.spaceBlockersSoFarCount != now.spaceBlockersSoFarCount)
	{
		for (int i = checkpoint.spaceBlockersSoFarCount; i < now.spaceBlockersSoFarCount; ++i)
		{
			if (_mirror.get_in_front(_context.get_space_blockers().blockers[i].box.get_centre()) > threshold)
			{
				_context.access_space_blockers().blockers.push_back(_context.get_space_blockers().blockers[i]);
			}
		}
	}
	if ((_data->flags & Modifiers::Utils::Flags::Bones) && checkpoint.bonesSoFarCount != now.bonesSoFarCount)
	{
		for (int i = checkpoint.bonesSoFarCount; i < now.bonesSoFarCount; ++i)
		{
			if (_mirror.get_in_front(_context.get_generated_bones()[i].placement.get_translation()) > threshold)
			{
				// will rename when mirroring
				_context.access_generated_bones().push_back(_context.get_generated_bones()[i]);
			}
		}
	}
	if ((_data->flags & Modifiers::Utils::Flags::AppearanceControllers) && checkpoint.appearanceControllersSoFarCount != now.appearanceControllersSoFarCount)
	{
		for (int i = checkpoint.appearanceControllersSoFarCount; i < now.appearanceControllersSoFarCount; ++i)
		{
			// will rename when mirroring
			if (auto * acCopy = _context.get_appearance_controllers()[i]->create_copy())
			{
				_context.access_appearance_controllers().push_back(AppearanceControllerDataPtr(acCopy));
			}
		}
	}

	return result;
}

//

bool Modifiers::mirror(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
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
		if (MeshMirrorData const * data = fast_cast<MeshMirrorData>(_data))
		{
			apply_mirror(_context, checkpoint, now, data);
		}
	}

	return result;
}

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_mirror_data()
{
	return new MeshMirrorData();
}

//

bool Modifiers::make_mirror(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
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
		if (MeshMirrorData const * data = fast_cast<MeshMirrorData>(_data))
		{
			Plane mirrorPlane = Plane(data->mirrorDir.get(_context).normal(), data->mirrorPoint.get(_context));

			// cut existing using plane
			Modifiers::Utils::cut_using_plane(_context, checkpoint, REF_ now, mirrorPlane, data->flags, data->dontCutMeshNodes);

			// create copy
			create_copy(_context, checkpoint, now, mirrorPlane, data);

			// apply mirror to copy
			Checkpoint copied(_context);
			apply_mirror(_context, now, copied, data);

			// check if bone names are unique
			for (int i = 0; i < _context.get_generated_bones().get_size(); ++i)
			{
				auto const & iBone = _context.get_generated_bones()[i];
				for (int j = i + 1; j < _context.get_generated_bones().get_size(); ++j)
				{
					auto const & jBone = _context.get_generated_bones()[j];
					if (iBone.boneName == jBone.boneName)
					{
						error_generating_mesh(_instigatorInstance, TXT("rename bones! mirrored \"%S\""), iBone.boneName.to_char());
						result = false;
					}
				}
			}

			// check if socket names are unique
			for (int i = 0; i < _context.get_sockets().get_sockets().get_size(); ++i)
			{
				auto const & iSocket = _context.get_sockets().get_sockets()[i];
				for (int j = i + 1; j < _context.get_sockets().get_sockets().get_size(); ++j)
				{
					auto const & jSocket = _context.get_sockets().get_sockets()[j];
					if (iSocket.get_name() == jSocket.get_name())
					{
						error_generating_mesh(_instigatorInstance, TXT("rename sockets! mirrored \"%S\""), iSocket.get_name().to_char());
						result = false;
					}
				}
			}
		}
	}

	return result;
}

//

REGISTER_FOR_FAST_CAST(MeshMirrorData);

bool MeshMirrorData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	mirrorDir.load_from_xml_child_node(_node, TXT("dir"));
	mirrorPoint.load_from_xml_child_node(_node, TXT("point"));

	if (mirrorDir.is_value_set())
	{
		if (mirrorDir.get_value().is_zero())
		{
			mirrorDir.set(Vector3::xAxis);
		}
	}

	dontCutMeshNodes = _node->get_bool_attribute_or_from_child_presence(TXT("dontCutMeshNodes"), dontCutMeshNodes);
	if (_node->get_bool_attribute_or_from_child_presence(TXT("socketsOnly")))
	{
		flags = Modifiers::Utils::Flags::Sockets;
	}

	boneRenamer.load_from_xml_child_node(_node, TXT("rename"));
	boneRenamer.load_from_xml_child_node(_node, TXT("renameBones"));

	return result;
}
