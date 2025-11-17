bool ViewFrustum::is_inside(Vector3 const & _point) const
{
#ifdef AN_ASSERT
	an_assert(normalsValid, TXT("call build() after adding points!"));
#endif
	for_every_const(edge, edges)
	{
		if (Vector3::dot(_point, edge->normal) < 0.0f)
		{
			return false;
		}
	}
	return true;
}

bool ViewFrustum::is_visible(Vector3 const & _point, float _radius, Vector3 const & _centreDistance) const
{
#ifdef AN_ASSERT
	an_assert(normalsValid, TXT("call build() after adding points!"));
#endif
	if (_centreDistance.is_zero())
	{
		for_every_const(edge, edges)
		{
			if (Vector3::dot(_point, edge->normal) < -_radius)
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
		for_every_const(edge, edges)
		{
			if (Vector3::dot(pointA, edge->normal) < -_radius &&
				Vector3::dot(pointB, edge->normal) < -_radius)
			{
				return false;
			}
		}
		return true;
	}
}

bool ViewFrustum::is_visible(Matrix44 const & _placement, Range3 const & _boundingBox) const
{
#ifdef AN_ASSERT
	an_assert(normalsValid, TXT("call build() after adding points!"));
#endif
	Vector3 const location = _placement.get_translation();
	Vector3 const xAxis = _placement.get_x_axis();
	Vector3 const yAxis = _placement.get_y_axis();
	Vector3 const zAxis = _placement.get_z_axis();
	ArrayStatic<Vector3,8> points; SET_EXTRA_DEBUG_INFO(points, TXT("ViewFrustum::is_visible.points"));
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
	for_every_const(edge, edges)
	{
		int outside = 0;
		for_every(point, points)
		{
			if (Vector3::dot(*point, edge->normal) < 0.0f)
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
