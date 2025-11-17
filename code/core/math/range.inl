void Range::include(Range const & _range)
{
	if (is_empty())
	{
		min = _range.min;
		max = _range.max;
	}
	else if (! _range.is_empty())
	{
		min = ::min(min, _range.min);
		max = ::max(max, _range.max);
	}
}

void Range::intersect_with(Range const & _range)
{
	if (! is_empty())
	{
		min = ::max(min, _range.min);
		max = ::min(max, _range.max);
	}
}

/** have valid value or empty */
void Range::include(float val)
{
	min = ::min(min, val);
	max = ::max(max, val);
}

void Range::expand_by(float _by)
{
	min -= _by;
	max += _by;
}

void Range::offset(float _by)
{
	min += _by;
	max += _by;
}

void Range::limit_min(float _to)
{
	min = ::max(min, _to);
}

void Range::limit_max(float _to)
{
	max = ::min(max, _to);
}

void Range::scale_relative_to_centre(float _by)
{
	float c = centre();
	min = c + (min - c) * _by;
	max = c + (max - c) * _by;
}

Range Range::moved_by(float _by) const
{
	return Range(min + _by, max + _by);
}

Range& Range::move_by(float _by)
{
	min += _by;
	max += _by;
	return *this;
}

Range Range::operator*(float _by) const
{
	return Range(min * _by, max * _by);
}
		
Range & Range::operator*=(float _by)
{
	min *= _by;
	max *= _by;
	return *this;
}

Range Range::operator+(float _by) const
{
	return Range(min + _by, max + _by);
}
		
Range & Range::operator+=(float _by)
{
	min += _by;
	max += _by;
	return *this;
}

Range Range::operator+(Range const & _o) const
{
	Range r = *this;
	r.min += _o.min;
	r.max += _o.max;
	return r;
}

Range Range::operator-(Range const & _o) const
{
	Range r = *this;
	r.min -= _o.min;
	r.max -= _o.max;
	return r;
}

/** needs at least one common point */
bool Range::overlaps(Range const & _other) const
{
	return ! is_empty() && ! _other.is_empty() && max >= _other.min && min <= _other.max;
}

/** cannot just touch, need to go into another */
bool Range::intersects(Range const & _other) const
{
	return ! is_empty() && ! _other.is_empty() && max > _other.min && min < _other.max;
}

bool Range::intersects_extended(Range const & _other, float _by) const
{
	return ! is_empty() && ! _other.is_empty() && max > _other.min - _by && min < _other.max + _by;
}

/** needs _other to be completely inside this range */
bool Range::does_contain(Range const & _other) const
{
	return min <= _other.min && max >= _other.max;
}

float Range::distance_from(float _value) const
{
	if (! is_empty())
	{
		return _value > max? _value - max :
				(_value < min? min - _value : 0.0f);
	}
	return 0.0f; // TODO infinity
}
		
float Range::clamp_extended(float _value, float _by) const
{
	if (min <= max)
	{
		if (_by < 0.0f)
		{
			if (_value < min) { return min - _by; }
			if (_value > max) { return max + _by; }
			_by = ::max(-(max - min) * 0.5f, _by);
		}
		return ::clamp(_value, min - _by, max + _by);
	}
	else
	{
		return _value;
	}
}
		
Range Range::get_part(Range const & _pt) const
{
	Range result;
	float const min2max = max - min;
	result.min = min + min2max * _pt.min;
	result.max = min + min2max * _pt.max;
	return result;
}

Range Range::lerp(float _t, Range const& _a, Range const& _b)
{
	Range r;
	r.min = ::lerp(_t, _a.min, _b.min);
	r.max = ::lerp(_t, _a.max, _b.max);
	return r;
}
