#include "math.h"

float find_closest_line_curve_point(Vector2 const & _p, Vector2 const & _dir, BezierCurve<Vector2> const & _curve, float _step)
{
	Vector2 dir = _dir.normal();
	Vector2 right = dir.rotated_right();

	float closestT = -1.0f;
	float closestDist = 0.0f;
	for (float t = 0.0f; t <= 1.0f; t += _step)
	{
		Vector2 atT = _curve.calculate_at(t);

		float dist = abs(Vector2::dot(atT - _p, right));
		if (dist < closestDist || closestT < 0.0f)
		{
			closestT = t;
			closestDist = dist;
		}
	}

	return closestT;
}

float remap_value(float _value, float _from0, float _from1, float _to0, float _to1, bool _clamp)
{
	float at1 = _from1 != _from0 ? (_value - _from0) / (_from1 - _from0) : 0.5f;
	if (_clamp)
	{
		at1 = clamp(at1, 0.0f, 1.0f);
	}
	return _to0 + (_to1 - _to0) * at1;
}
