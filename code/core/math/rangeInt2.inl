String RangeInt2::to_string() const
{
	return String::printf(TXT("x:%S y:%S"), x.to_string().to_char(), y.to_string().to_char());
}

void RangeInt2::include(RangeInt2 const & _range)
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
}

void RangeInt2::intersect_with(RangeInt2 const & _range)
{
	if (! is_empty())
	{
		x.intersect_with(_range.x);
		y.intersect_with(_range.y);
	}
}

/** have valid value or empty */
void RangeInt2::include(VectorInt2 const & val)
{
	x.include(val.x);
	y.include(val.y);
}

void RangeInt2::expand_by(VectorInt2 const & val)
{
	x.expand_by(val.x);
	y.expand_by(val.y);
}

void RangeInt2::offset(VectorInt2 const& _by)
{
	x.offset(_by.x);
	y.offset(_by.y);
}

/** needs at least one common point */
bool RangeInt2::overlaps(RangeInt2 const & _other) const
{
	return ! is_empty() && ! _other.is_empty() && x.overlaps(_other.x) && y.overlaps(_other.y);
}

/** cannot just touch, need to go into another */
bool RangeInt2::intersects(RangeInt2 const & _other) const
{
	return ! is_empty() && ! _other.is_empty() && x.intersects(_other.x) && y.intersects(_other.y);
}

/** needs _other to be completely inside this RangeInt2 */
bool RangeInt2::does_contain(RangeInt2 const & _other) const
{
	return x.does_contain(_other.x) && y.does_contain(_other.y);
}

bool RangeInt2::does_contain(VectorInt2 const & _point) const
{
	return x.does_contain(_point.x) && y.does_contain(_point.y);
}

VectorInt2 RangeInt2::centre() const
{
	return VectorInt2(x.centre(), y.centre());
}

Range2 RangeInt2::to_range2() const
{
	return Range2(x.to_range(), y.to_range());
}
