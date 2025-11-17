String RangeRotator3::to_debug_string() const
{
	return String::printf(TXT("pitch:%S yaw:%S roll:%S"), pitch.to_debug_string().to_char(), yaw.to_debug_string().to_char(), roll.to_debug_string().to_char());
}

void RangeRotator3::include(RangeRotator3 const & _range)
{
	if (is_empty())
	{
		pitch = _range.pitch;
		yaw = _range.yaw;
		roll = _range.roll;
	}
	else
	{
		pitch.include(_range.pitch);
		yaw.include(_range.yaw);
		roll.include(_range.roll);
	}
}

void RangeRotator3::intersect_with(RangeRotator3 const & _range)
{
	if (! is_empty())
	{
		pitch.intersect_with(_range.pitch);
		yaw.intersect_with(_range.yaw);
		roll.intersect_with(_range.roll);
	}
}

/** have valid value or empty */
void RangeRotator3::include(Rotator3 const & val)
{
	pitch.include(val.pitch);
	yaw.include(val.yaw);
	roll.include(val.roll);
}

void RangeRotator3::expand_by(Rotator3 const & _by)
{
	pitch.expand_by(_by.pitch);
	yaw.expand_by(_by.yaw);
	roll.expand_by(_by.roll);
}

void RangeRotator3::scale_relative_to_centre(Rotator3 const& _by)
{
	pitch.scale_relative_to_centre(_by.pitch);
	yaw.scale_relative_to_centre(_by.yaw);
	roll.scale_relative_to_centre(_by.roll);
}

/** needs at least one common point */
bool RangeRotator3::overlaps(RangeRotator3 const & _other) const
{
	return ! is_empty() && ! _other.is_empty() && pitch.overlaps(_other.pitch) && yaw.overlaps(_other.yaw) && roll.overlaps(_other.roll);
}

/** cannot just touch, need to go into another */
bool RangeRotator3::intersects(RangeRotator3 const & _other) const
{
	return ! is_empty() && ! _other.is_empty() && pitch.intersects(_other.pitch) && yaw.intersects(_other.yaw) && roll.intersects(_other.roll);
}

bool RangeRotator3::intersects_extended(RangeRotator3 const & _other, float _by) const
{
	return ! is_empty() && ! _other.is_empty() && pitch.intersects_extended(_other.pitch, _by) && yaw.intersects_extended(_other.yaw, _by) && roll.intersects_extended(_other.roll, _by);
}

/** needs _other to be completely inside this RangeRotator3 */
bool RangeRotator3::does_contain(RangeRotator3 const & _other) const
{
	return pitch.does_contain(_other.pitch) && yaw.does_contain(_other.yaw) && roll.does_contain(_other.roll);
}

bool RangeRotator3::does_contain(Rotator3 const & _point) const
{
	return pitch.does_contain(_point.pitch) && yaw.does_contain(_point.yaw) && roll.does_contain(_point.roll);
}

RangeRotator3 RangeRotator3::lerp(float _t, RangeRotator3 const& _a, RangeRotator3 const& _b)
{
	RangeRotator3 r;
	r.pitch = ::lerp(_t, _a.pitch, _b.pitch);
	r.yaw = ::lerp(_t, _a.yaw, _b.yaw);
	r.roll = ::lerp(_t, _a.roll, _b.roll);
	return r;
}
