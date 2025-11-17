String RangeInt3::to_string() const
{
	return String::printf(TXT("x:%S y:%S z:%S"), x.to_string().to_char(), y.to_string().to_char(), z.to_string().to_char());
}

void RangeInt3::include(RangeInt3 const & _range)
{
	if (is_empty())
	{
		x = _range.x;
		y = _range.y;
		z = _range.z;
	}
	else if (! _range.is_empty())
	{
		x.include(_range.x);
		y.include(_range.y);
		z.include(_range.z);
	}
}

void RangeInt3::intersect_with(RangeInt3 const & _range)
{
	if (! is_empty())
	{
		x.intersect_with(_range.x);
		y.intersect_with(_range.y);
		z.intersect_with(_range.z);
	}
}

/** have valid value or empty */
void RangeInt3::include(VectorInt3 const & val)
{
	x.include(val.x);
	y.include(val.y);
	z.include(val.z);
}

void RangeInt3::expand_by(VectorInt3 const & val)
{
	x.expand_by(val.x);
	y.expand_by(val.y);
	z.expand_by(val.z);
}

void RangeInt3::offset(VectorInt3 const & _by)
{
	x.offset(_by.x);
	y.offset(_by.y);
	z.offset(_by.z);
}

/** needs at least one common point */
bool RangeInt3::overlaps(RangeInt3 const & _other) const
{
	return ! is_empty() && ! _other.is_empty() && x.overlaps(_other.x) && y.overlaps(_other.y) && z.overlaps(_other.z);
}

/** cannot just touch, need to go into another */
bool RangeInt3::intersects(RangeInt3 const & _other) const
{
	return ! is_empty() && ! _other.is_empty() && x.intersects(_other.x) && y.intersects(_other.y) && z.intersects(_other.z);
}

/** needs _other to be completely inside this RangeInt3 */
bool RangeInt3::does_contain(RangeInt3 const & _other) const
{
	return x.does_contain(_other.x) && y.does_contain(_other.y) && z.does_contain(_other.z);
}

bool RangeInt3::does_contain(VectorInt3 const & _point) const
{
	return x.does_contain(_point.x) && y.does_contain(_point.y) && z.does_contain(_point.z);
}

VectorInt3 RangeInt3::centre() const
{
	return VectorInt3(x.centre(), y.centre(), z.centre());
}