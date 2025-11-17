String Range3::to_debug_string() const
{
	return String::printf(TXT("x:%S y:%S z:%S"), x.to_debug_string().to_char(), y.to_debug_string().to_char(), z.to_debug_string().to_char());
}

void Range3::include(Range3 const & _range)
{
	if (is_empty())
	{
		x = _range.x;
		y = _range.y;
		z = _range.z;
	}
	else
	{
		x.include(_range.x);
		y.include(_range.y);
		z.include(_range.z);
	}
}

void Range3::intersect_with(Range3 const & _range)
{
	if (! is_empty())
	{
		x.intersect_with(_range.x);
		y.intersect_with(_range.y);
		z.intersect_with(_range.z);
	}
}

/** have valid value or empty */
void Range3::include(Vector3 const & val)
{
	x.include(val.x);
	y.include(val.y);
	z.include(val.z);
}

void Range3::expand_by(Vector3 const & _by)
{
	x.expand_by(_by.x);
	y.expand_by(_by.y);
	z.expand_by(_by.z);
}

void Range3::scale_relative_to_centre(Vector3 const& _by)
{
	x.scale_relative_to_centre(_by.x);
	y.scale_relative_to_centre(_by.y);
	z.scale_relative_to_centre(_by.z);
}

/** needs at least one common point */
bool Range3::overlaps(Range3 const & _other) const
{
	return ! is_empty() && ! _other.is_empty() && x.overlaps(_other.x) && y.overlaps(_other.y) && z.overlaps(_other.z);
}

/** cannot just touch, need to go into another */
bool Range3::intersects(Range3 const & _other) const
{
	return ! is_empty() && ! _other.is_empty() && x.intersects(_other.x) && y.intersects(_other.y) && z.intersects(_other.z);
}

bool Range3::intersects_extended(Range3 const & _other, float _by) const
{
	return ! is_empty() && ! _other.is_empty() && x.intersects_extended(_other.x, _by) && y.intersects_extended(_other.y, _by) && z.intersects_extended(_other.z, _by);
}

/** needs _other to be completely inside this Range3 */
bool Range3::does_contain(Range3 const & _other) const
{
	return x.does_contain(_other.x) && y.does_contain(_other.y) && z.does_contain(_other.z);
}

bool Range3::does_contain(Vector3 const & _point) const
{
	return x.does_contain(_point.x) && y.does_contain(_point.y) && z.does_contain(_point.z);
}

void Range3::construct_from_location_and_axis_offsets(Vector3 const & _location, Vector3 const & _xm, Vector3 const & _xp, Vector3 const & _ym, Vector3 const & _yp, Vector3 const & _zm, Vector3 const & _zp)
{
	*this = empty;

	{
		Vector3 x_base = _location + _xm;
		{
			Vector3 y_base = x_base + _ym;
			include(y_base + _zm);
			include(y_base + _zp);
		}
		{
			Vector3 y_base = x_base + _yp;
			include(y_base + _zm);
			include(y_base + _zp);
		}
	}
	{
		Vector3 x_base = _location + _xp;
		{
			Vector3 y_base = x_base + _ym;
			include(y_base + _zm);
			include(y_base + _zp);
		}
		{
			Vector3 y_base = x_base + _yp;
			include(y_base + _zm);
			include(y_base + _zp);
		}
	}
}

void Range3::construct_from_placement_and_range3(Transform const & _placement, Range3 const & _range3)
{
	Vector3 const location = _placement.get_translation();
	Vector3 const xAxis = _placement.vector_to_world(Vector3::xAxis);
	Vector3 const yAxis = _placement.vector_to_world(Vector3::yAxis);
	Vector3 const zAxis = _placement.vector_to_world(Vector3::zAxis);
	construct_from_location_and_axis_offsets(location,
		xAxis * _range3.x.min, xAxis * _range3.x.max,
		yAxis * _range3.y.min, yAxis * _range3.y.max,
		zAxis * _range3.z.min, zAxis * _range3.z.max);
}

Range3 Range3::lerp(float _t, Range3 const& _a, Range3 const& _b)
{
	Range3 r;
	r.x = ::lerp(_t, _a.x, _b.x);
	r.y = ::lerp(_t, _a.y, _b.y);
	r.z = ::lerp(_t, _a.z, _b.z);
	return r;
}

void Range3::make_non_flat()
{
	if (x.length() == 0.0f) x.expand_by(0.001f);
	if (y.length() == 0.0f) y.expand_by(0.001f);
	if (z.length() == 0.0f) z.expand_by(0.001f);
}
