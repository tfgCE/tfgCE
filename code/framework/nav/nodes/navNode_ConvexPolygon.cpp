#include "navNode_ConvexPolygon.h"

#include "..\navMesh.h"
#include "..\navSystem.h"
#include "..\..\game\game.h"

#include "..\..\..\core\debug\debugRenderer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace Nav;
using namespace Nodes;

//

REGISTER_FOR_FAST_CAST(Nodes::ConvexPolygon);

void Nodes::ConvexPolygon::connect_as_open_node()
{
	an_assert(Game::get()->get_nav_system()->is_in_writing_mode());

	an_assert(openMoveToFind.is_zero(), TXT("implement convex polygon looking for something else"));

	//

	Mesh* mesh = belongsTo;
	if (!mesh)
	{
		return;
	}
	Transform currentPlacement = get_current_placement();
	for_every_ref(node, mesh->access_nodes())
	{
		if (auto* cp = fast_cast<ConvexPolygon>(node))
		{
			if (cp->is_open_node() &&
				! is_connected_to(node))
			{
				Transform otherCurrentPlacement = cp->get_current_placement();

				// [t]his
				// [o]ther

				bool connected = false;
				for_every(tEdge, edges)
				{
					Vector3 ta = currentPlacement.location_to_world(tEdge->start);
					Vector3 tb = currentPlacement.location_to_world(tEdge->end);
					for_every(oEdge, cp->edges)
					{
						Vector3 oa = otherCurrentPlacement.location_to_world(oEdge->start);
						Vector3 ob = otherCurrentPlacement.location_to_world(oEdge->end);
						if (Vector3::are_almost_equal(ta, ob) &&
							Vector3::are_almost_equal(tb, oa)) // they have to face each other!
						{
							auto* shared = connect(node);
							if (auto* tc = access_connect_by_shared_data(shared))
							{
								tc->set_edge_idx(for_everys_index(tEdge));
							}
							if (auto* oc = node->access_connect_by_shared_data(shared))
							{
								oc->set_edge_idx(for_everys_index(oEdge));
							}
							connected = true;
						}
						if (connected) break;
					}
					if (connected) break;
				}
			}
		}
	}
}

void Nodes::ConvexPolygon::add_clockwise(Vector3 const & _point)
{
	an_assert(Game::get()->get_nav_system()->is_in_writing_mode());

	points.push_back(_point);
	edges.clear();
}

void Nodes::ConvexPolygon::build(float _in)
{
	an_assert(Game::get()->get_nav_system()->is_in_writing_mode());

	edges.clear();
	if (points.is_empty())
	{
		return;
	}

	edges.make_space_for(points.get_size());

	// remove doubled
	for (int i = 0; i < points.get_size(); ++i)
	{
		int j = mod(i - 1, points.get_size());
		if (points[i] == points[j] ||
			Vector3::are_almost_equal(points[i], points[j]))
		{
			points.remove_at(i);
			--i;
		}
	}

	if (points.get_size() < 3)
	{
		warn(TXT("[nav] polygon degenerated below triangle"));
		return;
	}

	// recentre placement, points should be stored locally, we do all calculations locally
	Vector3 centre = Vector3::zero;
	float pointsWeight = 0.0f;
	for_every(point, points)
	{
		centre += *point;
		pointsWeight += 1.0f;
	}
	centre = centre / pointsWeight;

	// z axis - up
	Vector3 zAxis;
	{
		Vector3 zAxisAccumulated = Vector3::zero;
		Vector3 lastPoint = points.get_last();
		for_every(point, points)
		{
			zAxisAccumulated += Vector3::cross(*point - centre, lastPoint - centre);
			pointsWeight += 1.0f;
			lastPoint = *point;
		}

		if (points.get_size() == 2)
		{
			// just assume it is up
			zAxis = Vector3::zAxis;
		}
		else
		{
			an_assert(!zAxisAccumulated.is_zero());
			zAxis = zAxisAccumulated.normal();
			if (zAxisAccumulated.is_zero())
			{
				zAxis = Vector3::zAxis;
			}
		}
	}
	// y axis - forward - any point will do
	Vector3 yAxis = (points[0] - centre).normal();

	// set placement and move points to be local to new placement
	Transform newLocalPlacement = Transform::identity;
	newLocalPlacement.set_orientation(look_matrix33(yAxis, zAxis).to_quat());
	newLocalPlacement.set_translation(centre);
	placement = placement.to_world(newLocalPlacement);
	for_every(point, points)
	{
		*point = newLocalPlacement.location_to_local(*point);
	}

	{	// build edges (local points are aligned with z axis)
		Vector3 lastPoint = points.get_last();
		for_every(point, points)
		{
			Edge edge;
			edge.start = lastPoint;
			edge.end = *point;
			edge.dir = (edge.end - edge.start).normal();
			edge.in = Plane(Vector3::cross(Vector3::zAxis, -edge.dir).normal(), edge.start);
			an_assert(edge.in.get_normal().is_normalised());
			edges.push_back(edge);
			lastPoint = *point;
		}
	}

	if (_in != 0.0f)
	{
		// move points in

		// first make edges be inside
		for_every(edge, edges)
		{
			edge->in = edge->in.get_adjusted_along_normal(_in);
		}

		// now move points inside edges
		for_every(point, points)
		{
			int triesLeft = 100;
			while (triesLeft --)
			{
				bool ok = true;
				for_every(edge, edges)
				{
					float const threshold = -0.01f;
					float inFront = edge->in.get_in_front(*point);
					if (inFront < threshold)
					{
						*point += edge->in.get_normal() * (-inFront) * 0.05f; // this is to allow situations in which edges have collapsed
						ok = false;
					}
				}
				if (ok)
				{
					break;
				}
			}
		}

		// remove doubled points
		Vector3 lastPoint = points.get_last();
		for (int i = 0; i < points.get_size(); ++i)
		{
			if ((points[i] - lastPoint).is_almost_zero())
			{
				points.remove_at(i);
				--i;
				continue;
			}
			lastPoint = points[i];
		}

		// rebuild, we won't get back here as _in is 0.0
		build(0.0f);
	}
}

bool Nodes::ConvexPolygon::is_inside(Vector3 const & _point) const
{
	Transform currentPlacement = get_current_placement();
	Vector3 localPoint = currentPlacement.location_to_local(_point);
	float threshold = 0.00001f;
	for_every(edge, edges)
	{
		if (edge->in.get_in_front(localPoint) < -threshold)
		{
			return false;
		}
	}
	return true;
}

void Nodes::ConvexPolygon::debug_draw()
{
#ifdef AN_DEBUG_RENDERER
	base::debug_draw();

	if (is_outdated())
	{
		return;
	}

	Transform currentPlacement = get_current_placement();

	debug_push_transform(currentPlacement);
	for_every(edge, edges)
	{
		Colour c = Colour::lerp(0.6f, Colour::blue, Colour::green);
		debug_draw_triangle(true, c.with_alpha(0.05f), Vector3::zero, edge->start, edge->end);
		debug_draw_arrow(true, c, edge->start, edge->end);
		Vector3 centre = (edge->start + edge->end) * 0.5f;
		debug_draw_arrow(true, c, centre, centre + edge->in.get_normal() * 0.04f);
	}
	debug_pop_transform();
#endif
}

Optional<Segment> Nodes::ConvexPolygon::calculate_segment_for_edge(int _edgeIdx, float _inRadius) const
{
	if (!edges.is_index_valid(_edgeIdx))
	{
		return NP;
	}

	Transform currentPlacement = get_current_placement();

	// it's a naive approach but might be enough
	auto& edge = edges[_edgeIdx];
	Vector3 s = edge.start;
	Vector3 e = edge.end;
	Vector3 s2e = (e - s).normal();
	float length = (e - s).length();
	float halfLength = length * 0.5f;
	float in = min(_inRadius, halfLength);
	Vector3 moveIn = s2e * in;
	s += moveIn;
	e -= moveIn;

	return Segment(currentPlacement.location_to_world(s), currentPlacement.location_to_world(e));
}

bool Nodes::ConvexPolygon::does_intersect(Segment const & _segment, OUT_ Vector3 & _intersectionPoint) const
{
	Transform currentPlacement = get_current_placement();

	bool intersectionFound = false;
	Vector3 intersectionPoint = Vector3::zero; // to have anything
	float bestIntersectionPointT = 2.0f;
	Segment segment = currentPlacement.to_local(_segment);

	for_every(edge, edges)
	{
		float t = edge->in.calc_intersection_t(segment);
		if (t <= 1.0f)
		{
			Vector3 candidate = segment.get_at_t(t);
			bool ok = true;
			for_every(edge, edges)
			{
				float const threshold = -0.01f; // allow to be on the line or even outside a little bit
				if (edge->in.get_in_front(candidate) < threshold)
				{
					ok = false;
					break;
				}
			}
			if (ok)
			{
				if (t < bestIntersectionPointT)
				{
					bestIntersectionPointT = t;
					intersectionFound = true;
					intersectionPoint = candidate;
				}
			}
		}
	}
	_intersectionPoint = currentPlacement.location_to_world(intersectionPoint);
	return intersectionFound;
}

bool Nodes::ConvexPolygon::find_location(Vector3 const & _at, OUT_ Vector3 & _found, REF_ float & _dist) const
{
	if (is_outdated())
	{
		return false;
	}
	bool result = false;
	Transform placement = get_current_placement();
	Plane plane(placement.get_axis(Axis::Z), placement.get_translation());

	Vector3 droppedAt = plane.get_dropped(_at);
	float dropDist = (droppedAt - _at).length();

	Vector3 localDA = placement.location_to_local(droppedAt);
	an_assert(abs(localDA.z) < 0.01f);
	localDA.z = 0.0f;
	Vector3 curLDA = localDA;
	// move curLDA inside
	int triesLeft = 10;
	while (--triesLeft)
	{
		float threshold = 0.00001f;
		bool allOk = true;
		for_every(edge, edges)
		{
			float inFront = edge->in.get_in_front(curLDA);

			if (inFront < -threshold)
			{
				allOk = false;

				// move inside
				curLDA -= edge->in.get_normal() * inFront;
				curLDA.z = 0.0;
			}
		}
	}

	float dist = sqrt(sqr((curLDA - localDA).length()) + sqr(dropDist));
	if (_dist > dist)
	{
		_dist = dist;
		_found = placement.location_to_world(curLDA);
		result = true;
	}

	return result;
}

void Nodes::ConvexPolygon::reset_to_new()
{
	base::reset_to_new();

	points.clear();
	edges.clear();
}

float Nodes::ConvexPolygon::calculate_greater_radius() const
{
	Range2 range = Range2::empty;
	for_every(edge, edges)
	{
		range.include(edge->start.to_vector2());
	}
	return sqrt(sqr(range.x.length()) + sqr(range.y.length())) * 0.5f;
}