#include "..\meshGenBasicShapes.h"

#include "meshGenBasicShapeData.h"

#include "..\..\meshGenGenerationContext.h"
#include "..\..\meshGenUtils.h"

#include "..\..\..\collision\physicalMaterial.h"

#include "..\..\..\..\core\collision\model.h"
#include "..\..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\..\core\mesh\builders\mesh3dBuilder_IPU.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace BasicShapes;

//

bool BasicShapes::bounding_box(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data)
{
	Array<int8> elementsData;
	Array<int8> vertexData;

	::Meshes::Builders::IPU ipu;

	MeshGeneration::load_ipu_parameters(ipu, _context);

	//

	Range3 range = Range3::empty;

	Checkpoint checkpoint(_context);
	checkpoint = _context.get_checkpoint(1); // of parent

	Checkpoint now(_context);

	{
		for (int i = checkpoint.partsSoFarCount; i < now.partsSoFarCount; ++i)
		{
			::Meshes::Mesh3DPart* part = _context.get_parts()[i].get();
			int8* vertex = part->access_vertex_data().get_data();
			auto const& vFormat = part->get_vertex_format();
			int offset = vFormat.get_location_offset();
			int stride = vFormat.get_stride();
			int8* location = vertex + offset;
			for_count(int, v, part->get_number_of_vertices())
			{
				Vector3 loc = Vector3::zero;
				if (vFormat.get_location() == ::System::VertexFormatLocation::XY)
				{
					memory_copy(&loc.x, location, 2 * sizeof(float));
				}
				else if (vFormat.get_location() == ::System::VertexFormatLocation::XYZ)
				{
					memory_copy(&loc.x, location, 3 * sizeof(float));
				}
				else
				{
					todo_implement;
				}
				range.include(loc);
				location += stride;
			}
		}
	}

	if (range.is_empty())
	{
		return true;
	}

	Vector3 useSize = range.length();
	Vector3 offset = range.centre();

	//

	{
		Vector3 hs = useSize * 0.5f;

		{
			ipu.add_point(Vector3(-hs.x, -hs.y, -hs.z) + offset);
			ipu.add_point(Vector3(-hs.x,  hs.y, -hs.z) + offset);
			ipu.add_point(Vector3( hs.x,  hs.y, -hs.z) + offset);
			ipu.add_point(Vector3( hs.x, -hs.y, -hs.z) + offset);

			ipu.add_point(Vector3(-hs.x, -hs.y,  hs.z) + offset);
			ipu.add_point(Vector3(-hs.x,  hs.y,  hs.z) + offset);
			ipu.add_point(Vector3( hs.x,  hs.y,  hs.z) + offset);
			ipu.add_point(Vector3( hs.x, -hs.y,  hs.z) + offset);
		}

		if (_context.does_require_movement_collision())
		{
			if (auto const * data = fast_cast<Data>(_data))
			{
				if (data->createMovementCollision.create)
				{
					Collision::Model* part = new Collision::Model();
					Collision::Box box;
					box.setup(offset, hs, Vector3::xAxis, Vector3::yAxis);
					box.use_material(data->createMovementCollision.get_physical_material(_context, _instance));
					part->add(box);
					_context.store_movement_collision_part(part);
				}
				if (data->createMovementCollisionBox.create)
				{
					Collision::Model* part = new Collision::Model();
					Collision::Box box;
					box.setup(offset, hs, Vector3::xAxis, Vector3::yAxis);
					box.use_material(data->createMovementCollisionBox.get_physical_material(_context, _instance));
					part->add(box);
					_context.store_movement_collision_part(part);
				}
			}
		}

		if (_context.does_require_precise_collision())
		{
			if (auto const * data = fast_cast<Data>(_data))
			{
				if (data->createPreciseCollision.create)
				{
					Collision::Model* part = new Collision::Model();
					Collision::Box box;
					box.setup(offset, hs, Vector3::xAxis, Vector3::yAxis);
					box.use_material(data->createPreciseCollision.get_physical_material(_context, _instance));
					part->add(box);
					_context.store_precise_collision_part(part);
				}
				if (data->createPreciseCollisionBox.create)
				{
					Collision::Model* part = new Collision::Model();
					Collision::Box box;
					box.setup(offset, hs, Vector3::xAxis, Vector3::yAxis);
					box.use_material(data->createPreciseCollisionBox.get_physical_material(_context, _instance));
					part->add(box);
					_context.store_precise_collision_part(part);
				}
			}
		}

		if (_context.does_require_space_blockers())
		{
			if (auto const* data = fast_cast<Data>(_data))
			{
				if (data->createSpaceBlocker.create)
				{
					SpaceBlocker sb;
					sb.box.setup(offset, hs, Vector3::xAxis, Vector3::yAxis);
					sb.blocks = data->createSpaceBlocker.get_blocks(_context);
					sb.requiresToBeEmpty = data->createSpaceBlocker.get_requires_to_be_empty(_context);
					_context.store_space_blocker(sb);
				}
			}
		}
	}

	{
		// bottom, top
		ipu.add_quad(0.0f, 0, 3, 2, 1);
		ipu.add_quad(0.0f, 4, 5, 6, 7);

		// left, right
		ipu.add_quad(0.0f, 0, 1, 5, 4);
		ipu.add_quad(0.0f, 3, 7, 6, 2);

		// front, back
		ipu.add_quad(0.0f, 0, 4, 7, 3);
		ipu.add_quad(0.0f, 2, 6, 5, 1);
	}

	if (_context.does_require_movement_collision())
	{
		if (fast_cast<Data>(_data) && fast_cast<Data>(_data)->createMovementCollisionMesh.create)
		{
			_context.store_movement_collision_parts(create_collision_meshes_from(ipu, _context, _instance, fast_cast<Data>(_data)->createMovementCollisionMesh));
		}
	}

	if (_context.does_require_precise_collision())
	{
		if (fast_cast<Data>(_data) && fast_cast<Data>(_data)->createPreciseCollisionMesh.create)
		{
			_context.store_precise_collision_parts(create_collision_meshes_from(ipu, _context, _instance, fast_cast<Data>(_data)->createPreciseCollisionMesh));
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

	if (vertexCount)
	{
		// create part
		Meshes::Mesh3DPart* part = _context.get_generated_mesh()->load_part_data(vertexData.get_data(), elementsData.get_data(), Meshes::Primitive::Triangle, vertexCount, elementsCount, _context.get_vertex_format(), _context.get_index_format());

#ifdef AN_DEVELOPMENT
		part->debugInfo = String::printf(TXT("cube @ %S"), _instance.get_element()->get_location_info().to_char());
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
