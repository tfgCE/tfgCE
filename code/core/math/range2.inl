String Range2::to_debug_string() const
{
	return String::printf(TXT("x:%S y:%S"), x.to_debug_string().to_char(), y.to_debug_string().to_char());
}

Range2 & Range2::include(Range2 const & _range)
{
	if (is_empty())
	{
		x = _range.x;
		y = _range.y;
	}
	else if (! _range.is_empty())
	{
		x.include(_range.x);
		y.include(_range.y);
	}
	return *this;
}

void Range2::intersect_with(Range2 const & _range)
{
	if (! is_empty())
	{
		x.intersect_with(_range.x);
		y.intersect_with(_range.y);
	}
}

/** have valid value or empty */
Range2 & Range2::include(Vector2 const & val)
{
	x.include(val.x);
	y.include(val.y);
	return *this;
}

/** needs at least one common point */
bool Range2::overlaps(Range2 const & _other) const
{
	return ! is_empty() && ! _other.is_empty() && x.overlaps(_other.x) && y.overlaps(_other.y);
}

/** cannot just touch, need to go into another */
bool Range2::intersects(Range2 const & _other) const
{
	return ! is_empty() && ! _other.is_empty() && x.intersects(_other.x) && y.intersects(_other.y);
}

bool Range2::intersects_extended(Range2 const & _other, float _by) const
{
	return ! is_empty() && ! _other.is_empty() && x.intersects_extended(_other.x, _by) && y.intersects_extended(_other.y, _by);
}

/** needs _other to be completely inside this Range2 */
bool Range2::does_contain(Range2 const & _other) const
{
	return x.does_contain(_other.x) && y.does_contain(_other.y);
}

bool Range2::does_contain(Vector2 const & _other) const
{
	return x.does_contain(_other.x) && y.does_contain(_other.y);
}

void Range2::expand_by(Vector2 const & _size)
{
	x.expand_by(_size.x);
	y.expand_by(_size.y);
}

Range2 Range2::moved_by(Vector2 const & _by) const
{
	return Range2(x.moved_by(_by.x), y.moved_by(_by.y));
}

Range2& Range2::move_by(Vector2 const & _by)
{
	x.move_by(_by.x);
	y.move_by(_by.y);
	return *this;
}

Range2 Range2::lerp(float _t, Range2 const& _a, Range2 const& _b)
{
	if (_a.is_empty()) return _b;
	if (_b.is_empty()) return _a;
	Range2 r;
	r.x.min = ::lerp(_t, _a.x.min, _b.x.min);
	r.x.max = ::lerp(_t, _a.x.max, _b.x.max);
	r.y.min = ::lerp(_t, _a.y.min, _b.y.min);
	r.y.max = ::lerp(_t, _a.y.max, _b.y.max);
	return r;
}
