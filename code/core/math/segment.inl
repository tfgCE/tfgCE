
void Segment::set_start_end(float _startT, float _endT)
{
	startT = _startT;
	endT = _endT;
	boundingBoxCalculated = false;
}

void Segment::set_end(float _endT)
{
	endT = _endT;
	boundingBoxCalculated = false;
}

bool Segment::collide_at(float _t)
{
	if (would_collide_at(_t))
	{
		endT = _t;
		boundingBoxCalculated = false;
		return true;
	}
	return false;
}

bool Segment::would_collide_at(float _t) const
{
	if (_t >= startT && _t <= endT)
	{
		return true;
	}
	return false;
}

Segment& Segment::operator = (Segment const & _segment)
{
	start = _segment.start;
	end = _segment.end;
	startT = _segment.startT;
	endT = _segment.endT;
	startToEnd = _segment.startToEnd;
	startToEndDir = _segment.startToEndDir;
	startToEndLength = _segment.startToEndLength;
	return *this;
}

void Segment::copy_t_from(Segment const & _segment)
{
	set_start_end(_segment.startT, _segment.endT);
	boundingBoxCalculated = false;
}

void Segment::update(Vector3 const & _start, Vector3 const & _end)
{
	start = _start;
	end = _end;
	startToEnd = end - start;
	startToEndLength = startToEnd.length();
	startToEndDir = startToEndLength != 0.0f ? startToEnd / startToEndLength : Vector3::zero;
	boundingBoxCalculated = false;
}

void Segment::update_bounding_box()
{
	if (boundingBoxCalculated) return;
	boundingBoxCalculated = true;
	Vector3 currentStart = get_at_start_t();
	Vector3 currentEnd = get_at_end_t();
	boundingBox = Range3(currentStart.x < currentEnd.x ? Range(currentStart.x, currentEnd.x) : Range(currentEnd.x, currentStart.x),
						 currentStart.y < currentEnd.y ? Range(currentStart.y, currentEnd.y) : Range(currentEnd.y, currentStart.y),
						 currentStart.z < currentEnd.z ? Range(currentStart.z, currentEnd.z) : Range(currentEnd.z, currentStart.z));
}

Vector3 Segment::get_at_t(float _t) const
{
	return start + startToEnd * _t;
}

Vector3 Segment::get_hit() const
{
	return get_at_t(endT);
}

float Segment::get_t(Vector3 const & _a) const
{
	Vector3 rel = _a - start;
	float along = Vector3::dot(rel, startToEndDir);

	return startToEndLength != 0.0f? clamp(along / startToEndLength, 0.0f, 1.0f) : 0.0f;
}

float Segment::get_t_not_clamped(Vector3 const & _a) const
{
	Vector3 rel = _a - start;
	float along = Vector3::dot(rel, startToEndDir);

	return startToEndLength != 0.0f? (along / startToEndLength) : 0.0f;
}

float Segment::get_distance(Vector3 const & _p) const
{
	float t = get_t(_p);
	return Vector3::distance(get_at_t(t), _p);
}

float Segment::get_distance(Vector3 const & _p, OUT_ float & _t) const
{
	_t = get_t(_p);
	return Vector3::distance(get_at_t(_t), _p);
}

void Segment::get_closest_points_ts(Segment const& _s, OUT_ float& _thisT, OUT_ float& _sT) const
{
	// based on: http://geomalgorithms.com/a07-_distance.html#dist3D_Segment_to_Segment
	Vector3 u = startToEnd;
	Vector3 v = _s.end - _s.start;
	Vector3 w = start - _s.start;
	float a = Vector3::dot(u, u);         // always >= 0
	float b = Vector3::dot(u, v);
	float c = Vector3::dot(v, v);         // always >= 0
	float d = Vector3::dot(u, w);
	float e = Vector3::dot(v, w);
	float D = a * c - b * b;        // always >= 0
	float sc, sN, sD = D;       // sc = sN / sD, default sD = D >= 0
	float tc, tN, tD = D;       // tc = tN / tD, default tD = D >= 0

	// compute the line parameters of the two closest points
	if (D < MARGIN)
	{
		// the lines are almost parallel
		sN = 0.0;         // force using point P0 on segment S1
		sD = 1.0;         // to prevent possible division by 0.0 later
		tN = e;
		tD = c;
	}
	else
	{
		// get the closest points on the infinite lines
		sN = (b * e - c * d);
		tN = (a * e - b * d);
		if (sN < 0.0)
		{
			// sc < 0 => the s=0 edge is visible
			sN = 0.0;
			tN = e;
			tD = c;
		}
		else if (sN > sD)
		{
			// sc > 1  => the s=1 edge is visible
			sN = sD;
			tN = e + b;
			tD = c;
		}
	}

	if (tN < 0.0)
	{
		// tc < 0 => the t=0 edge is visible
		tN = 0.0;
		// recompute sc for this edge
		if (-d < 0.0)
		{
			sN = 0.0;
		}
		else if (-d > a)
		{
			sN = sD;
		}
		else
		{
			sN = -d;
			sD = a;
		}
	}
	else if (tN > tD)
	{
		// tc > 1  => the t=1 edge is visible
		tN = tD;
		// recompute sc for this edge
		if ((-d + b) < 0.0)
		{
			sN = 0;
		}
		else if ((-d + b) > a)
		{
			sN = sD;
		}
		else
		{
			sN = (-d + b);
			sD = a;
		}
	}
	// finally do the division to get sc and tc
	sc = (abs(sN) < MARGIN ? 0.0f : sN / sD);
	tc = (abs(tN) < MARGIN ? 0.0f : tN / tD);

	sc = clamp(sc, 0.0f, 1.0f);
	tc = clamp(tc, 0.0f, 1.0f);

	_thisT = sc;
	_sT = tc;
}

float Segment::get_distance(Segment const & _s) const
{
	Vector3 u = startToEnd;
	Vector3 v = _s.end - _s.start;
	Vector3 w = start - _s.start;

	float sc;
	float tc;

	get_closest_points_ts(_s, sc, tc);

	// get the difference of the two closest points
	Vector3 dP = w + (sc * u) - (tc * v);  // =  S1(sc) - S2(tc)

	return dP.length();
}

float Segment::get_closest_t(Segment const & _s) const
{
	float sc;
	float tc;

	get_closest_points_ts(_s, sc, tc);

	return sc;
}
