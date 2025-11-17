
void Segment2::update(Vector2 const & _start, Vector2 const & _end)
{
	start = _start;
	end = _end;
	startToEnd = end - start;
	startToEndLength = startToEnd.length();
	startToEndDir = startToEndLength != 0.0f ? startToEnd / startToEndLength : Vector2::zero;
}

bool Segment2::does_intersect_with(Segment2 const & _other, float _epsilon) const
{
	if (abs(Vector2::dot(startToEndDir, _other.startToEndDir)) >= 0.999f)
	{
		// parallel
		return false;
	}

	// check for both segments if points of other segment lay on one or other side
	// if they lay on one or other side, they are not intersecting

	{
		Vector2 rightDir = startToEndDir.rotated_right();
		float startT = Vector2::dot(rightDir, _other.start - start);
		float endT = Vector2::dot(rightDir, _other.end - start);

		if ((startT >= -_epsilon && endT >= -_epsilon) ||
			(startT <=  _epsilon && endT <=  _epsilon))
		{
			return false;
		}
	}

	{
		Vector2 otherRightDir = _other.startToEndDir.rotated_right();
		float startT = Vector2::dot(otherRightDir, start - _other.start);
		float endT = Vector2::dot(otherRightDir, end - _other.start);

		if ((startT >= -_epsilon && endT >= -_epsilon) ||
			(startT <=  _epsilon && endT <=  _epsilon))
		{
			return false;
		}
	}

	return true;
}

Optional<float> Segment2::calc_intersect_with(Segment2 const& _other, float _epsilon) const
{
	if (abs(Vector2::dot(startToEndDir, _other.startToEndDir)) >= 0.999f)
	{
		// parallel
		return NP;
	}

	{
		Vector2 otherRightDir = _other.startToEndDir.rotated_right();
		float startT = Vector2::dot(otherRightDir, start - _other.start);
		float endT = Vector2::dot(otherRightDir, end - _other.start);

		if ((startT >= -_epsilon && endT >= -_epsilon) ||
			(startT <= _epsilon && endT <= _epsilon))
		{
			return NP;
		}

		float atT = (0.0f - startT) / (endT - startT);
		Vector2 at0 = get_at_t(atT);

		float alongOther = Vector2::dot(at0 - _other.start, _other.get_start_to_end_dir());

		if (alongOther >= 0.0f && alongOther <= _other.length())
		{
			return atT;
		}

	}

	return NP;
}

float Segment2::calculate_distance_from(Vector2 const & _point) const
{
	Vector2 const offset = _point - start;
	float t = clamp(Vector2::dot(startToEndDir, offset), 0.0f, startToEndLength);
	Vector2 const pointOnSegmentOffset = t * startToEndDir;
	return (offset - pointOnSegmentOffset).length();
}

float Segment2::calculate_distance_from(Segment2 const & _other) const
{
	if (does_intersect_with(_other))
	{
		return 0.0f;
	}

	return min(calculate_distance_from(_other.start), calculate_distance_from(_other.end));
}
