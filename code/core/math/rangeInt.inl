String RangeInt::to_string() const
{
	return String::printf(TXT("min:%i max:%i"), min, max);
}

String RangeInt::to_parsable_string() const
{
	return String::printf(TXT("%i,%i"), min, max);
}

Range RangeInt::to_range() const
{
	return Range((float)min, (float)max);
}

void RangeInt::include(RangeInt const & _range)
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

void RangeInt::intersect_with(RangeInt const & _range)
{
	if (! is_empty())
	{
		min = ::max(min, _range.min);
		max = ::min(max, _range.max);
	}
}

/** have valid value or empty */
void RangeInt::include(int val)
{
	min = ::min(min, val);
	max = ::max(max, val);
}

void RangeInt::expand_by(int _by)
{
	min -= _by;
	max += _by;
}

void RangeInt::offset(int _by)
{
	min += _by;
	max += _by;
}

void RangeInt::limit_min(int _to)
{
	min = ::max(min, _to);
}

void RangeInt::limit_max(int _to)
{
	max = ::min(max, _to);
}

RangeInt RangeInt::get_limited_to(RangeInt const & _range) const
{
	RangeInt r = *this;
	if (is_empty())
	{
		r = _range;
	}
	else if (_range.is_empty())
	{
		// don't limit
	}
	else
	{
		if (r.max <= _range.min)
		{
			r.min = r.max = _range.min;
		}
		else if (r.min >= _range.max)
		{
			r.max = r.min = _range.max;
		}
		else
		{
			r.min = ::max(r.min, _range.min);
			r.max = ::min(r.max, _range.max);
		}
	}
	return r;
}

RangeInt RangeInt::operator*(int _by) const
{
	return RangeInt(min * _by, max * _by);
}
		
/** needs at least one common point */
bool RangeInt::overlaps(RangeInt const & _other) const
{
	return ! is_empty() && ! _other.is_empty() && max >= _other.min && min <= _other.max;
}

/** cannot just touch, need to go into another */
bool RangeInt::intersects(RangeInt const & _other) const
{
	return ! is_empty() && ! _other.is_empty() && max > _other.min && min < _other.max;
}

bool RangeInt::intersects_extended(RangeInt const & _other, int _by) const
{
	return ! is_empty() && ! _other.is_empty() && max > _other.min - _by && min < _other.max + _by;
}

/** needs _other to be completely inside this RangeInt */
bool RangeInt::does_contain(RangeInt const & _other) const
{
	return min <= _other.min && max >= _other.max;
}

int RangeInt::distance_from(int _value) const
{
	if (! is_empty())
	{
		return _value > max? _value - max :
			  (_value < min? min - _value : 0);
	}
	return 0; // TODO infinity
}
		
int RangeInt::clamp_extended(int _value, int _by) const
{
	if (min <= max)
	{
		if (_by < 0.0f)
		{
			if (_value < min) { return min - _by; }
			if (_value > max) { return max + _by; }
			_by = ::max(-(max - min) / 2, _by);
		}
		return ::clamp(_value, min - _by, max + _by);
	}
	else
	{
		return _value;
	}
}
		