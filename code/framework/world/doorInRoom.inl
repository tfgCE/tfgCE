float DoorInRoom::calculate_dist_of(Vector3 const & _point, float _inFront, OUT_ Vector3 & _onDoorPlaneInsideDoor, float _offHoleDistMultiplier) const
{
	// get location on plane
	Vector3 const onDoorPlaneRS = _point - plane.get_normal() * _inFront;
	Vector3 const onDoorPlaneLS = outboundMatrix.location_to_local(onDoorPlaneRS);

	// get location inside hole
	Vector3 const insideDoorHoleLS = door->get_hole()->move_point_inside(get_side(), get_hole_scale(), onDoorPlaneLS);

	// calculate
	_onDoorPlaneInsideDoor = outboundMatrix.location_to_world(insideDoorHoleLS);

	if (_offHoleDistMultiplier != 1.0f)
	{
		// compose distance using perpendicular distances, one in door plane, other as distance from door plane
		return sqrt(sqr((_onDoorPlaneInsideDoor - onDoorPlaneRS).length() * _offHoleDistMultiplier) + sqr(_inFront));
	}
	else
	{
		return (_onDoorPlaneInsideDoor - _point).length();
	}
}

float DoorInRoom::calculate_dist_of(Vector3 const & _point, float _inFront, float _offHoleDistMultiplier) const
{
	// get location on plane
	Vector3 const onDoorPlaneRS = _point - plane.get_normal() * _inFront;
	Vector3 const onDoorPlaneLS = outboundMatrix.location_to_local(onDoorPlaneRS);

	// get location inside hole
	Vector3 const insideDoorHoleLS = door->get_hole()->move_point_inside(get_side(), get_hole_scale(), onDoorPlaneLS);

	// calculate
	Vector3 onDoorPlaneInsideDoorRS = outboundMatrix.location_to_world(insideDoorHoleLS);

#ifdef INVESTIGATE__CHECKS_FOR_DOOR_IN_ROOM
	debug_draw_arrow(true, Colour::white, _point, onDoorPlaneRS);
	debug_draw_arrow(true, Colour::white, onDoorPlaneRS, onDoorPlaneInsideDoorRS);
#endif

	if (_offHoleDistMultiplier != 1.0f)
	{
		// compose distance using perpendicular distances, one in door plane, other as distance from door plane
		return sqrt(sqr((onDoorPlaneInsideDoorRS - onDoorPlaneRS).length() * _offHoleDistMultiplier) + sqr(_inFront));
	}
	else
	{
		return (onDoorPlaneInsideDoorRS - _point).length();
	}
}

float DoorInRoom::calculate_dist_of(Vector3 const & _point, float _offHoleDistMultiplier) const
{
	float inFront = plane.get_in_front(_point);

	// get location on plane
	Vector3 const onDoorPlaneRS = _point - plane.get_normal() * inFront;
	Vector3 const onDoorPlaneLS = outboundMatrix.location_to_local(onDoorPlaneRS);

	// get location inside hole
	Vector3 const insideDoorHoleLS = door->get_hole()->move_point_inside(get_side(), get_hole_scale(), onDoorPlaneLS);

	// calculate
	Vector3 onDoorPlaneInsideDoor = outboundMatrix.location_to_world(insideDoorHoleLS);

	// compose distance using perpendicular distances, one in door plane, other as distance from door plane
	if (_offHoleDistMultiplier != 1.0f)
	{
		// compose distance using perpendicular distances, one in door plane, other as distance from door plane
		return sqrt(sqr((onDoorPlaneInsideDoor - onDoorPlaneRS).length() * _offHoleDistMultiplier) + sqr(inFront));
	}
	else
	{
		return (onDoorPlaneInsideDoor - _point).length();
	}
}

bool DoorInRoom::is_behind(Plane const & _plane) const
{
	if (DoorHole const * hole = get_hole())
	{
		return hole->is_behind(side, _plane);
	}
	else
	{
		return false;
	}
}
