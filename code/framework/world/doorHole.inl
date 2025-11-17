Vector3 DoorHole::to_inner_scale(DoorSide::Type _side, Vector2 const &  _scale, Vector3 const & _point) const
{
	return (_point) * _scale.inverted().to_vector3_xz(1.0f);
}

Vector3 DoorHole::to_outer_scale(DoorSide::Type _side, Vector2 const & _scale, Vector3 const & _point) const
{
	return (_point) * _scale.to_vector3_xz(1.0f);
}

bool DoorHole::is_point_inside(DoorSide::Type _side, Vector2 const & _scale, Vector3 const & _point, float _ext) const
{
	Vector3 point = to_inner_scale(_side, _scale, _point);
	an_assert(planesValid, TXT("call build() after adding points!"));
	an_assert(is_convex(), TXT("door hole is not convex!"));
	for_every_const(edge, type == DoorHoleType::Symmetrical || _side == DoorSide::A? aEdges : bEdges)
	{
#ifdef AN_ASSERT
#ifndef AN_CLANG
		float inFront = edge->plane.get_in_front(point);
#endif
#endif
		if (edge->plane.get_in_front(point) < -_ext) 
		{
			return false;
		}
	}
	return true;
}

bool DoorHole::is_segment_inside(DoorSide::Type _side, Vector2 const & _scale, SegmentSimple const & _segment, float _ext) const
{
	Segment segment = Segment(to_inner_scale(_side, _scale, _segment.get_start()),
							  to_inner_scale(_side, _scale, _segment.get_end()));
	an_assert(planesValid, TXT("call build() after adding points!"));
	an_assert(is_convex(), TXT("door hole is not convex!"));
	auto const & edges = type == DoorHoleType::Symmetrical || _side == DoorSide::A ? aEdges : bEdges;

	ARRAY_STACK(float, startInFront, edges.get_size());
	ARRAY_STACK(float, endInFront, edges.get_size());

	// check trivial cases if both are outside, or are inside all
	int allInsideCount = 0;
	for_every_const(edge, edges)
	{
		float sif = edge->plane.get_in_front(segment.get_start()) + _ext;
		float eif = edge->plane.get_in_front(segment.get_end()) + _ext;
		if (sif < 0.0f &&
			eif < 0.0f)
		{
			return false;
		}
		if (sif >= 0.0f && eif >= 0.0f) ++allInsideCount;
		startInFront.push_back(sif);
		endInFront.push_back(eif);
	}

	if (allInsideCount == edges.get_size())
	{
		return true;
	}

	// check if we can find segment intersecting with edge and if that point lies inside - if so, return true
	float const * sif = startInFront.get_data();
	float const * eif = endInFront.get_data();
	for_every(onEdge, edges)
	{
		if (*sif * *eif <= 0.0f &&
			(*sif != 0.0f || *eif != 0.0f))
		{
			float t = -*sif / (*eif - *sif); // (0 - s) / (e - s) to find point on segment
			an_assert(t >= 0.0f && t <= 1.0f);
			Vector3 point = segment.get_at_t(t);
			bool allInside = true;
			for_every_const(edge, edges)
			{
				if (edge->plane.get_in_front(point) < 0.0f)
				{
					allInside = false;
					break;
				}
			}
			if (allInside)
			{
				return true;
			}
		}
		++sif;
		++eif;
	}

	return false;
}

#define for_every_edge_and_prev(edge, prev, edges) \
	if (auto edge = edges.begin()) \
	if (auto end##edge = edges.end()) \
	if (auto prev = end##edge) \
	for (edges.is_empty()? (prev = edges.begin()) : -- prev; edge != end##edge; prev = edge, ++ edge)

Vector3 DoorHole::move_point_inside(DoorSide::Type _side, Vector2 const & _scale, Vector3 const & _point, float _moveInside) const
{
	an_assert(planesValid, TXT("call build() after adding points!"));
	an_assert(is_convex(), TXT("door hole is not convex!"));
	// find if it is outside for point
	Vector3 const point = to_inner_scale(_side, _scale, _point);
	Vector3 const offsetFromDoorHolePlane = Vector3::dot(point, Vector3::yAxis) * Vector3::yAxis;
	Array<Edge> const & edges = type == DoorHoleType::Symmetrical || _side == DoorSide::A ? aEdges : bEdges;
	for_every_edge_and_prev(edge, prev, edges)
	{
		float const inFrontPrev = prev->plane.get_in_front(point);
		Vector3 const edgePoint2point = point - edge->point;
		float const toPrevDot = Vector3::dot(edgePoint2point, edge->toPrev);
		if (inFrontPrev < 0.0f)
		{
			float const inFront = edge->plane.get_in_front(point);
			if (inFront < 0.0f &&
				toPrevDot <= 0.001f &&
				Vector3::dot(edgePoint2point, edge->toNext) <= 0.001f)
			{
				// collapse to point
				Vector3 result = edge->point + offsetFromDoorHolePlane;
				return _moveInside > 0.0f? result.normal() * max(0.0f, result.length() - _moveInside) : result;
			}
			else if (toPrevDot >= 0.0f &&
				Vector3::dot(point - prev->point, prev->toNext) >= 0.0f)
			{
				// collapse to prev edge
				Vector3 result = to_outer_scale(_side, _scale, point - prev->plane.get_normal() * inFrontPrev + offsetFromDoorHolePlane);
				return _moveInside > 0.0f ? result.normal() * max(0.0f, result.length() - _moveInside) : result;
			}
		}
	}

	return _point;
}

Vector3 DoorHole::move_point_outside_to_edge(DoorSide::Type _side, Vector2 const & _scale, Vector3 const & _point) const
{
	an_assert(planesValid, TXT("call build() after adding points!"));
	an_assert(is_convex(), TXT("door hole is not convex!"));
	// find if it is outside for point
	Vector3 const point = to_inner_scale(_side, _scale, _point);
	Vector3 const offsetFromDoorHolePlane = Vector3::dot(point, Vector3::yAxis) * Vector3::yAxis;
	Array<Edge> const & edges = type == DoorHoleType::Symmetrical || _side == DoorSide::A ? aEdges : bEdges;
	for_every_edge_and_prev(edge, prev, edges)
	{
		float const inFrontPrev = prev->plane.get_in_front(point);
		Vector3 const edgePoint2point = point - edge->point;
		float const toPrevDot = Vector3::dot(edgePoint2point, edge->toPrev);
		if (inFrontPrev < 0.0f)
		{
			float const inFront = edge->plane.get_in_front(point);
			if (inFront < 0.0f &&
				toPrevDot <= 0.001f &&
				Vector3::dot(edgePoint2point, edge->toNext) <= 0.001f)
			{
				// collapse to point
				return edge->point + offsetFromDoorHolePlane;
			}
			else if (toPrevDot >= 0.0f &&
				Vector3::dot(point - prev->point, prev->toNext) >= 0.0f)
			{
				// collapse to prev edge
				return to_outer_scale(_side, _scale, point - prev->plane.get_normal() * inFrontPrev + offsetFromDoorHolePlane);
			}
		}
	}

	return _point;
}

#undef for_every_edge_and_prev

bool DoorHole::is_convex() const
{
#ifdef AN_ASSERT
	an_assert(planesValid, TXT("call build() after adding points!"));
#endif
	return is_convex(aEdges) && is_convex(bEdges);
}

bool DoorHole::is_convex(Array<Edge> const & _edges) const
{
	for_every_const(edge, _edges)
	{
		bool isInside = true;
		for_every_const(againstEdge, _edges)
		{
			if (againstEdge->plane.get_in_front(edge->point) < -0.001f) 
			{
#ifdef AN_ASSERT
#ifndef AN_CLANG
				float inFront = againstEdge->plane.get_in_front(edge->point);
#endif
#endif
				isInside = false;
				break;
			}
		}
		if (! isInside)
		{
			return false;
		}
	}
	return true;
}

bool DoorHole::is_symmetrical() const
{
	an_assert(planesValid, TXT("call build() after adding points!"));
	return is_symmetrical(aEdges) && is_symmetrical(bEdges);
}

bool DoorHole::is_symmetrical(Array<Edge> const & _edges) const
{
	for_every_const(edge, _edges)
	{
		bool hasSymmetrical = false;
		for_every_const(againstEdge, _edges)
		{
			if (edge->point.x == -againstEdge->point.x)
			{
				hasSymmetrical = true;
				break;
			}
		}
		if (! hasSymmetrical)
		{
			return false;
		}
	}
	return true;
}

bool DoorHole::is_behind(DoorSide::Type _side, Plane const & _plane) const
{
	Array<Edge> const & edges = type == DoorHoleType::Symmetrical || _side == DoorSide::A ? aEdges : bEdges;
	for_every(edge, edges)
	{
		if (_plane.get_in_front(edge->point) > 0.0f)
		{
			return false;
		}
	}
	return true;
}

template <typename Class>
void DoorHole::setup_frustum_view(DoorSide::Type _side, Vector2 const & _scale, ::System::ViewFrustum & _frustum, Class const & _modelViewMatrix, Class const & _doorInRoomOutboundMatrix) const
{
	Class const doorModelViewMatrix = _modelViewMatrix.to_world(_doorInRoomOutboundMatrix);

	// first, clear it
	_frustum.clear();

	// go in normal order as we work with outbound matrix and points are stored in outbound order
	for_every_const(edge, type == DoorHoleType::Symmetrical || _side == DoorSide::A ? aEdges : bEdges)
	{
		_frustum.add_point(doorModelViewMatrix.location_to_world(to_outer_scale(_side, _scale, edge->point)));
	}

	_frustum.build();
}

template <typename Class>
void DoorHole::setup_frustum_placement(DoorSide::Type _side, Vector2 const & _scale, ::System::ViewFrustum & _frustum, Class const & _placementMatrix, Class const & _doorInRoomOutboundMatrix) const
{
	Class const doorInPlacementMatrix = _placementMatrix.to_local(_doorInRoomOutboundMatrix);

	// first, clear it
	_frustum.clear();

	// go in normal order as we work with outbound matrix and points are stored in outbound order
	for_every_const(edge, type == DoorHoleType::Symmetrical || _side == DoorSide::A ? aEdges : bEdges)
	{
		_frustum.add_point(doorInPlacementMatrix.location_to_world(to_outer_scale(_side, _scale, edge->point)));
	}

	_frustum.build();
}
