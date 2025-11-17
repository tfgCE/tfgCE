#include "..\meshGenBasicShapes.h"

#include "meshGenBasicShapeData.h"

#include "..\..\meshGenGenerationContext.h"
#include "..\..\meshGenUtils.h"
#include "..\..\meshGenSubStepDef.h"

#include "..\..\..\collision\physicalMaterial.h"

#include "..\..\..\..\core\collision\model.h"
#include "..\..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\..\core\mesh\builders\mesh3dBuilder_IPU.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace BasicShapes;

//

DEFINE_STATIC_NAME(radius);
DEFINE_STATIC_NAME(length);
DEFINE_STATIC_NAME(height);

//

bool BasicShapes::cylinder(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data)
{
	Array<int8> elementsData;
	Array<int8> vertexData;

	::Meshes::Builders::IPU ipu;

	MeshGeneration::load_ipu_parameters(ipu, _context);

	//

	float useRadius = 1.0f;
	{
		FOR_PARAM(_context, float, radius)
		{
			useRadius = *radius;
		}
	}

	float useLength = 1.0f;
	{
		FOR_PARAM(_context, float, length)
		{
			useLength = *length;
		}
		FOR_PARAM(_context, float, height)
		{
			useLength = *height;
		}
	}

	//

	SubStepDef subStep;
	subStep.load_from_context(_context);
	int useSubStepCount = subStep.calculate_sub_step_count_for(useRadius * pi<float>() * 2.0f, _context, NP, fast_cast<Data>(_data) && fast_cast<Data>(_data)->noMesh); // quarter of circumference

	{
		// we will end up with doubled points on ha = 0.0 and on top and bottom
		int hiMax = max(3, useSubStepCount);
		ipu.will_need_at_least_points((hiMax + 1) * 2 + 2);
		float hiMaxInverted = 1.0f / (float)hiMax;
		for (int side = 0; side < 2; ++side)
		{
			float sideA = useLength * 0.5f * (side == 0 ? -1.0f : 1.0f);
			ipu.add_point(Vector3(0.0f, 0.0f, sideA));
			for (int hi = 0; hi <= hiMax; hi++)
			{
				float ha = 360.0f * (float)hi * hiMaxInverted;
				float hc = cos_deg(ha);
				float hs = sin_deg(ha);
				ipu.add_point(Vector3(hs * useRadius, hc * useRadius, sideA));
			}
		}

		for (int side = 0; side < 2; ++side)
		{
			int centre = (hiMax + 2) * side;
			int edge = centre + 1;
			for (int hi = 0; hi < hiMax; hi++)
			{
				if (side == 0)
				{
					ipu.add_triangle(0.0f, centre, edge + 1, edge);
				}
				else
				{
					ipu.add_triangle(0.0f, centre, edge, edge + 1);
				}
				++ edge;
			}
		}

		{
			int lower = 1;
			int upper = (hiMax + 2) + 1;
			for (int hi = 0; hi < hiMax; hi++)
			{
				ipu.add_quad(0.0f, lower, lower + 1, upper + 1, upper);
				++ lower;
				++ upper;
			}
		}
	}

	if (_context.does_require_movement_collision())
	{
		if (auto const * data = fast_cast<Data>(_data))
		{
			if (data->createMovementCollision.create)
			{
				todo_note(TXT("this is not a cylinder, it is an approximation using capsule!"));
				Collision::Model* part = new Collision::Model();
				Collision::Capsule capsule;
				float radius = min(useRadius, 0.5f * useLength);
				float halfLength = 0.5f * useLength - radius;
				Vector3 half(0.0f, 0.0f, halfLength);
				capsule.setup(-half, half, radius);
				capsule.use_material(data->createMovementCollision.get_physical_material(_context, _instance));
				part->add(capsule);
				_context.store_movement_collision_part(part);
			}
			if (data->createMovementCollisionBox.create)
			{
				_context.store_movement_collision_part(create_collision_box_from(ipu, _context, _instance, data->createMovementCollisionBox));
			}
			if (data->createMovementCollisionMesh.create)
			{
				_context.store_movement_collision_parts(create_collision_meshes_from(ipu, _context, _instance, data->createMovementCollisionMesh));
			}
		}
	}

	if (_context.does_require_precise_collision())
	{
		if (auto const * data = fast_cast<Data>(_data))
		{
			if (data->createPreciseCollision.create)
			{
				todo_note(TXT("this is not a cylinder, it is an approximation using capsule!"));
				Collision::Model* part = new Collision::Model();
				Collision::Capsule capsule;
				float radius = min(useRadius, 0.5f * useLength);
				float halfLength = 0.5f * useLength - radius;
				Vector3 half(0.0f, 0.0f, halfLength);
				capsule.setup(-half, half, radius);
				capsule.use_material(data->createPreciseCollision.get_physical_material(_context, _instance));
				part->add(capsule);
				_context.store_precise_collision_part(part);
			}
			if (data->createPreciseCollisionBox.create)
			{
				_context.store_precise_collision_part(create_collision_box_from(ipu, _context, _instance, data->createPreciseCollisionBox));
			}
			if (data->createPreciseCollisionMesh.create)
			{
				_context.store_precise_collision_parts(create_collision_meshes_from(ipu, _context, _instance, data->createPreciseCollisionMesh));
			}
		}
	}

	if (_context.does_require_space_blockers())
	{
		if (auto const* data = fast_cast<Data>(_data))
		{
			if (data->createSpaceBlocker.create)
			{
				_context.store_space_blocker(create_space_blocker_from(ipu, _context, _instance, data->createSpaceBlocker));
			}
		}
	}

	if (fast_cast<Data>(_data) && fast_cast<Data>(_data)->noMesh)
	{
		ipu.keep_polygons(0);
	}

	//

	// prepare data for vertex and index formats, so further modifications may be applied and mesh can be imported
	ipu.prepare_data(_context.get_vertex_format(), _context.get_index_format(), vertexData, elementsData);

	int vertexCount = vertexData.get_size() / _context.get_vertex_format().get_stride();
	int elementsCount = elementsData.get_size() / _context.get_index_format().get_stride();

	//

	if (!ipu.is_empty())
	{
		// create part
		Meshes::Mesh3DPart* part = _context.get_generated_mesh()->load_part_data(vertexData.get_data(), elementsData.get_data(), Meshes::Primitive::Triangle, vertexCount, elementsCount, _context.get_vertex_format(), _context.get_index_format());

#ifdef AN_DEVELOPMENT
		part->debugInfo = String::printf(TXT("cylinder @ %S"), _instance.get_element()->get_location_info().to_char());
#endif

		_context.store_part(part, _instance);

		if (auto const * data = fast_cast<Data>(_data))
		{
			if (data->ignoreForCollision)
			{
				_context.ignore_part_for_collision(part);
			}
		}
	}

	return true;
}
