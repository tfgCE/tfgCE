#include "..\meshGenAdvancedShapes.h"

#include "..\..\meshGenGenerationContext.h"
#include "..\..\meshGenCreateCollisionInfo.h"
#include "..\..\meshGenCreateSpaceBlockerInfo.h"
#include "..\..\meshGenParamImpl.inl"
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

//

class CrossSectionsContext;

struct CrossSectionPoint
{
	Name id;
	Vector3 point = Vector3::zero;
	MeshGenParam<float> u;
	MeshGenParam<float> uPrev;
	MeshGenParam<float> uNext;
	MeshGenParam<Name> skinTo;
	MeshGenParam<bool> noPolygon;
	MeshGenParam<bool> noPrevPolygon;
	MeshGenParam<bool> noNextPolygon;
	WheresMyPoint::ToolSet wmpToolSet;
};

struct CrossSection
{
	Name id;
	List<CrossSectionPoint> points;
	MeshGenParam<Transform> placement;
	MeshGenParam<float> u;
	MeshGenParam<Name> skinTo;
};

struct CrossSectionPointInstance
{
	Name id;
	Vector3 point = Vector3::zero;
	Optional<float> u;
	Optional<float> uPrev;
	Optional<float> uNext;
	Optional<bool> noPolygon;
	Optional<bool> noPrevPolygon;
	Optional<bool> noNextPolygon;
	int ipuIdx = NONE;
	// prev and next might be not aligned
	float anglePrev = 0.0f;
	float angleNext = 0.0f;
};

struct CrossSectionInstance
{
	Array<CrossSectionPointInstance> points;
	Vector3 centre;
	Optional<Transform> placement;
	Optional<float> u;
	int startingPrevIdx = 0;
	int startingNextIdx = 0;

	void find_starting_index(bool _next)
	{
		int bestIdx = NONE;
		float lowestAngle = 360.0f;
		for_every(p, points)
		{
			int idx = for_everys_index(p);
			float & angle = _next ? p->angleNext : p->anglePrev;
			angle = mod(angle, 360.0f);
			if (bestIdx == NONE || angle < lowestAngle)
			{
				bestIdx = idx;
				lowestAngle = angle;
			}
		}
		int& startingIdx = _next ? startingNextIdx : startingPrevIdx;
		startingIdx = bestIdx;
	}
};

class CrossSectionsData
: public ElementShapeData
{
	FAST_CAST_DECLARE(CrossSectionsData);
	FAST_CAST_BASE(ElementShapeData);
	FAST_CAST_END();

	typedef ElementShapeData base;

public:
	bool create(CrossSectionsContext & _context) const;

	Optional<Vector3> find_point(Name const& _id) const;

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

	bool reverseSurfaces = false;

	List<CrossSection> crossSections;

#ifdef AN_DEVELOPMENT
	bool debugDrawPoints = false;
	float debugDrawPointsTextScale = 1.0f;
#endif
};

class CrossSectionsContext
: public WheresMyPoint::IOwner
{
	FAST_CAST_DECLARE(CrossSectionsContext);
	FAST_CAST_BASE(WheresMyPoint::IOwner);
	FAST_CAST_END();

	typedef WheresMyPoint::IOwner base;
public:
	CrossSectionsContext(GenerationContext& _generationContext, ElementInstance& _elementInstance, CrossSectionsData const* _crossSectionsData, ::Meshes::Builders::IPU& _ipu);
	~CrossSectionsContext();

	GenerationContext& access_generation_context() { return generationContext; }
	ElementInstance& access_element_instance() { return elementInstance; }
	CrossSectionsData const* get_cross_sectionss_data() const { return crossSectionsData; }
	::Meshes::Builders::IPU& access_ipu() { return ipu; }

public: // WheresMyPoint::IOwner
	override_ bool get_point_for_wheres_my_point(Name const& _byName, OUT_ Vector3& _point) const;
	override_ bool store_value_for_wheres_my_point(Name const& _byName, WheresMyPoint::Value const& _value);
	override_ bool restore_value_for_wheres_my_point(Name const& _byName, OUT_ WheresMyPoint::Value& _value) const;
	override_ bool store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to);
	override_ bool rename_value_forwheres_my_point(Name const& _from, Name const& _to);
	override_ bool store_global_value_for_wheres_my_point(Name const& _byName, WheresMyPoint::Value const& _value);
	override_ bool restore_global_value_for_wheres_my_point(Name const& _byName, OUT_ WheresMyPoint::Value& _value) const;

	override_ IOwner* get_wmp_owner() { return &elementInstance; }

private:
	GenerationContext& generationContext;
	ElementInstance& elementInstance;
	CrossSectionsData const* crossSectionsData;
	::Meshes::Builders::IPU& ipu;
};

//

REGISTER_FOR_FAST_CAST(CrossSectionsContext);

CrossSectionsContext::CrossSectionsContext(GenerationContext& _generationContext, ElementInstance& _elementInstance, CrossSectionsData const* _crossSectionsData, ::Meshes::Builders::IPU& _ipu)
: generationContext(_generationContext)
, elementInstance(_elementInstance)
, crossSectionsData(_crossSectionsData)
, ipu(_ipu)
{
}

CrossSectionsContext::~CrossSectionsContext()
{
}

bool CrossSectionsContext::get_point_for_wheres_my_point(Name const& _byName, OUT_ Vector3& _point) const
{
	Optional<Vector3> point = crossSectionsData->find_point(_byName);
	if (point.is_set())
	{
		_point = point.get();
		return true;
	}
	else
	{
		return false;
	}
}

bool CrossSectionsContext::store_value_for_wheres_my_point(Name const& _byName, WheresMyPoint::Value const& _value)
{
	return elementInstance.access_context().store_value_for_wheres_my_point(_byName, _value);
}

bool CrossSectionsContext::restore_value_for_wheres_my_point(Name const& _byName, OUT_ WheresMyPoint::Value& _value) const
{
	return elementInstance.access_context().restore_value_for_wheres_my_point(_byName, OUT_ _value);
}

bool CrossSectionsContext::store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to)
{
	return elementInstance.access_context().store_convert_value_for_wheres_my_point(_byName, _to);
}

bool CrossSectionsContext::rename_value_forwheres_my_point(Name const& _from, Name const& _to)
{
	return elementInstance.access_context().rename_value_forwheres_my_point(_from, _to);
}

bool CrossSectionsContext::store_global_value_for_wheres_my_point(Name const& _byName, WheresMyPoint::Value const& _value)
{
	return elementInstance.access_context().store_global_value_for_wheres_my_point(_byName, _value);
}

bool CrossSectionsContext::restore_global_value_for_wheres_my_point(Name const& _byName, OUT_ WheresMyPoint::Value& _value) const
{
	return elementInstance.access_context().restore_global_value_for_wheres_my_point(_byName, OUT_ _value);
}

//

Optional<Vector3> CrossSectionsData::find_point(Name const& _id) const
{
	for_every(cs, crossSections)
	{
		for_every(p, cs->points)
		{
			if (p->id == _id)
			{
				return p->point;
			}
		}
	}

	return NP;
}

bool CrossSectionsData::create(CrossSectionsContext& _csContext) const
{
	GenerationContext& _context = _csContext.access_generation_context();
	ElementInstance& _instance = _csContext.access_element_instance();
	::Meshes::Builders::IPU& ipu = _csContext.access_ipu();

	bool result = true;

	Array<CrossSectionInstance> crossSectionInstances;
	crossSectionInstances.set_size(crossSections.get_size());

	int rgIdx = 0;

	// prepare cross section instances
	for_every(csi, crossSectionInstances)
	{
		CrossSection const& cs = crossSections[for_everys_index(csi)];
		csi->centre = Vector3::zero;
		float weight = 0.0f;
		Meshes::VertexSkinningInfo csvsi;

		if (cs.skinTo.is_set())
		{
			int b = _context.find_bone_index(cs.skinTo.get(&_instance, Name::invalid()));
			if (b != NONE)
			{
				csvsi.add(Meshes::VertexSkinningInfo::Bone(b, 1.0f));
			}
		}

		for_every(p, cs.points)
		{
			Vector3 point = p->point;
			if (!p->wmpToolSet.is_empty())
			{
				WheresMyPoint::Context wmpContext(&_csContext); // use this context as we will be accessing points!
				wmpContext.set_random_generator(_context.access_random_generator());
				wmpContext.access_random_generator().advance_seed(rgIdx, rgIdx);
				++rgIdx;
				WheresMyPoint::Value value;
				value.set(point);
				p->wmpToolSet.update(value, wmpContext);
				if (value.get_type() == type_id<Vector3>())
				{
					point = value.get_as<Vector3>();
				}
				else
				{
					error_generating_mesh(_instance, TXT("wmp resulted in non Vector3 point"));
					result = false;
				}
			}
			{
				Meshes::VertexSkinningInfo* vsi = &csvsi;
				Meshes::VertexSkinningInfo pvsi;

				if (p->skinTo.is_set())
				{
					int b = _context.find_bone_index(p->skinTo.get(&_instance, Name::invalid()));
					if (b != NONE)
					{
						pvsi.add(Meshes::VertexSkinningInfo::Bone(b, 1.0f));
						vsi = &pvsi;
					}
				}

				CrossSectionPointInstance cspi;
				cspi.id = p->id;
				cspi.point = point;
				cspi.ipuIdx = ipu.add_point(point, NP, vsi);
				if (p->noPolygon.is_set()) { cspi.noPolygon = p->noPolygon.get(&_instance, 0.0f); }
				if (p->noPrevPolygon.is_set()) { cspi.noPrevPolygon = p->noPrevPolygon.get(&_instance, 0.0f); }
				if (p->noNextPolygon.is_set()) { cspi.noNextPolygon = p->noNextPolygon.get(&_instance, 0.0f); }
				if (p->u.is_set()) { cspi.u = p->u.get(&_instance, 0.0f); }
				if (p->uPrev.is_set()) { cspi.uPrev = p->uPrev.get(&_instance, 0.0f); }
				if (p->uNext.is_set()) { cspi.uNext = p->uNext.get(&_instance, 0.0f); }
				csi->points.push_back(cspi);
#ifdef AN_DEVELOPMENT
				if (debugDrawPoints)
				{
					if (auto* drf = _context.debugRendererFrame)
					{
						drf->add_sphere(true, true, Colour::purple, 0.3f, Sphere(cspi.point, 0.02f));
						if (cspi.id.is_valid())
						{
							drf->add_text(true, Colour::purple.blended_to(Colour::white, 0.5f), cspi.point, NP, true, 0.1f * debugDrawPointsTextScale, NP, cspi.id.to_char());
						}
					}
				}
#endif
			}
			csi->centre += point;
			if (cs.placement.is_set())
			{
				csi->placement = cs.placement.get(&_instance, Transform::identity);
			}
			if (cs.u.is_set())
			{
				csi->u = cs.u.get(&_instance, 0.0f);
			}
			weight += 1.0f;
		}
		if (weight != 0.0f)
		{
			csi->centre /= weight;
		}
	}

	// calculate connecting lines and angles
	for_count(int, i, crossSectionInstances.get_size() - 1)
	{
		auto& prevCSI = crossSectionInstances[i];
		auto& nextCSI = crossSectionInstances[i + 1];

		Vector3 dir = (nextCSI.centre - prevCSI.centre).normal();
		Vector3 up = abs(dir.z) < 0.7f ? Vector3::zAxis : Vector3::yAxis;
		Transform placement = prevCSI.placement.is_set()? prevCSI.placement.get() : look_matrix(prevCSI.centre, dir, up).to_transform();

		for_every(p, prevCSI.points)
		{
			Vector3 local = placement.location_to_local(p->point);
			Vector2 l2(local.x, local.z);
			p->angleNext = l2.angle();
		}

		for_every(p, nextCSI.points)
		{
			Vector3 local = placement.location_to_local(p->point);
			Vector2 l2(local.x, local.z);
			p->anglePrev = l2.angle();
		}

		prevCSI.find_starting_index(true);
		nextCSI.find_starting_index(false);
	}

	// build triangles
	for_count(int, i, crossSectionInstances.get_size() - 1)
	{
		auto& prevCSI = crossSectionInstances[i];
		auto& nextCSI = crossSectionInstances[i + 1];

		/*
		 *	starts with the lowest angle, out of two next available, chooses the lower
		 */
		int prevIdx = prevCSI.startingNextIdx;
		int nextIdx = nextCSI.startingPrevIdx;
		bool prevEnd = false;
		bool nextEnd = false;
		float prevAngle = prevCSI.points[prevIdx].angleNext;
		float nextAngle = nextCSI.points[nextIdx].anglePrev;
		while (true)
		{
			int pIdx = mod(prevIdx + 1, prevCSI.points.get_size());
			int nIdx = mod(nextIdx + 1, nextCSI.points.get_size());
			float pAngle = prevCSI.points[prevIdx].angleNext; if (pIdx == prevCSI.startingNextIdx) pAngle += 360.0f;
			float nAngle = nextCSI.points[nextIdx].anglePrev; if (nIdx == nextCSI.startingPrevIdx) nAngle += 360.0f;

			if ((pAngle <= nAngle && ! prevEnd) || nextEnd)
			{
				if (!prevCSI.points[prevIdx].noNextPolygon.get(prevCSI.points[prevIdx].noPolygon.get(false)))
				{
					int a = prevCSI.points[prevIdx].ipuIdx;
					int b = nextCSI.points[nextIdx].ipuIdx;
					int c = prevCSI.points[pIdx].ipuIdx;
					ipu.add_triangle(prevCSI.points[prevIdx].uNext.get(prevCSI.points[prevIdx].u.get(prevCSI.u.get(0.0f))), a, b, c);
				}
				prevIdx = pIdx;
				prevEnd = prevIdx == prevCSI.startingNextIdx;
				prevAngle = pAngle;
			}
			else if ((nAngle <= pAngle && ! nextEnd) || prevEnd)
			{
				if (!nextCSI.points[nextIdx].noPrevPolygon.get(nextCSI.points[nextIdx].noPolygon.get(false)))
				{
					int a = prevCSI.points[prevIdx].ipuIdx;
					int b = nextCSI.points[nextIdx].ipuIdx;
					int c = nextCSI.points[nIdx].ipuIdx;
					ipu.add_triangle(nextCSI.points[nextIdx].uPrev.get(nextCSI.points[nextIdx].u.get(prevCSI.u.get(0.0f))), a, b, c);
				}
				nextIdx = nIdx;
				nextEnd = nextIdx == nextCSI.startingPrevIdx;
				nextAngle = nAngle;
			}
			else
			{
				error_generating_mesh(_instance, TXT("could not build triangles for cross sections mesh gen shape"));
				result = false;
				break;
			}

			if (prevIdx == prevCSI.startingNextIdx &&
				nextIdx == nextCSI.startingPrevIdx)
			{
				break;
			}
		}
	}

	return result;
}

//

ElementShapeData* AdvancedShapes::create_cross_sections_data()
{
	return new CrossSectionsData();
}

//

bool AdvancedShapes::cross_sections(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data)
{
	Array<int8> elementsData;
	Array<int8> vertexData;

	::Meshes::Builders::IPU ipu;

	MeshGeneration::load_ipu_parameters(ipu, _context);

	//

	if (CrossSectionsData const * csData = fast_cast<CrossSectionsData>(_data))
	{
		CrossSectionsContext csc(_context, _instance, csData, ipu);
		csData->create(csc);
	}
	
	//

	// prepare data for vertex and index formats, so further modifications may be applied and mesh can be imported
	ipu.prepare_data(_context.get_vertex_format(), _context.get_index_format(), vertexData, elementsData);

	int vertexCount = vertexData.get_size() / _context.get_vertex_format().get_stride();
	int elementsCount = elementsData.get_size() / _context.get_index_format().get_stride();

	//

	if (_context.does_require_movement_collision())
	{
		if (CrossSectionsData const * data = fast_cast<CrossSectionsData>(_data))
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
		if (CrossSectionsData const * data = fast_cast<CrossSectionsData>(_data))
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
		if (CrossSectionsData const* data = fast_cast<CrossSectionsData>(_data))
		{
			if (data->createSpaceBlocker.create)
			{
				_context.store_space_blocker(create_space_blocker_from(ipu, _context, _instance, data->createSpaceBlocker));
			}
		}
	}

	//

	if (fast_cast<CrossSectionsData>(_data) && fast_cast<CrossSectionsData>(_data)->noMesh)
	{
		ipu.keep_polygons(0);
	}

	//

	if (vertexCount > 0)
	{
		// create part
		Meshes::Mesh3DPart* part = _context.get_generated_mesh()->load_part_data(vertexData.get_data(), elementsData.get_data(), Meshes::Primitive::Triangle, vertexCount, elementsCount, _context.get_vertex_format(), _context.get_index_format());

#ifdef AN_DEVELOPMENT
		part->debugInfo = String::printf(TXT("cross section s @ %S"), _instance.get_element()->get_location_info().to_char());
#endif

		_context.store_part(part, _instance);

		if (CrossSectionsData const * data = fast_cast<CrossSectionsData>(_data))
		{
			if (data->ignoreForCollision)
			{
				_context.ignore_part_for_collision(part);
			}
		}
	}
	else
	{
		error_generating_mesh(_instance, TXT("no cross section created"));
	}

	return true;
}

//

REGISTER_FOR_FAST_CAST(CrossSectionsData);

bool CrossSectionsData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

#ifdef AN_DEVELOPMENT
	debugDrawPoints = _node->get_bool_attribute_or_from_child_presence(TXT("debugDrawPoints"));

	debugDrawPointsTextScale = _node->get_float_attribute_or_from_child(TXT("debugDrawPointsTextScale"), debugDrawPointsTextScale);
	if (auto* n = _node->first_child_named(TXT("debugDrawPoints")))
	{
		debugDrawPointsTextScale = n->get_float_attribute(TXT("textScale"), debugDrawPointsTextScale);
	}

#endif

	createMovementCollisionMesh.load_from_xml(_node, TXT("createMovementCollisionMesh"), _lc);
	createMovementCollisionBox.load_from_xml(_node, TXT("createMovementCollisionBox"), _lc);
	createPreciseCollisionMesh.load_from_xml(_node, TXT("createPreciseCollisionMesh"), _lc);
	createPreciseCollisionBox.load_from_xml(_node, TXT("createPreciseCollisionBox"), _lc);
	error_loading_xml_on_assert(!_node->first_child_named(TXT("createPreciseCollisionMeshSkinned")), result, _node, TXT("createPreciseCollisionMeshSkinned not handled"));
	createSpaceBlocker.load_from_xml(_node, TXT("createSpaceBlocker"));
	noMesh = _node->get_bool_attribute_or_from_child_presence(TXT("noMesh"), noMesh);
	ignoreForCollision = _node->get_bool_attribute_or_from_child_presence(TXT("ignoreForCollision"), ignoreForCollision);

	reverseSurfaces = _node->get_bool_attribute(TXT("reverseSurfaces"), reverseSurfaces);

	for_every(cssNode, _node->children_named(TXT("crossSections")))
	{
		for_every(csNode, cssNode->children_named(TXT("crossSection")))
		{
			crossSections.push_back(CrossSection());
			CrossSection & cs = crossSections.get_last();
			cs.id = csNode->get_name_attribute_or_from_child(TXT("id"), cs.id);
			cs.placement.load_from_xml(csNode, TXT("placement"));
			cs.u.load_from_xml(csNode, TXT("u"));
			cs.skinTo.load_from_xml(csNode, TXT("skinTo"));
			MeshGenParam<float> u;
			for_every(pNode, csNode->children_named(TXT("point")))
			{
				cs.points.push_back(CrossSectionPoint());
				CrossSectionPoint & csp = cs.points.get_last();
				csp.id = pNode->get_name_attribute_or_from_child(TXT("id"), csp.id);
				csp.point.load_from_xml(pNode, TXT("x"), TXT("y"), TXT("z"), TXT("axis"), true);
				csp.noPolygon.load_from_xml(pNode, TXT("noPolygon"));
				csp.noPrevPolygon.load_from_xml(pNode, TXT("noPrevPolygon"));
				csp.noNextPolygon.load_from_xml(pNode, TXT("noNextPolygon"));
				csp.u = u;
				csp.uPrev = u;
				csp.uNext = u;
				csp.u.load_from_xml(pNode, TXT("u"));
				csp.uPrev.load_from_xml(pNode, TXT("uPrev"));
				csp.uNext.load_from_xml(pNode, TXT("uNext"));
				u = csp.u;
				csp.skinTo.load_from_xml(pNode, TXT("skinTo"));
				for_every(n, pNode->children_named(TXT("wheresMyPoint")))
				{
					result &= csp.wmpToolSet.load_from_xml(n);
				}
			}
		}
	}


	return result;
}

bool CrossSectionsData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}