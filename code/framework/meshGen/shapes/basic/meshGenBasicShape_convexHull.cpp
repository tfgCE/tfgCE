#include "..\meshGenBasicShapes.h"

#include "meshGenBasicShapeData.h"

#include "..\..\meshGenConfig.h"
#include "..\..\meshGenGenerationContext.h"
#include "..\..\meshGenUtils.h"
#include "..\..\meshGenSubStepDef.h"
#include "..\..\meshGenSimplifyToBoxInfo.h"

#include "..\..\..\collision\physicalMaterial.h"

#include "..\..\..\..\core\collision\model.h"
#include "..\..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\..\core\mesh\builders\mesh3dBuilder_IPU.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;
using namespace BasicShapes;

//

DEFINE_STATIC_NAME(scale);
DEFINE_STATIC_NAME(applyLODToCount);
DEFINE_STATIC_NAME(vertexCount);
DEFINE_STATIC_NAME(withinBox);
DEFINE_STATIC_NAME(withinSphere);
DEFINE_STATIC_NAME(withinSphereMin);
DEFINE_STATIC_NAME(reverseSurfaces);

//

class ConvexHullData
: public Data
{
	FAST_CAST_DECLARE(ConvexHullData);
	FAST_CAST_BASE(Data);
	FAST_CAST_END();

	typedef Data base;

public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
	override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

public:
	SimplifyToBoxInfo simplifyToBoxInfo;

	ConvexHull convexHull;

	Array<Name> varIDs;
};

REGISTER_FOR_FAST_CAST(ConvexHullData);

bool ConvexHullData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	simplifyToBoxInfo.load_from_child_node_xml(_node, _lc);

	if (IO::XML::Node const * node = _node->first_child_named(TXT("convexHull")))
	{
		result &= convexHull.load_from_xml(node, true);
	}
	
	for_every(nodePoints, _node->children_named(TXT("pointsFromVars")))
	{
		for_every(node, nodePoints->children_named(TXT("var")))
		{
			Name varID = node->get_name_attribute(TXT("id"));
			if (varID.is_valid())
			{
				varIDs.push_back(varID);
			}
			else
			{
				error_loading_xml(node, TXT("no id for var"));
				result = false;
			}
		}
	}

	return result;
}

bool ConvexHullData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

//

ElementShapeData* BasicShapes::create_convex_hull_data()
{
	return new ConvexHullData();
}

//

bool BasicShapes::convex_hull(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data)
{
	int meshPartsSoFar = _context.get_parts().get_size();

	Array<int8> elementsData;
	Array<int8> vertexData;

	::Meshes::Builders::IPU ipu;

	MeshGeneration::load_ipu_parameters(ipu, _context);

	//

	bool result = true;

	Vector3 useScale = Vector3::one;
	{
		FOR_PARAM(_context, float, scale)
		{
			useScale.x *= *scale;
			useScale.y *= *scale;
			useScale.z *= *scale;
		}
		FOR_PARAM(_context, Vector3, scale)
		{
			useScale.x *= scale->x;
			useScale.y *= scale->y;
			useScale.z *= scale->z;
		}
	}

	ConvexHull convexHull;

	if (ConvexHullData const * chData = fast_cast<ConvexHullData>(_data))
	{
		convexHull = chData->convexHull;

		for_every(varID, chData->varIDs)
		{
			WheresMyPoint::Value value;
			if (_context.restore_value_for_wheres_my_point(*varID, value) &&
				value.get_type() == type_id<Vector3>())
			{
				convexHull.add(value.get_as<Vector3>());
			}
			else
			{
				error_generating_mesh(_instance, TXT("no var or of invalid type \"%S\""), varID->to_char());
				result = false;
			}
		}
	}

	float useApplyLODToCount = 0.0f;
	{
		FOR_PARAM(_context, float, applyLODToCount)
		{
			useApplyLODToCount = *applyLODToCount;
		}
	}

	int useVertexCount = 0;
	{
		FOR_PARAM(_context, int, vertexCount)
		{
			useVertexCount = *vertexCount;
		}
	}

	if (useApplyLODToCount > 0.0f && useVertexCount > 4)
	{
		float lod0 = MeshGeneration::Config::get().get_sub_step_def_modify_divider(0, 1.0f);
		float lodN = MeshGeneration::Config::get().get_sub_step_def_modify_divider(_context.get_for_lod(), 1.0f);
		if (lod0 > 0.0f && lodN > lod0)
		{
			float nto0 = lodN / lod0;
			float use = 1.0f + (nto0 - 1.0f) * useApplyLODToCount;
			useVertexCount = max(4, (int)((float)useVertexCount / use));
		}
	}

	if (useVertexCount > 0 && useVertexCount < 4)
	{
		warn_generating_mesh_element(_instance.get_element(), TXT("provided vertex count is %i, should be at least 4"), useVertexCount);
		useVertexCount = 4;
	}

	Vector3 useWithinBox = Vector3::zero;
	{
		FOR_PARAM(_context, Vector3, withinBox)
		{
			useWithinBox = *withinBox;
		}
	}

	float useWithinSphere = 0.0f;
	float useWithinSphereMin = 0.0f;
	{
		FOR_PARAM(_context, float, withinSphere)
		{
			useWithinSphere = *withinSphere;
		}
		FOR_PARAM(_context, float, withinSphereMin)
		{
			useWithinSphereMin = *withinSphereMin;
		}
	}

	bool doReverseSurfaces = false;
	{
		FOR_PARAM(_context, bool, reverseSurfaces)
		{
			doReverseSurfaces = *reverseSurfaces;
		}
	}

	//

	SubStepDef subStep;
	subStep.load_from_context(_context);

	convexHull.clear_keep_points();
	while (convexHull.get_vertex_count() < useVertexCount)
	{
		if (!useWithinBox.is_zero())
		{
			Vector3 point;
			point.x = _context.access_random_generator().get_float(-useWithinBox.x * 0.5f, useWithinBox.x * 0.5f);
			point.y = _context.access_random_generator().get_float(-useWithinBox.y * 0.5f, useWithinBox.y * 0.5f);
			point.z = _context.access_random_generator().get_float(-useWithinBox.z * 0.5f, useWithinBox.z * 0.5f);
			convexHull.add(point);
		}
		else if (useWithinSphere > 0.0f)
		{
			Vector3 point;
			point.x = _context.access_random_generator().get_float(-useWithinSphere, useWithinSphere);
			point.y = _context.access_random_generator().get_float(-useWithinSphere, useWithinSphere);
			point.z = _context.access_random_generator().get_float(-useWithinSphere, useWithinSphere);
			point = point.normal() * _context.access_random_generator().get_float(useWithinSphereMin, useWithinSphere);
			convexHull.add(point);
		}
		else
		{
			error(TXT("not enough points provided for convex hull but requested to have %i"), useVertexCount);
			break;
		}
	}

	if (useScale != Vector3::one)
	{
		Array<Vector3> points;
		points.make_space_for(convexHull.get_vertex_count());
		while (true)
		{
			Vector3 p;
			if (convexHull.get_vertex(points.get_size(), p))
			{
				points.push_back(p* useScale);
			}
			else
			{
				break;
			}
		}
		convexHull.clear();
		for_every(p, points)
		{
			convexHull.add(*p);
		}
	}

	convexHull.build();

	{
		for_count(int, i, convexHull.get_triangle_count())
		{
			Vector3 a, b, c;
			if (convexHull.get_triangle(i, OUT_ a, OUT_ b, OUT_ c))
			{
				int pointIdx = ipu.get_point_count();
				ipu.add_point(a);
				ipu.add_point(b);
				ipu.add_point(c);
				if (doReverseSurfaces)
				{
					ipu.add_triangle(0.0f, pointIdx + 2, pointIdx + 1, pointIdx);
				}
				else
				{
					ipu.add_triangle(0.0f, pointIdx, pointIdx + 1, pointIdx + 2);
				}
			}
		}
	}

	if (_context.does_require_movement_collision())
	{
		if (auto const * data = fast_cast<Data>(_data))
		{
			if (data->createMovementCollision.create)
			{
				Collision::Model* part = new Collision::Model();
				Collision::ConvexHull hull;
				hull.set(convexHull);
				hull.use_material(data->createMovementCollision.get_physical_material(_context, _instance));
				part->add(hull);
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
				Collision::Model* part = new Collision::Model();
				Collision::ConvexHull hull;
				hull.set(convexHull);
				hull.use_material(data->createPreciseCollision.get_physical_material(_context, _instance));
				part->add(hull);
				_context.store_precise_collision_part(part);
			}
			if (data->createPreciseCollisionBox.create)
			{
				_context.store_precise_collision_part(create_collision_box_from(ipu, _context, _instance, data->createPreciseCollisionBox));
			}
			if (data->createPreciseCollisionMesh.create)
			{
				_context.store_precise_collision_parts(create_collision_meshes_from(ipu, _context, _instance, data->createMovementCollisionMesh));
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
		part->debugInfo = String::printf(TXT("convex hull @ %S"), _instance.get_element()->get_location_info().to_char());
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

	if (ConvexHullData const* chData = fast_cast<ConvexHullData>(_data))
	{
		chData->simplifyToBoxInfo.process(_context, _instance, meshPartsSoFar, true);
	}

	return result;
}
