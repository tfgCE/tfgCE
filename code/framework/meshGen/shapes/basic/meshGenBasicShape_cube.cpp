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

DEFINE_STATIC_NAME(size);
DEFINE_STATIC_NAME(topOnly);
DEFINE_STATIC_NAME(bottomOnly);
DEFINE_STATIC_NAME(nearOnly);
DEFINE_STATIC_NAME(farOnly);
DEFINE_STATIC_NAME(sidesOnly);
DEFINE_STATIC_NAME(rightOnly);
DEFINE_STATIC_NAME(leftOnly);
DEFINE_STATIC_NAME(withoutNear);
DEFINE_STATIC_NAME(withoutFar);
DEFINE_STATIC_NAME(withoutBottom);
DEFINE_STATIC_NAME(withoutTop);
DEFINE_STATIC_NAME(withoutLeft);
DEFINE_STATIC_NAME(withoutRight);
DEFINE_STATIC_NAME(withoutSides);
DEFINE_STATIC_NAME(withNear);
DEFINE_STATIC_NAME(withFar);
DEFINE_STATIC_NAME(withBottom);
DEFINE_STATIC_NAME(withSides);
DEFINE_STATIC_NAME(withTop);
DEFINE_STATIC_NAME(withLeft);
DEFINE_STATIC_NAME(withRight);
DEFINE_STATIC_NAME(reverseSurfaces);
DEFINE_STATIC_NAME(atPt);
DEFINE_STATIC_NAME(addOffset);

//

bool BasicShapes::cube(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data)
{
	Array<int8> elementsData;
	Array<int8> vertexData;

	::Meshes::Builders::IPU ipu;

	MeshGeneration::load_ipu_parameters(ipu, _context);

	//

	bool doTop = true;
	bool doBottom = true;
	bool doLeft = true;
	bool doRight = true;
	bool doNear = true;
	bool doFar = true;

	bool doReverseSurfaces = false;

	Vector3 useSize = Vector3::one;
	{
		FOR_PARAM(_context, float, size)
		{
			useSize.x = *size;
			useSize.y = *size;
			useSize.z = *size;
		}
	}
	{
		FOR_PARAM(_context, Vector3, size)
		{
			useSize = *size;
		}
	}
	{
		FOR_PARAM(_context, bool, topOnly)
		{
			if (*topOnly)
			{
				doTop = true;
				doBottom = false;
				doLeft = false;
				doRight = false;
				doNear = false;
				doFar = false;
			}
		}
	}
	{
		FOR_PARAM(_context, bool, bottomOnly)
		{
			if (*bottomOnly)
			{
				doTop = false;
				doBottom = true;
				doLeft = false;
				doRight = false;
				doNear = false;
				doFar = false;
			}
		}
	}
	{
		FOR_PARAM(_context, bool, nearOnly)
		{
			if (*nearOnly)
			{
				doTop = false;
				doBottom = false;
				doLeft = false;
				doRight = false;
				doNear = true;
				doFar = false;
			}
		}
	}
	{
		FOR_PARAM(_context, bool, farOnly)
		{
			if (*farOnly)
			{
				doTop = false;
				doBottom = false;
				doLeft = false;
				doRight = false;
				doNear = false;
				doFar = true;
			}
		}
	}
	{
		FOR_PARAM(_context, bool, sidesOnly)
		{
			if (*sidesOnly)
			{
				doTop = false;
				doBottom = false;
				doLeft = true;
				doRight = true;
				doNear = true;
				doFar = true;
			}
		}
	}
	{
		FOR_PARAM(_context, bool, rightOnly)
		{
			if (*rightOnly)
			{
				doTop = false;
				doBottom = false;
				doLeft = false;
				doRight = true;
				doNear = false;
				doFar = false;
			}
		}
	}
	{
		FOR_PARAM(_context, bool, leftOnly)
		{
			if (*leftOnly)
			{
				doTop = false;
				doBottom = false;
				doLeft = true;
				doRight = false;
				doNear = false;
				doFar = false;
			}
		}
	}
	//
	{
		FOR_PARAM(_context, bool, withoutNear)
		{
			if (*withoutNear)
			{
				doNear = false;
			}
		}
	}
	{
		FOR_PARAM(_context, bool, withoutFar)
		{
			if (*withoutFar)
			{
				doFar = false;
			}
		}
	}
	{
		FOR_PARAM(_context, bool, withoutBottom)
		{
			if (*withoutBottom)
			{
				doBottom = false;
			}
		}
	}
	{
		FOR_PARAM(_context, bool, withoutTop)
		{
			if (*withoutTop)
			{
				doTop = false;
			}
		}
	}
	{
		FOR_PARAM(_context, bool, withoutLeft)
		{
			if (*withoutLeft)
			{
				doLeft = false;
			}
		}
	}
	{
		FOR_PARAM(_context, bool, withoutRight)
		{
			if (*withoutRight)
			{
				doRight = false;
			}
		}
	}
	{
		FOR_PARAM(_context, bool, withoutSides)
		{
			if (*withoutSides)
			{
				doLeft = false;
				doRight = false;
				doNear = false;
				doFar = false;
			}
		}
	}
	//
	{
		FOR_PARAM(_context, bool, withTop)
		{
			if (*withTop)
			{
				doTop = true;
			}
		}
	}
	{
		FOR_PARAM(_context, bool, withBottom)
		{
			if (*withBottom)
			{
				doBottom = true;
			}
		}
	}
	{
		FOR_PARAM(_context, bool, withNear)
		{
			if (*withNear)
			{
				doNear = true;
			}
		}
	}
	{
		FOR_PARAM(_context, bool, withFar)
		{
			if (*withFar)
			{
				doFar = true;
			}
		}
	}
	{
		FOR_PARAM(_context, bool, withLeft)
		{
			if (*withLeft)
			{
				doLeft = true;
			}
		}
	}
	{
		FOR_PARAM(_context, bool, withRight)
		{
			if (*withRight)
			{
				doRight = true;
			}
		}
	}
	//
	{
		FOR_PARAM(_context, bool, reverseSurfaces)
		{
			doReverseSurfaces = *reverseSurfaces;
		}
	}
	//
	Optional<Vector3> useAtPt;
	{
		FOR_PARAM(_context, Vector3, atPt)
		{
			useAtPt = *atPt;
		}
	}
	//
	Optional<Vector3> useAddOffset;
	{
		FOR_PARAM(_context, Vector3, addOffset)
		{
			useAddOffset = *addOffset;
		}
	}

	//

	{
		Vector3 hs = useSize * 0.5f;
		Vector3 offset = Vector3::zero;

		if (useAtPt.is_set())
		{
			offset = -(useAtPt.get() - Vector3::one * 0.5f) * useSize;
		}
		if (useAddOffset.is_set())
		{
			offset += useAddOffset.get();
		}

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

	if (! doReverseSurfaces)
	{
		// bottom, top
		if (doBottom) { ipu.add_quad(0.0f, 0, 3, 2, 1); }
		if (doTop) { ipu.add_quad(0.0f, 4, 5, 6, 7); }

		// left, right
		if (doLeft) { ipu.add_quad(0.0f, 0, 1, 5, 4); }
		if (doRight) { ipu.add_quad(0.0f, 3, 7, 6, 2); }

		// front, back
		if (doNear) { ipu.add_quad(0.0f, 0, 4, 7, 3); }
		if (doFar) { ipu.add_quad(0.0f, 2, 6, 5, 1); }
	}
	else
	{
		// bottom, top
		if (doBottom) { ipu.add_quad(0.0f, 0, 1, 2, 3); }
		if (doTop) { ipu.add_quad(0.0f, 4, 7, 6, 5); }

		// left, right
		if (doLeft) { ipu.add_quad(0.0f, 0, 4, 5, 1); }
		if (doRight) { ipu.add_quad(0.0f, 3, 2, 6, 7); }

		// front, back
		if (doNear) { ipu.add_quad(0.0f, 0, 3, 7, 4); }
		if (doFar) { ipu.add_quad(0.0f, 2, 1, 5, 6); }
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
