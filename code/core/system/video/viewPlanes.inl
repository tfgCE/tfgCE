bool ViewPlanes::is_inside(Vector3 const & _point) const
{
	for_every_const(plane, planes)
	{
		if (plane->get_in_front(_point) < 0.0f)
		{
			return false;
		}
	}
	return true;
}

bool ViewPlanes::is_visible(Vector3 const & _point, float _radius, Vector3 const & _centreDistance) const
{
	if (_centreDistance.is_zero())
	{
		for_every_const(plane, planes)
		{
			if (plane->get_in_front(_point) < -_radius)
			{
				return false;
			}
		}
		return true;
	}
	else
	{
		Vector3 const halfCentreDistance = _centreDistance * 0.5f;
		Vector3 const pointA = _point - halfCentreDistance;
		Vector3 const pointB = _point + halfCentreDistance;
		for_every_const(plane, planes)
		{
			if (plane->get_in_front(pointA) < -_radius &&
				plane->get_in_front(pointB) < -_radius)
			{
				return false;
			}
		}
		return true;
	}
}

bool ViewPlanes::is_visible(Matrix44 const & _placement, Range3 const & _boundingBox) const
{
	Vector3 const location = _placement.get_translation();
	Vector3 const xAxis = _placement.get_x_axis();
	Vector3 const yAxis = _placement.get_y_axis();
	Vector3 const zAxis = _placement.get_z_axis();
	ArrayStatic<Vector3,8> points; SET_EXTRA_DEBUG_INFO(points, TXT("ViewPlanes::is_visible.points"));
	points.set_size(8);
	{
		Vector3 xBase = location + xAxis * _boundingBox.x.min;
		{
			Vector3 yBase = xBase + yAxis * _boundingBox.y.min;
			points[0] = yBase + zAxis * _boundingBox.z.min;
			points[1] = yBase + zAxis * _boundingBox.z.max;
		}
		{
			Vector3 yBase = xBase + yAxis * _boundingBox.y.max;
			points[2] = yBase + zAxis * _boundingBox.z.min;
			points[3] = yBase + zAxis * _boundingBox.z.max;
		}
	}
	{
		Vector3 xBase = location + xAxis * _boundingBox.x.max;
		{
			Vector3 yBase = xBase + yAxis * _boundingBox.y.min;
			points[4] = yBase + zAxis * _boundingBox.z.min;
			points[5] = yBase + zAxis * _boundingBox.z.max;
		}
		{
			Vector3 yBase = xBase + yAxis * _boundingBox.y.max;
			points[6] = yBase + zAxis * _boundingBox.z.min;
			points[7] = yBase + zAxis * _boundingBox.z.max;
		}
	}
	for_every_const(plane, planes)
	{
		int outside = 0;
		for_every(point, points)
		{
			if (plane->get_in_front(*point) < -0.0f)
			{
				++outside;
			}
			else
			{
				break;
			}
		}
		if (outside == 8)
		{
			return false;
		}
	}
	return true;
}
