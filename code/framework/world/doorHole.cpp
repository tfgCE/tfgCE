#include "doorHole.h"

#include "door.h"

#include "..\..\core\concurrency\asynchronousJob.h"
#include "..\..\core\mesh\mesh3dBuilder.h"
#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\io\xml.h"
#include "..\library\library.h"
#include "..\game\game.h"
#include "..\jobSystem\jobSystem.h"

using namespace Framework;

#define for_every_edge_and_prev(edge, prev, edges) \
	if (auto edge = edges.begin()) \
	if (auto end##edge = edges.end()) \
	if (auto prev = end##edge) \
	for (edges.is_empty()? (prev = edges.begin()) : -- prev; edge != end##edge; prev = edge, ++ edge)

DoorHoleType::Type DoorHoleType::parse(String const & _string)
{
	if (_string == TXT("twoSided") ||
		_string == TXT("asymmetrical"))
	{
		return DoorHoleType::TwoSided;
	}
	if (_string == TXT("symmetrical"))
	{
		return DoorHoleType::Symmetrical;
	}
	error(TXT("DoorHoleType \"%S\" not recognised"), _string.to_char());
	return DoorHoleType::Symmetrical;
}

DoorHole::DoorHole()
:  type( DoorHoleType::Symmetrical )
#ifdef AN_ASSERT
, planesValid( false )
#endif
, outboundAMesh(nullptr)
, outboundBMesh(nullptr)
{
}

struct DoorHole_DeleteMeshes
: public Concurrency::AsynchronousJobData
{
	Meshes::Mesh3D* meshA;
	Meshes::Mesh3D* meshB;
	DoorHole_DeleteMeshes(Meshes::Mesh3D* _meshA, Meshes::Mesh3D* _meshB)
	: meshA(_meshA)
	, meshB(_meshB)
	{}

	static void perform(Concurrency::JobExecutionContext const * _executionContext, void* _data)
	{
		DoorHole_DeleteMeshes* data = (DoorHole_DeleteMeshes*)_data;
		delete data->meshA;
		delete data->meshB;
	}
};

DoorHole::~DoorHole()
{
	if (outboundAMesh || outboundBMesh)
	{
		Game::async_system_job(Game::get(), DoorHole_DeleteMeshes::perform, new DoorHole_DeleteMeshes(outboundAMesh, outboundBMesh));
	}
}

void DoorHole::add_point(Vector2 const & LOCATION_ _point, Vector2 const& _customData)
{
	aEdges.push_back(Edge(Vector3(_point.x, 0.0f, _point.y)));
	aEdges.get_last().customData = _customData;
	bEdges.insert_at(0, Edge(Vector3(-_point.x, 0.0f, _point.y)));
	aEdges.get_first().customData = _customData;
#ifdef AN_ASSERT
	planesValid = false;
#endif
}

void DoorHole::make_symmetrical_if_needed()
{
	if (type == DoorHoleType::Symmetrical)
	{
		// remove first all that are on left side
		for (int32 idx = aEdges.get_size() - 1; idx >= 0; -- idx)
		{
			Edge& edge = aEdges[idx];
			if (edge.point.x < 0.0f)
			{
				aEdges.remove_at(idx);
			}
		}

		// add symmetrical - start from end
		for (int32 idx = aEdges.get_size() - 1; idx >= 0; -- idx)
		{
			Edge& edge = aEdges[idx];
			if (edge.point.x != 0.0f)
			{
				aEdges.push_back(Edge(edge.point * Vector3(-1.0f, 1.0f, 1.0f)));
				aEdges.get_last().customData = edge.customData;
			}
		}

		// bEdges from aEdges
		bEdges.clear();
		for (int32 idx = aEdges.get_size() - 1; idx >= 0; --idx)
		{
			Edge& edge = aEdges[idx];
			bEdges.push_back(Edge(edge.point * Vector3(-1.0f, 1.0f, 1.0f)));
			bEdges.get_last().customData = edge.customData;
		}
	}

#ifdef AN_ASSERT
	planesValid = false;
#endif
}

void DoorHole::build()
{
	build_side(aEdges, *outboundAMesh);
	build_side(bEdges, *outboundBMesh);

	aCentre = Vector3::zero;
	if (!aEdges.is_empty())
	{
		for_every(edge, aEdges)
		{
			aCentre += edge->point;
		}
		aCentre *= 1.0f / (float)aEdges.get_size();
	}
	bCentre = aCentre * Vector3(-1.0f, 1.0f, 1.0f); // mirrored

#ifdef AN_ASSERT
	planesValid = true;
#endif
}

void DoorHole::build_side(Array<Edge> & _edges, Meshes::Mesh3D & _outboundMesh)
{
	for_every_edge_and_prev(edge, prev, _edges)
	{
		// clockwise
		prev->plane = Plane(prev->point, edge->point, edge->point + Vector3(0.0f, 1.0f, 0.0f));
		prev->toNext = (edge->point - prev->point).normal();
		edge->toPrev = -prev->toNext;
	}

	Meshes::Mesh3DBuilder builder;
	builder.setup(Meshes::Primitive::TriangleFan, ::System::vertex_format().with_texture_uv(), Meshes::Primitive::vertex_count_to_primitive_count(Meshes::Primitive::TriangleFan, _edges.get_usize()));
	for_every(edge, _edges)
	{
		builder.add_point();
		builder.location(edge->point);
		builder.uv(edge->customData);
	}
	builder.build();
	_outboundMesh.load_builder(&builder);
}

bool DoorHole::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("type")))
	{
		type = DoorHoleType::parse(attr->get_as_string());
	}

	nominalDoorWidth = _node->get_float_attribute(TXT("nominalDoorWidth"));
	nominalDoorHeight = _node->get_float_attribute(TXT("nominalDoorHeight"));
	for_every(pointNode, _node->children_named(TXT("point")))
	{
		Vector2 point;
		Vector2 customData = Vector2::zero;
		point.load_from_xml(pointNode);
		customData.load_from_xml(pointNode, TXT("u"), TXT("v"));
		add_point(point, customData);
	}

	make_symmetrical_if_needed();

	scale_to_nominal_door_size_and_build();

	return result;
}

void DoorHole::scale_to_nominal_door_size_and_build()
{
	Vector3 scale = Vector3::one;
	// use existing values as we're rescaling existing stuff, otherwise if we change it too often it will break
	if (Door::get_nominal_door_width().is_set() &&
		nominalDoorWidth != 0.0f)
	{
		scale.x = Door::get_nominal_door_width().get() / nominalDoorWidth;
		nominalDoorWidth = Door::get_nominal_door_width().get();
	}
	if (nominalDoorHeight != 0.0f)
	{
		if (heightFromWidth &&
			Door::get_nominal_door_width().is_set())
		{
			scale.z = Door::get_nominal_door_width().get() / nominalDoorHeight;
			nominalDoorHeight = Door::get_nominal_door_width().get();
		}
		else
		{
			scale.z = Door::get_nominal_door_height() / nominalDoorHeight;
			nominalDoorHeight = Door::get_nominal_door_height();
		}
	}
	for_every(edge, aEdges)
	{
		edge->point *= scale;
	}
	for_every(edge, bEdges)
	{
		edge->point *= scale;
	}

	if (outboundAMesh || outboundBMesh)
	{
		Game::async_system_job(Game::get(), DoorHole_DeleteMeshes::perform, new DoorHole_DeleteMeshes(outboundAMesh, outboundBMesh));
	}

	outboundAMesh = new Meshes::Mesh3D();
	outboundBMesh = new Meshes::Mesh3D();

#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
	outboundAMesh->set_debug_mesh_name(String(TXT("outbound door mesh")));
	outboundBMesh->set_debug_mesh_name(String(TXT("outbound door mesh")));
#endif

	build();
}

Range2 DoorHole::calculate_size(DoorSide::Type _side, Vector2 const & _scale) const
{
	Array<Edge> const & edges = type == DoorHoleType::Symmetrical || _side != DoorSide::B ? aEdges : bEdges;

	Vector3 edgePoint = to_outer_scale(_side, _scale, edges[0].point);
	Vector2 firstPoint = Vector2(edgePoint.x, edgePoint.z);
	Range2 size(Range(firstPoint.x, firstPoint.x), Range(firstPoint.y, firstPoint.y));
	// we may use a-side because we calculate size
	for_every(edge, edges)
	{
		Vector3 edgePoint = to_outer_scale(_side, _scale, edge->point);
		size.include(Vector2(edgePoint.x, edgePoint.z));
	}
	return size;
}

void DoorHole::debug_draw(DoorSide::Type _side, Vector2 const & _scale, bool _front, Colour const & _colour) const
{
	Array<Edge> const & edges = type == DoorHoleType::Symmetrical || _side != DoorSide::B ? aEdges : bEdges;
	if (edges.is_empty())
	{
		return;
	}
	Vector3 scale3(_scale.x, 1.0f, _scale.y);
	Vector3 point = edges.get_last().point;
	for_every(edge, edges)
	{
		debug_draw_line(_front, _colour, point * scale3, edge->point * scale3);
		point = edge->point;
	}
}

void DoorHole::create_edge_point_array(DoorSide::Type _side, OUT_ Array<Vector2> & _array, OUT_ Vector2 & _centre) const
{
	_array.clear();

	Array<Edge> const & edges = type == DoorHoleType::Symmetrical || _side != DoorSide::B ? aEdges : bEdges;
	for_every(edge, edges)
	{
		_array.push_back(Vector2(edge->point.x, edge->point.z));
	}

	Vector3 const centre = type == DoorHoleType::Symmetrical || _side != DoorSide::B ? aCentre : bCentre;
	_centre = Vector2(centre.x, centre.z);
}

bool DoorHole::is_in_front_of(DoorSide::Type _side, Vector2 const & _scale, Plane const & _plane, float _minDist) const
{
	for_every_const(edge, type == DoorHoleType::Symmetrical || _side != DoorSide::B ? aEdges : bEdges)
	{
		if (_plane.get_in_front(to_outer_scale(_side, _scale, edge->point)) > _minDist)
		{
			return true;
		}
	}

	return false;
}

Range3 DoorHole::calculate_flat_box(DoorSide::Type _side, Vector2 const& _scale, Matrix44 const& _at) const // x,z is as y=1, but y is kept as it was, makes sense only for y > 0
{
	Range3 box = Range3::empty;
	for_every_const(edge, type == DoorHoleType::Symmetrical || _side != DoorSide::B ? aEdges : bEdges)
	{
		Vector3 p = _at.location_to_world(to_outer_scale(_side, _scale, edge->point));
		if (p.y > 0.0f)
		{
			float scale = 1.0f / p.y;
			p.x *= scale;
			p.z *= scale;
		}
		box.include(p);
	}
	return box;
}

void DoorHole::provide_outbound_points_2d(DoorSide::Type _side, OUT_ Array<Vector2>& _outboundPoints) const
{
	auto& edges = _side == DoorSide::A ? aEdges : bEdges;

	_outboundPoints.make_space_for_additional(edges.get_size());
	for_every(e, edges)
	{
		_outboundPoints.push_back(Vector2(e->point.x, e->point.z));
	}
}

