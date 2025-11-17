#include "meshGenModifierUtils.h"

#include "..\meshGenCheckpoint.h"
#include "..\meshGenGenerationContext.h"
#include "..\meshGenUtils.h"

#include "..\..\appearance\appearanceControllerData.h"

#include "..\..\world\pointOfInterest.h"

#include "..\..\..\core\collision\model.h"
#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\mesh\socketDefinitionSet.h"
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
using namespace Modifiers;
using namespace Utils;

//

bool Utils::cut_using_plane(GenerationContext& _context, ::Meshes::Mesh3DPart* part, Plane const& _plane)
{
	if (part->is_empty())
	{
		part->be_empty_part(true);
		return true;
	}
	if (part->get_index_format().get_element_size() != ::System::IndexElementSize::None)
	{
		if (part->get_number_of_elements() == 0)
		{
			part->be_empty_part(true);
			//warn(TXT("whole thing cut out earlier? maybe we removed too much already"));
			return true;
		}
		auto const vFormat = part->get_vertex_format();
		auto const iFormat = part->get_index_format();
		an_assert(iFormat.get_element_size() != ::System::IndexElementSize::None);
		int vStride = vFormat.get_stride();
		int iStride = iFormat.get_stride();
		int locOffset = vFormat.get_location_offset();
		// copy!
		Array<int8> vertexData = part->access_vertex_data();
		Array<int8> elementData = part->access_element_data();
		int vrtCount = part->get_number_of_vertices();
		int triCount = part->get_number_of_tris();
		int elCount = part->get_number_of_elements();
		int orgElCount = part->get_number_of_elements();
		if (triCount > 0)
		{
			for (int i = 0; i < triCount; ++i)
			{
				// indices for this triangle
				uint idx[] = { (uint)i * 3 + 0,
							   (uint)i * 3 + 1,
							   (uint)i * 3 + 2 };
				// actual vertices
				uint vrt[] = { ::System::IndexFormatUtils::get_value(iFormat, elementData.get_data(), idx[0]),
							   ::System::IndexFormatUtils::get_value(iFormat, elementData.get_data(), idx[1]),
							   ::System::IndexFormatUtils::get_value(iFormat, elementData.get_data(), idx[2]) };
				int8 const* vrtData[] = { vertexData.get_data() + vrt[0] * vStride,
										   vertexData.get_data() + vrt[1] * vStride,
										   vertexData.get_data() + vrt[2] * vStride };
				Vector3 loc[] = { *(Vector3*)(vrtData[0] + locOffset),
								  *(Vector3*)(vrtData[1] + locOffset),
								  *(Vector3*)(vrtData[2] + locOffset) };
				float inFront[] = { _plane.get_in_front(loc[0]),
									_plane.get_in_front(loc[1]),
									_plane.get_in_front(loc[2]) };
				if (inFront[0] <= 0.0f && inFront[1] <= 0.0f && inFront[2] <= 0.0f)
				{
					// remove this triangle
					elementData.remove_at(i * 3 * iStride, 3 * iStride);
					--i;
					--triCount;
					elCount -= 3;
					continue;
				}
				else if (inFront[0] >= 0.0f && inFront[1] >= 0.0f && inFront[2] >= 0.0f)
				{
					// all good
					continue;
				}
				else
				{

					int m[] = { 0, 1, 2 };
					// rearrange indices so we will have 0+ , 1+/- , 2-
					if (inFront[0] >= 0.0f)
					{
						if (inFront[1] >= 0.0f ||
							inFront[2] < 0.0f)
						{
							// all good
						}
						else
						{
							// 0+, 1-, 2+ -> 2+, 0+, 1-
							m[0] = 2; m[1] = 0; m[2] = 1;
						}
					}
					else
					{
						// 0-
						if (inFront[1] >= 0.0f)
						{
							// 0-, 1+, 2? -> 1+, 2?, 0-
							m[0] = 1; m[1] = 2; m[2] = 0;
						}
						else
						{
							// 0-, 1-, 2? -> 2?, 0-, 1-
							m[0] = 2; m[1] = 0; m[2] = 1;
						}
					}

					an_assert(inFront[m[2]] < 0.0f);
					if (inFront[m[1]] < 0.0f)
					{
						// we have to just cut this one, add two new vertices
						int newVrt[] = { vrtCount, vrtCount + 1 };
						vrtCount += 2;
						vertexData.grow_size(vStride * 2);
						void* newVrtData[] = { vertexData.get_data() + vStride * newVrt[0],
											   vertexData.get_data() + vStride * newVrt[1] };
						// as vrtData from block above might be invalid here after growSize
						int8 const* vrtData[] = { vertexData.get_data() + vrt[0] * vStride,
												  vertexData.get_data() + vrt[1] * vStride,
												  vertexData.get_data() + vrt[2] * vStride };

						// interpolate
						::System::VertexFormatUtils::interpolate_vertex(vFormat, newVrtData[0], vrtData[m[0]], vrtData[m[1]], calc_pt(inFront[m[0]], inFront[m[1]], 0.0f));
						::System::VertexFormatUtils::interpolate_vertex(vFormat, newVrtData[1], vrtData[m[0]], vrtData[m[2]], calc_pt(inFront[m[0]], inFront[m[2]], 0.0f));

						// replace elements
						::System::IndexFormatUtils::set_value(iFormat, elementData.get_data(), idx[m[1]], newVrt[0]);
						::System::IndexFormatUtils::set_value(iFormat, elementData.get_data(), idx[m[2]], newVrt[1]);
					}
					else
					{
						// 0+, 1+, 2-
						// we will have to add one triangle
						int newVrt[] = { vrtCount, vrtCount + 1 };
						vrtCount += 2;
						vertexData.grow_size(vStride * 2);
						void* newVrtData[] = { vertexData.get_data() + vStride * newVrt[0],
											   vertexData.get_data() + vStride * newVrt[1] };
						// as vrtData from block above might be invalid here after growSize
						int8 const* vrtData[] = { vertexData.get_data() + vrt[0] * vStride,
												  vertexData.get_data() + vrt[1] * vStride,
												  vertexData.get_data() + vrt[2] * vStride };

						// interpolate
						::System::VertexFormatUtils::interpolate_vertex(vFormat, newVrtData[0], vrtData[m[0]], vrtData[m[2]], calc_pt(inFront[m[0]], inFront[m[2]], 0.0f));
						::System::VertexFormatUtils::interpolate_vertex(vFormat, newVrtData[1], vrtData[m[1]], vrtData[m[2]], calc_pt(inFront[m[1]], inFront[m[2]], 0.0f));

						// replace element in existing triangle and add new one
						int newIdx[] = { elCount, elCount + 1, elCount + 2 };
						elementData.grow_size(iStride * 3);
						elCount += 3;
						//  1 :			  1 :
						//    :				n1
						//	  : 2	->		: 2
						//    :				n0
						//  0 :		      0 :
						::System::IndexFormatUtils::set_value(iFormat, elementData.get_data(), idx[m[2]], newVrt[0]);
						// new triangle
						::System::IndexFormatUtils::set_value(iFormat, elementData.get_data(), newIdx[0], newVrt[0]);
						::System::IndexFormatUtils::set_value(iFormat, elementData.get_data(), newIdx[1], vrt[m[1]]);
						::System::IndexFormatUtils::set_value(iFormat, elementData.get_data(), newIdx[2], newVrt[1]);
					}
				}
			}
		}
		else if (part->get_number_of_vertices() != 0)
		{
			todo_important(TXT("not mirrored - implement_ mirroring for such case!"));
		}
		if (vFormat.get_normal() != ::System::VertexFormatNormal::No)
		{
			Optional<float> smoothingDotLimit = MeshGeneration::get_smoothing_dot_limit(_context);
			if (smoothingDotLimit.is_set())
			{
				float sdl = smoothingDotLimit.get();
				int norOffset = vFormat.get_normal_offset();
				byte* loc = (byte*)vertexData.get_data() + locOffset;
				byte* nor = (byte*)vertexData.get_data() + norOffset;
				int vCount = vertexData.get_size() / vStride;
				for (int i = 0; i < vCount; ++i, loc += vStride, nor += vStride)
				{
					Vector3 const& l = *((Vector3*)loc);
					Vector3 n;
					if (vFormat.is_normal_packed())
					{
						n = ((VectorPacked*)nor)->get_as_vertex_normal();
					}
					else
					{
						n = *((Vector3*)nor);
					}
					float locInFront = _plane.get_in_front(l);
					if (abs(locInFront) < 0.001f)
					{
						Vector3 mn = _plane.get_mirrored_dir(n);
						if (Vector3::dot(n, mn) >= sdl)
						{
							n = _plane.get_dropped_dir(n).normal();
							if (vFormat.is_normal_packed())
							{
								((VectorPacked*)nor)->set_as_vertex_normal(n);
							}
							else
							{
								*((Vector3*)nor) = n;
							}
						}
					}
				}
			}
		}
		part->load_data(vertexData.get_data(), elementData.get_data(), part->get_primitive_type(), vrtCount, elCount, vFormat, iFormat);
		if (elCount == 0 && orgElCount != 0)
		{
			part->be_empty_part(true);
		}
	}
	else
	{
		todo_important(TXT("implement_ mirroring for such case!"));
	}

	return true;
}

bool Utils::cut_using_plane(GenerationContext& _context, Checkpoint const& checkpoint, REF_ Checkpoint& now, Plane const& _plane, Optional<int> const & _flags, Optional<bool> const & _dontCutMeshNodes)
{
	int flags = _flags.get(Flags::All);
	bool dontCutMeshNodes = _dontCutMeshNodes.get(false);

	bool result = true;

	// cut all behind - keep those at the plane, when making a copy, we will not copy those at the plane
	float threshold = -mesh_threshold();

	if ((flags & Flags::Mesh) && checkpoint.partsSoFarCount != now.partsSoFarCount)
	{
		// for meshes apply transform and reverse order of elements (triangles, quads)
		for (int i = checkpoint.partsSoFarCount; i < now.partsSoFarCount; ++i)
		{
			::Meshes::Mesh3DPart* part = _context.get_parts()[i].get();

			result &= cut_using_plane(_context, part, _plane);
		}
	}
	if ((flags & Flags::MovementCollision) && checkpoint.movementCollisionPartsSoFarCount != now.movementCollisionPartsSoFarCount)
	{
		for (int i = checkpoint.movementCollisionPartsSoFarCount; i < now.movementCollisionPartsSoFarCount; ++i)
		{
			_context.get_movement_collision_parts()[i]->remove_at_or_behind(_plane, threshold);
		}
	}
	if ((flags & Flags::PreciseCollision) && checkpoint.preciseCollisionPartsSoFarCount != now.preciseCollisionPartsSoFarCount)
	{
		for (int i = checkpoint.preciseCollisionPartsSoFarCount; i < now.preciseCollisionPartsSoFarCount; ++i)
		{
			_context.get_precise_collision_parts()[i]->remove_at_or_behind(_plane, threshold);
		}
	}
	if ((flags & Flags::Sockets) && checkpoint.socketsGenerationIdSoFar != now.socketsGenerationIdSoFar)
	{
		// keep them as they are?
	}
	if ((flags & Flags::MeshNodes) && checkpoint.meshNodesSoFarCount != now.meshNodesSoFarCount && ! dontCutMeshNodes)
	{
		for (int i = checkpoint.meshNodesSoFarCount; i < now.meshNodesSoFarCount; ++i)
		{
			if (_plane.get_in_front(_context.get_mesh_nodes()[i]->placement.get_translation()) <= threshold)
			{
				_context.access_mesh_nodes().remove_at(i);
				--i;
				--now.meshNodesSoFarCount;
			}
		}
	}
	if ((flags & Flags::POIs) && checkpoint.poisSoFarCount != now.poisSoFarCount)
	{
		for (int i = checkpoint.poisSoFarCount; i < now.poisSoFarCount; ++i)
		{
			if (_plane.get_in_front(_context.get_pois()[i]->offset.get_translation()) <= threshold)
			{
				_context.access_pois().remove_at(i);
				--i;
				--now.poisSoFarCount;
			}
		}
	}
	if ((flags & Flags::SpaceBlockers) && checkpoint.spaceBlockersSoFarCount != now.spaceBlockersSoFarCount)
	{
		for (int i = checkpoint.spaceBlockersSoFarCount; i < now.spaceBlockersSoFarCount; ++i)
		{
			if (_plane.get_in_front(_context.get_space_blockers().blockers[i].box.get_centre()) <= threshold)
			{
				_context.access_space_blockers().blockers.remove_at(i);
				--i;
				--now.spaceBlockersSoFarCount;
			}
		}
	}
	if ((flags & Flags::Bones) && checkpoint.bonesSoFarCount != now.bonesSoFarCount)
	{
		for (int i = checkpoint.bonesSoFarCount; i < now.bonesSoFarCount; ++i)
		{
			if (_plane.get_in_front(_context.get_generated_bones()[i].placement.get_translation()) <= threshold)
			{
				_context.access_generated_bones().remove_at(i);
				--i;
				--now.bonesSoFarCount;
			}
		}
	}
	if ((flags & Flags::AppearanceControllers) && checkpoint.appearanceControllersSoFarCount != now.appearanceControllersSoFarCount)
	{
		todo_note(TXT("keep them as they are?"));
	}

	return result;
}

bool Utils::create_copy(GenerationContext & _context, Checkpoint const & checkpoint, Checkpoint const & now, Optional<int> const& _flags)
{
	int flags = _flags.get(Modifiers::Utils::Flags::All);

	bool result = true;

	if ((flags & Modifiers::Utils::Flags::Mesh) && checkpoint.partsSoFarCount != now.partsSoFarCount)
	{
		// for meshes apply transform and reverse order of elements (triangles, quads)
		for (int i = checkpoint.partsSoFarCount; i < now.partsSoFarCount; ++i)
		{
			::Meshes::Mesh3DPart* part = _context.get_parts()[i].get();
			RefCountObjectPtr<::Meshes::Mesh3DPart> copy(part->create_copy());
			_context.store_part_just_as(copy.get(), part);
		}
	}
	if ((flags & Modifiers::Utils::Flags::MovementCollision) && checkpoint.movementCollisionPartsSoFarCount != now.movementCollisionPartsSoFarCount)
	{
		for (int i = checkpoint.movementCollisionPartsSoFarCount; i < now.movementCollisionPartsSoFarCount; ++i)
		{
			auto* copy = _context.get_movement_collision_parts()[i]->create_copy();
			_context.store_movement_collision_part(copy);
		}
	}
	if ((flags & Modifiers::Utils::Flags::PreciseCollision) && checkpoint.preciseCollisionPartsSoFarCount != now.preciseCollisionPartsSoFarCount)
	{
		for (int i = checkpoint.preciseCollisionPartsSoFarCount; i < now.preciseCollisionPartsSoFarCount; ++i)
		{
			auto* copy = _context.get_precise_collision_parts()[i]->create_copy();
			_context.store_precise_collision_part(copy);
		}
	}
	if ((flags & Modifiers::Utils::Flags::Sockets) && checkpoint.socketsGenerationIdSoFar != now.socketsGenerationIdSoFar)
	{
		int socketsSoFar = _context.get_sockets().get_sockets().get_size();
		for (int i = 0; i < socketsSoFar; ++ i)
		{
			if (_context.get_sockets().get_sockets()[i].get_generation_id() > checkpoint.socketsGenerationIdSoFar)
			{
				auto socket = _context.get_sockets().get_sockets()[i];
				socket.set_generation_id(_context.get_new_sockets_generation_id());
				_context.access_sockets().access_sockets().push_back(socket);
			}
		}
		result = false;
	}
	if ((flags & Modifiers::Utils::Flags::MeshNodes) && checkpoint.meshNodesSoFarCount != now.meshNodesSoFarCount)
	{
		for (int i = checkpoint.meshNodesSoFarCount; i < now.meshNodesSoFarCount; ++i)
		{
			_context.access_mesh_nodes().push_back(MeshNodePtr(_context.get_mesh_nodes()[i]->create_copy()));
		}
	}
	if ((flags & Modifiers::Utils::Flags::POIs) && checkpoint.poisSoFarCount != now.poisSoFarCount)
	{
		for (int i = checkpoint.poisSoFarCount; i < now.poisSoFarCount; ++i)
		{
			_context.access_pois().push_back(PointOfInterestPtr(_context.get_pois()[i]->create_copy()));
		}
	}
	if ((flags & Modifiers::Utils::Flags::SpaceBlockers) && checkpoint.spaceBlockersSoFarCount != now.spaceBlockersSoFarCount)
	{
		for (int i = checkpoint.spaceBlockersSoFarCount; i < now.spaceBlockersSoFarCount; ++i)
		{
			_context.access_space_blockers().blockers.push_back(_context.get_space_blockers().blockers[i]);
		}
	}
	if ((flags & Modifiers::Utils::Flags::Bones) && checkpoint.bonesSoFarCount != now.bonesSoFarCount)
	{
		for (int i = checkpoint.bonesSoFarCount; i < now.bonesSoFarCount; ++i)
		{
			_context.access_generated_bones().push_back(_context.get_generated_bones()[i]);
		}
	}
	if ((flags & Modifiers::Utils::Flags::AppearanceControllers) && checkpoint.appearanceControllersSoFarCount != now.appearanceControllersSoFarCount)
	{
		for (int i = checkpoint.appearanceControllersSoFarCount; i < now.appearanceControllersSoFarCount; ++i)
		{
			if (auto * acCopy = _context.get_appearance_controllers()[i]->create_copy())
			{
				_context.access_appearance_controllers().push_back(AppearanceControllerDataPtr(acCopy));
			}
		}
	}

	return result;
}

