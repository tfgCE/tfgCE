#include "..\meshGenAdvancedShapes.h"

#include "..\..\meshGenGenerationContext.h"
#include "..\..\meshGenCreateCollisionInfo.h"
#include "..\..\meshGenCreateSpaceBlockerInfo.h"
#include "..\..\meshGenUtils.h"
#include "..\..\meshGenSubStepDef.h"
#include "..\..\customData\meshGenSpline.h"
#include "..\..\customData\meshGenSplineRef.h"

#include "..\..\..\library\customLibraryDatas.h"
#include "..\..\..\library\customLibraryDataLookup.h"
#include "..\..\..\library\customLibraryDataLookup.inl"

#include "..\..\..\..\core\collision\model.h"
#include "..\..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\..\core\mesh\builders\mesh3dBuilder_IPU.h"
#include "..\..\..\..\core\mesh\helpers\mesh3dHelper_bezierSurface.h"
#include "..\..\..\..\core\wheresMyPoint\wmp.h"

#ifdef AN_CLANG
DEFINE_STATIC_NAME(zero);
#include "..\..\customData\meshGenSplineImpl.inl"
#include "..\..\customData\meshGenSplineRefImpl.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;
using namespace AdvancedShapes;

//

DEFINE_STATIC_NAME(csStartsAt);
DEFINE_STATIC_NAME(csEndsAt);
DEFINE_STATIC_NAME(csForward);
DEFINE_STATIC_NAME(csRadius);
DEFINE_STATIC_NAME(csNominalRadius);
DEFINE_STATIC_NAME(csAngleOffset);

//

class CrossSectionCylindricalData
: public ElementShapeData
{
	FAST_CAST_DECLARE(CrossSectionCylindricalData);
	FAST_CAST_BASE(ElementShapeData);
	FAST_CAST_END();

	typedef ElementShapeData base;

public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
	override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

public:
	CreateCollisionInfo createMovementCollisionMesh; // will create mesh (that might be broken into convex hulls)
	CreateCollisionInfo createMovementCollisionBox; // will create mesh (that might be broken into convex hulls)
	CreateCollisionInfo createPreciseCollisionMesh; // will create mesh
	CreateCollisionInfo createPreciseCollisionBox; // will create mesh
	CreateSpaceBlockerInfo createSpaceBlocker;
	bool noMesh = false;
	bool ignoreForCollision = false;

	/**
	 *	Requires refPoints for crossSection:
	 *		zero
	 *		height (along y axis)
	 *		radius (along x axis)
	 *
	 *	Requires parameters:
	 *		startsAt
	 *		endsAt
	 *		forward (optional)
	 *		radius or nominalRadius
	 */
	CustomData::SplineRef<Vector2> halfCrossSection;
	bool reverseSurfaces = false;
	SubStepDef halfCrossSectionSubStep;
	SubStepDef cylinderSubStep;
};

//

ElementShapeData* AdvancedShapes::create_cross_section_cylindrical_data()
{
	return new CrossSectionCylindricalData();
}

//

bool AdvancedShapes::cross_section_cylindrical(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data)
{
	Array<int8> elementsData;
	Array<int8> vertexData;

	::Meshes::Builders::IPU ipu;

	MeshGeneration::load_ipu_parameters(ipu, _context);

	//

	if (CrossSectionCylindricalData const * csData = fast_cast<CrossSectionCylindricalData>(_data))
	{
		Vector3 useStartsAt = Vector3::zero;
		Vector3 useEndsAt = Vector3::zero;
		Vector3 useForward = Vector3::zero;
		float useRadius = 1.0f;
		float useNominalRadius = 1.0f;
		float useAngleOffset = 0.0f;

		bool startsAtPresent = false;
		bool endsAtPresent = false;
		bool radiusPresent = false;
		bool nominalRadiusPresent = false;
		{
			FOR_PARAM(_context, Vector3, csStartsAt)
			{
				useStartsAt = *csStartsAt;
				startsAtPresent = true;
			}
			FOR_PARAM(_context, Vector3, csEndsAt)
			{
				useEndsAt = *csEndsAt;
				endsAtPresent = true;
			}
			FOR_PARAM(_context, Vector3, csForward)
			{
				useForward = *csForward;
			}
			FOR_PARAM(_context, float, csRadius)
			{
				useRadius = *csRadius;
				radiusPresent = true;
			}
			FOR_PARAM(_context, float, csNominalRadius)
			{
				useNominalRadius = *csNominalRadius; // nominal radius works as scale
				nominalRadiusPresent = true;
			}
			FOR_PARAM(_context, float, csAngleOffset)
			{
				useAngleOffset = *csAngleOffset;
			}
		}

		if (!startsAtPresent)
		{
			error_generating_mesh(_instance, TXT("no \"csStartsAt\" defined"));
		}
		if (!endsAtPresent)
		{
			error_generating_mesh(_instance, TXT("no \"csEndsAt\" defined"));
		}
		if (!nominalRadiusPresent && !radiusPresent)
		{
			error_generating_mesh(_instance, TXT("no \"csNominalRadius\" nor \"csRadius\" defined"));
		}
		if (useStartsAt == useEndsAt)
		{
			error_generating_mesh(_instance, TXT("\"csStartsAt\" equals \"csEndsAt\""));
		}

		if (startsAtPresent && endsAtPresent &&
			(nominalRadiusPresent || radiusPresent))
		{
			// get all required parameters
			Matrix33 from = Matrix33::identity;
			DEFINE_STATIC_NAME(zero);
			if (Vector2 const * point = csData->halfCrossSection.get()->find_ref_point(NAME(zero)))
			{
				from.set_translation(*point);
			}
			DEFINE_STATIC_NAME(height);
			if (Vector2 const * point = csData->halfCrossSection.get()->find_ref_point(NAME(height)))
			{
				from.set_y_axis(*point);
			}
			DEFINE_STATIC_NAME(radius);
			if (Vector2 const * point = csData->halfCrossSection.get()->find_ref_point(NAME(radius)))
			{
				from.set_x_axis(*point);
			}
			else
			{
				from.set_x_axis(from.get_y_axis_2().rotated_right());
			}

			// calculate values for end mesh
			float height = (useEndsAt - useStartsAt).length();
			float radius = useRadius;
			if (nominalRadiusPresent)
			{
				radius = useNominalRadius * from.get_x_axis_2().length();
			}

			// ready transform so we will have cross section properly scaled
			Matrix33 to = Matrix33::identity;
			to.set_translation(Vector2::zero);
			to.set_y_axis(Vector2(0.0f, height));
			to.set_x_axis(Vector2(radius, 0.0f));

			Array<CustomData::Spline<Vector2>::Segment> csSegments;
			{
				CustomData::SplineContext<Vector2> csContext;
				csContext.transform = to.to_world(from.full_inverted());

				WheresMyPoint::Context wmpContext(&_instance);
				wmpContext.set_random_generator(_instance.access_context().access_random_generator());
				csData->halfCrossSection.get()->get_segments_for(csSegments, wmpContext, csContext);
			}

			CustomData::Spline<Vector2>::make_sure_segments_are_clockwise(csSegments);

			// change segments into actual points
			Array<Vector2> points;
			Array<UVDef> us;
			CustomData::Spline<Vector2>::change_segments_into_points(csSegments, Vector2::one /* already scaled */, csData->halfCrossSectionSubStep, _context, OUT_ points, OUT_ us, fast_cast<CrossSectionCylindricalData>(_data) && fast_cast<CrossSectionCylindricalData>(_data)->noMesh);

			// create transform matrix to place points in 3d
			// x and z are in plane of cylinder, y is from start to end along axis
			if (useForward.is_zero())
			{
				// but first make sure we will have some fallback forward axis
				Vector3 diff = useEndsAt - useStartsAt;
				if (abs(diff.y) > abs(diff.z) &&
					abs(diff.y) > abs(diff.x))
				{
					useForward = Vector3::zAxis;
				}
				else
				{
					useForward = Vector3::yAxis;
				}
			}
			Matrix44 placePointsTransform = look_at_matrix(useStartsAt, useEndsAt, useForward);
			
			// calculate required segments using radius
			int cylinderSubStepCount = max(3, csData->cylinderSubStep.calculate_sub_step_count_for(radius * 2.0f * pi<float>(), _context, NP, fast_cast<CrossSectionCylindricalData>(_data) && fast_cast<CrossSectionCylindricalData>(_data)->noMesh));

			// reserve space
			ipu.will_need_at_least_points((cylinderSubStepCount + 1) * points.get_size());
			ipu.will_need_at_least_polygons(cylinderSubStepCount * us.get_size());

			// add points and quads
			float invCylinderSubStepCount = 1.0f / (float)cylinderSubStepCount;
			for (int i = 0; i <= cylinderSubStepCount; ++i)
			{
				float angle = -90.0f + 360.0f * ((float)i - 0.5f) * invCylinderSubStepCount + useAngleOffset;
				for_every(point, points)
				{
					Vector3 p3d = Vector3(point->x, point->y, 0.0f);
					p3d.rotate_by_roll(angle);
					ipu.add_point(placePointsTransform.location_to_world(p3d));
				}
				if (i > 0)
				{
					int thisIdx = points.get_size() * i;
					int prevIdx = points.get_size() * (i - 1);
					for_every(uvDef, us)
					{
						ipu.add_quad(uvDef->get_u(_context), prevIdx + 1, prevIdx, thisIdx, thisIdx + 1, csData->reverseSurfaces);
						++thisIdx;
						++prevIdx;
					}
				}
			}
		}
	}
	
	//

	// prepare data for vertex and index formats, so further modifications may be applied and mesh can be imported
	ipu.prepare_data(_context.get_vertex_format(), _context.get_index_format(), vertexData, elementsData);

	int vertexCount = vertexData.get_size() / _context.get_vertex_format().get_stride();
	int elementsCount = elementsData.get_size() / _context.get_index_format().get_stride();

	//

	if (_context.does_require_movement_collision())
	{
		if (CrossSectionCylindricalData const * data = fast_cast<CrossSectionCylindricalData>(_data))
		{
			if (data->createMovementCollisionMesh.create)
			{
				_context.store_movement_collision_parts(create_collision_meshes_from(ipu, _context, _instance, data->createMovementCollisionMesh));
			}
			if (data->createMovementCollisionBox.create)
			{
				_context.store_movement_collision_part(create_collision_box_from(ipu, _context, _instance, data->createMovementCollisionBox));
			}
		}
	}

	if (_context.does_require_precise_collision())
	{
		if (CrossSectionCylindricalData const * data = fast_cast<CrossSectionCylindricalData>(_data))
		{
			if (data->createPreciseCollisionMesh.create)
			{
				_context.store_precise_collision_parts(create_collision_meshes_from(ipu, _context, _instance, data->createPreciseCollisionMesh));
			}
			if (data->createPreciseCollisionBox.create)
			{
				_context.store_precise_collision_part(create_collision_box_from(ipu, _context, _instance, data->createPreciseCollisionBox));
			}
		}
	}

	if (_context.does_require_space_blockers())
	{
		if (CrossSectionCylindricalData const* data = fast_cast<CrossSectionCylindricalData>(_data))
		{
			if (data->createSpaceBlocker.create)
			{
				_context.store_space_blocker(create_space_blocker_from(ipu, _context, _instance, data->createSpaceBlocker));
			}
		}
	}

	//

	if (fast_cast<CrossSectionCylindricalData>(_data) && fast_cast<CrossSectionCylindricalData>(_data)->noMesh)
	{
		vertexCount = 0;
		elementsCount = 0;
		ipu.keep_polygons(0);
	}

	//

	if (vertexCount > 0)
	{
		// create part
		Meshes::Mesh3DPart* part = _context.get_generated_mesh()->load_part_data(vertexData.get_data(), elementsData.get_data(), Meshes::Primitive::Triangle, vertexCount, elementsCount, _context.get_vertex_format(), _context.get_index_format());

#ifdef AN_DEVELOPMENT
		part->debugInfo = String::printf(TXT("cross section cylindrical @ %S"), _instance.get_element()->get_location_info().to_char());
#endif

		_context.store_part(part, _instance);

		if (CrossSectionCylindricalData const * data = fast_cast<CrossSectionCylindricalData>(_data))
		{
			if (data->ignoreForCollision)
			{
				_context.ignore_part_for_collision(part);
			}
		}
	}
	else if (! fast_cast<CrossSectionCylindricalData>(_data) || ! fast_cast<CrossSectionCylindricalData>(_data)->noMesh)
	{
		error_generating_mesh(_instance, TXT("no cross section created"));
	}

	return true;
}

//

REGISTER_FOR_FAST_CAST(CrossSectionCylindricalData);

bool CrossSectionCylindricalData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	createMovementCollisionMesh.load_from_xml(_node, TXT("createMovementCollisionMesh"), _lc);
	createMovementCollisionBox.load_from_xml(_node, TXT("createMovementCollisionBox"), _lc);
	createPreciseCollisionMesh.load_from_xml(_node, TXT("createPreciseCollisionMesh"), _lc);
	createPreciseCollisionBox.load_from_xml(_node, TXT("createPreciseCollisionBox"), _lc);
	error_loading_xml_on_assert(!_node->first_child_named(TXT("createPreciseCollisionMeshSkinned")), result, _node, TXT("createPreciseCollisionMeshSkinned not handled"));
	createSpaceBlocker.load_from_xml(_node, TXT("createSpaceBlocker"));
	noMesh = _node->get_bool_attribute_or_from_child_presence(TXT("noMesh"), noMesh);
	ignoreForCollision = _node->get_bool_attribute_or_from_child_presence(TXT("ignoreForCollision"), ignoreForCollision);

	reverseSurfaces = _node->get_bool_attribute(TXT("reverseSurfaces"), reverseSurfaces);

	result &= halfCrossSection.load_from_xml(_node, TXT("halfCrossSectionSpline"), TXT("halfCrossSectionSpline"), _lc);

	result &= halfCrossSectionSubStep.load_from_xml_child_node(_node, TXT("halfCrossSectionSubStep"));
	result &= cylinderSubStep.load_from_xml_child_node(_node, TXT("cylinderSubStep"));

	return result;
}

bool CrossSectionCylindricalData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= halfCrossSection.prepare_for_game(_library, _pfgContext);

	return result;
}