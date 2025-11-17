template <>
inline bool load_value_from_xml<BezierCurveTBasedPoint<float>>(REF_ BezierCurveTBasedPoint<float> & _a, IO::XML::Node const * _node, tchar const * _valueName)
{
	if (_node)
	{
		if (auto node = _node->first_child_named(_valueName))
		{
			_a.t = node->get_float_attribute_or_from_child(TXT("t"), _a.t);
			_a.p = node->get_float(_a.p);
			_a.p = node->get_float_attribute_or_from_child(TXT("value"), _a.p);
			_a.p = node->get_float_attribute_or_from_child(TXT("v"), _a.p);
			return true;
		}
		if (auto attr = _node->get_attribute(_valueName))
		{
			_a.p = attr->get_as_float();
			return true;
		}
	}
	return false;
}
//
template <>
inline bool load_value_from_xml<BezierCurveTBasedPoint<Vector2>>(REF_ BezierCurveTBasedPoint<Vector2> & _a, IO::XML::Node const * _node, tchar const * _valueName)
{
	if (_node)
	{
		if (auto node = _node->first_child_named(_valueName))
		{
			_a.t = node->get_float_attribute_or_from_child(TXT("t"), _a.t);
			return _a.p.load_from_xml(node);
		}
	}
	return false;
}
//
template <>
inline bool load_value_from_xml<BezierCurveTBasedPoint<Vector3>>(REF_ BezierCurveTBasedPoint<Vector3> & _a, IO::XML::Node const * _node, tchar const * _valueName)
{
	if (_node)
	{
		if (auto node = _node->first_child_named(_valueName))
		{
			_a.t = node->get_float_attribute_or_from_child(TXT("t"), _a.t);
			return _a.p.load_from_xml(node);
		}
	}
	return false;
}
//
template <>
inline bool load_value_from_xml<BezierCurveTBasedPoint<Rotator3>>(REF_ BezierCurveTBasedPoint<Rotator3> & _a, IO::XML::Node const * _node, tchar const * _valueName)
{
	if (_node)
	{
		if (auto node = _node->first_child_named(_valueName))
		{
			_a.t = node->get_float_attribute_or_from_child(TXT("t"), _a.t);
			return _a.p.load_from_xml(node);
		}
	}
	return false;
}

template <>
inline float length_of<BezierCurveTBasedPoint<float>>(BezierCurveTBasedPoint<float> const & _a)
{
	return Vector2(_a.t, _a.p).length();
}
//
template <>
inline float length_of<BezierCurveTBasedPoint<Vector2>>(BezierCurveTBasedPoint<Vector2> const & _a)
{
	return Vector3(_a.t, _a.p.x, _a.p.y).length();
}
//
template <>
inline float length_of<BezierCurveTBasedPoint<Vector3>>(BezierCurveTBasedPoint<Vector3> const & _a)
{
	return Vector4(_a.t, _a.p.x, _a.p.y, _a.p.z).length();
}
//
template <>
inline float length_of<BezierCurveTBasedPoint<Rotator3>>(BezierCurveTBasedPoint<Rotator3> const & _a)
{
	return Vector4(_a.t, _a.p.pitch, _a.p.yaw, _a.p.roll).length();
}

template <>
inline BezierCurveTBasedPoint<float> normal<BezierCurveTBasedPoint<float>>(BezierCurveTBasedPoint<float> const & _a)
{
	Vector2 n = Vector2(_a.t, _a.p).normal();
	return BezierCurveTBasedPoint<float>(n.x, n.y);
}
//
template <>
inline BezierCurveTBasedPoint<Vector2> normal<BezierCurveTBasedPoint<Vector2>>(BezierCurveTBasedPoint<Vector2> const & _a)
{
	Vector3 n = Vector3(_a.t, _a.p.x, _a.p.y).normal();
	return BezierCurveTBasedPoint<Vector2>(n.x, Vector2(n.y, n.z));
}
//
template <>
inline BezierCurveTBasedPoint<Vector3> normal<BezierCurveTBasedPoint<Vector3>>(BezierCurveTBasedPoint<Vector3> const & _a)
{
	Vector4 n = Vector4(_a.t, _a.p.x, _a.p.y, _a.p.z).normal();
	return BezierCurveTBasedPoint<Vector3>(n.x, Vector3(n.y, n.z, n.w));
}
//
template <>
inline BezierCurveTBasedPoint<Rotator3> normal<BezierCurveTBasedPoint<Rotator3>>(BezierCurveTBasedPoint<Rotator3> const& _a)
{
	Vector4 n = Vector4(_a.t, _a.p.pitch, _a.p.yaw, _a.p.roll).normal();
	return BezierCurveTBasedPoint<Rotator3>(n.x, Rotator3(n.y, n.z, n.w));
}

template <>
inline float dot_product<BezierCurveTBasedPoint<float>>(BezierCurveTBasedPoint<float> const & _a, BezierCurveTBasedPoint<float> const & _b)
{
	return Vector2::dot(Vector2(_a.t, _a.p), Vector2(_b.t, _b.p));
}
//
template <>
inline float dot_product<BezierCurveTBasedPoint<Vector2>>(BezierCurveTBasedPoint<Vector2> const & _a, BezierCurveTBasedPoint<Vector2> const & _b)
{
	return Vector3::dot(Vector3(_a.t, _a.p.x, _a.p.y), Vector3(_b.t, _b.p.x, _b.p.y));
}
//
template <>
inline float dot_product<BezierCurveTBasedPoint<Vector3>>(BezierCurveTBasedPoint<Vector3> const & _a, BezierCurveTBasedPoint<Vector3> const & _b)
{
	return Vector4::dot(Vector4(_a.t, _a.p.x, _a.p.y, _a.p.z), Vector4(_b.t, _b.p.x, _b.p.y, _b.p.z));
}
//
template <>
inline float dot_product<BezierCurveTBasedPoint<Rotator3>>(BezierCurveTBasedPoint<Rotator3> const & _a, BezierCurveTBasedPoint<Rotator3> const & _b)
{
	return Vector4::dot(Vector4(_a.t, _a.p.pitch, _a.p.yaw, _a.p.roll), Vector4(_b.t, _b.p.pitch, _b.p.yaw, _b.p.roll));
}

//

template <typename Point>
float BezierCurveTBasedPoint<Point>::find_t_at(float _t, BezierCurve<BezierCurveTBasedPoint<Point>> const & _curve, OUT_ float * _distance, float _accuracyT, float _startingStepT)
{
	BezierCurve<float> tCurve;
	tCurve.p0 = _curve.p0.t;
	tCurve.p1 = _curve.p1.t;
	tCurve.p2 = _curve.p2.t;
	tCurve.p3 = _curve.p3.t;
	return tCurve.find_t_at(_t, _distance, _accuracyT, _startingStepT);
}

//

template <typename Point>
BezierCurve<Point>::BezierCurve()
{
}

template <typename Point>
BezierCurve<Point>::BezierCurve(Point const & _p0, Point const & _p3)
: p0(_p0)
, p3(_p3)
{
	make_linear();
}

template <typename Point>
BezierCurve<Point>::BezierCurve(Point const & _p0, Point const & _p1, Point const & _p2, Point const & _p3)
: p0(_p0)
, p1(_p1)
, p2(_p2)
, p3(_p3)
{
}

template <typename Point>
Point BezierCurve<Point>::calculate_at(float _t) const
{
	_t = clamp(_t, 0.0f, 1.0f);
	float ot = 1.0f - _t;
	return p0 * (cbc(ot))
		 + p1 * (3.0f * sqr(ot) * _t)
		 + p2 * (3.0f * ot * sqr(_t))
		 + p3 * (cbc(_t));
}

template <typename Point>
Point BezierCurve<Point>::calculate_tangent_at(float _t, float _step) const
{
	if (_t == 0.0f)
	{
		return normal_of(p1 - p0);
	}
	else if (_t == 1.0f)
	{
		return normal_of(p3 - p2);
	}
	else
	{
		Point a = calculate_at(clamp(_t - _step * 0.5f, 0.0f, 1.0f));
		Point b = calculate_at(clamp(_t + _step * 0.5f, 0.0f, 1.0f));
		return normal_of(b - a);
	}
}

template <typename Point>
void BezierCurve<Point>::make_linear()
{
	p1 = p0 * 0.666667f + p3 * 0.333333f;
	p2 = p0 * 0.333333f + p3 * 0.666667f;
}

template <typename Point>
void BezierCurve<Point>::make_constant()
{
	p1 = p0;
	p2 = p0;
	p3 = p0;
}

template <typename Point>
bool BezierCurve<Point>::is_linear() const
{
	Point p01 = p1 - p0;
	Point p02 = p2 - p0;
	Point p03 = p3 - p0;
	Point p32 = p2 - p3;
	float d01 = length_of(p01);
	float d02 = length_of(p02);
	float d03 = length_of(p03);
	float d32 = length_of(p32);
	if (d02 == 0.0f && d32 == 0.0f)
	{
		return true;
	}
	if (d03 != 0.0f)
	{
		Point np03 = p03 * (1.0f / d03);

		if ((d01 < MARGIN || length_of((p01 * (1.0f / d01)) - np03) < MARGIN) &&	// p01 is along p03
			(d32 < MARGIN || length_of((p32 * (1.0f / d32)) + np03) < MARGIN))		// p32 is along p30 (-p03)
		{
			return true;
		}
	}
	return false;
}

template <typename Point>
bool BezierCurve<Point>::is_degenerated() const
{
	return length_of(p1 - p0) == 0.0f
		&& length_of(p2 - p0) == 0.0f
		&& length_of(p3 - p0) == 0.0f;
}

template <typename Point>
float BezierCurve<Point>::distance(float _t0, float _t1, float _step) const
{
	float dist = 0.0f;
	Point prev = calculate_at(_t0);
	float t = min(_t1, _t0 + _step);
	while (t <= _t1)
	{
		Point point = calculate_at(t);
		dist += length_of(point - prev);
		prev = point;
		if (t < _t1)
		{
			t = min(_t1, t + _step);
		}
		else
		{
			t += _step;
		}
	}
	return dist;
}

template <typename Point>
float BezierCurve<Point>::get_t_at_distance(float _distance, float _accuracy, float _startingStep) const
{
	return get_t_at_distance_ex(_distance, 0.0f, true, _accuracy, _startingStep);
}

template <typename Point>
float BezierCurve<Point>::get_t_at_distance_from_end(float _distance, float _accuracy, float _startingStep) const
{
	return get_t_at_distance_ex(_distance, 1.0f, false, _accuracy, _startingStep);
}

template <typename Point>
float BezierCurve<Point>::get_t_at_distance_ex(float _distance, float _startAtT, bool _forward, float _accuracy, float _startingStep) const
{
	if (is_linear())
	{
		float len = length();
		if (len != 0.0f)
		{
			return (_startAtT * len + (_forward? _distance : -_distance)) / len;
		}
	}
	if (_distance < EPSILON)
	{
		return _startAtT;
	}
	// allow more precise depending on distance to check
	_accuracy = max(EPSILON, min(_accuracy, _distance * 0.1f));
	_startingStep = max(EPSILON, min(_startingStep, _distance * 0.1f));

	float dist = 0.0f;
	float step = _forward ? _startingStep : -_startingStep;
	Point prev = calculate_at(_startAtT);
	float t = _startAtT + step;
	while (true)
	{
		Point point = calculate_at(t);
		float prevDist = dist;
		dist += (((step > 0.0f) ^ (_forward))? -1.0f : 1.0f) * length_of(point - prev);
		prev = point;
		if (dist == _distance ||
			abs(dist - _distance) <= _accuracy)
		{
			return t;
		}
		if ((dist - _distance) * (prevDist - _distance) <= 0.0f)
		{
			step = -step * 0.5f;
			if (abs(step) <= _accuracy)
			{
				return t;
			}
		}
		if (t > 0.0f && t < 1.0f)
		{
			// allow to stop at 0 or 1
			t = clamp(t + step, 0.0f, 1.0f);
		}
		else
		{
			t += step;
			if (t >= 1.0f && step > 0.0f)
			{
				return 1.0f;
			}
			if (t <= 0.0f && step < 0.0f)
			{
				return 0.0f;
			}
		}
	}
}

template <typename Point>
float BezierCurve<Point>::calculate_round_separation() const
{
	Point const p01 = p1 - p0;
	Point const p32 = p2 - p3;
	Point const np01 = normal<Point>(p01);
	Point const np32 = normal<Point>(p32);

	// calculate basing on this
	//                 p3
	//	      p1      o
	//		   +. x*c/
	//		x /  `. / x
	//		 /     +
	//		o      p2
	//     p0
	// | (p0 + n(p1-p0) * x) - (p3 + n(p2-p3) * x) | = x * coef
	//		coef is used to have ratios 0.55191502449, 0.6336878494618582, 0.55191502449 note: http://spencermortensen.com/articles/bezier-circle/
	//			 .---o   whole thing is 1,1
	//		    /  d
	//		   .			d = 0.55191502449
	//		   |d
	//		   |
	//		   o 
	//		\/(1 - d)^2 + (1 - d)^2' = 0.6336878494618582
	//		coef is 0.6336878494618582 / 0.55191502449
	// which means that in our final solution we will have distances
	// |p0 p1| = |p2 p3| = |p1 p2| / coef
	// our result is length of that distance, "even separation"
	// | (p0 - p3) + (n(p1-p0) - n(p2-p3)) * x | = x * coef
	// | diff + diffCP * x | = x * coef
	// \/ diff^2 + 2 * diff * diffCP * x + diffCP^2 * x^2' = x * coef
	// diff^2 + 2 * diff * diffCP * x + diffCP^2 * x^2' = x^2 * coef^2
	// x^2 * (diffCP^2 - coef^2) + x * (2 * diff * diffCP) + diff^2 = 0

	Point const diff = (p0 - p3);
	Point const diffCP = (np01 - np32);
	float const coefSq = 1.318275941269866f; // see above (1.148161983898555 ^ 2)

	float a = length_squared_of(diffCP) - coefSq; // if a == 0 only if np01 and np32 would be parallel pointing in same direction
	float b = 2.0f * dot_product(diff, diffCP);
	float c = length_squared_of(diff);

	float result = 0;

	if (a != 0)
	{
		float delta = b*b - 4.0f*a*c;
		if (delta <= 0.0f)
		{
			// better this (as closest) than nothing
			result = max(0.0f, -b / (2.0f * a));
		}
		else
		{
			delta = sqrt(delta);
			a = 1.0f / (2.0f * a);
			float result1 = (-b - delta) * a;
			float result2 = (-b + delta) * a;
			// get smaller of two
			if (result1 < result2)
			{
				result = result1 > 0.0f ? result1 : max(result2, 0.0f);
			}
			else
			{
				result = result2 > 0.0f ? result2 : max(result1, 0.0f);
			}
		}
	}
	else
	{
		if (c > 0.0f)
		{
			result = sqrt(c);
		}
		else
		{
			result = 0.0f;
		}
	}

	return result;
}

template <typename Point>
void BezierCurve<Point>::make_roundly_separated()
{
	float roundSeparation = calculate_round_separation();

	float p01 = length_of(p1 - p0);
	float p32 = length_of(p2 - p3);

	if (p01 != 0.0f)
	{
		p1 = p0 + (p1 - p0) * (roundSeparation / p01);
	}
	if (p32 != 0.0f)
	{
		p2 = p3 + (p2 - p3) * (roundSeparation / p32);
	}
}

template <typename Point>
bool BezierCurve<Point>::load_from_xml(IO::XML::Node const * _node, bool _makeLinearByDefault, bool _autoRound)
{
	if (!_node)
	{
		return false;
	}
	bool result = true;
	// we need to have those values
	result &= load_value_from_xml(p0, _node, TXT("p0"));
	result &= load_value_from_xml(p3, _node, TXT("p3"));
	if (_makeLinearByDefault)
	{
		make_linear();
	}
	// we can skip those
	load_value_from_xml(p1, _node, TXT("p12"));
	load_value_from_xml(p2, _node, TXT("p12"));
	load_value_from_xml(p1, _node, TXT("p1"));
	load_value_from_xml(p2, _node, TXT("p2"));
	if (_autoRound)
	{
		if (_node->get_bool_attribute_or_from_child_presence(TXT("withEvenSeparation")) ||
			_node->get_bool_attribute_or_from_child_presence(TXT("evenSeparation")) ||
			_node->get_bool_attribute_or_from_child_presence(TXT("evenlySeparated")))
		{
			warn_loading_xml(_node, TXT("use round* instead of even*"));
			make_roundly_separated();
		}
		if (_node->get_bool_attribute_or_from_child_presence(TXT("withRoundSeparation")) ||
			_node->get_bool_attribute_or_from_child_presence(TXT("roundSeparation")) ||
			_node->get_bool_attribute_or_from_child_presence(TXT("roundlySeparated")))
		{
			make_roundly_separated();
		}
	}
	return result;
}

template <typename Point>
float BezierCurve<Point>::find_t_at(Point const & _p, OUT_ float * _distance, float _accuracyT, float _startingStep) const
{
	float startAtT = 0.5f;

	// allow more precise depending on distance to check
	float step = _startingStep;
	Point prev = calculate_at(startAtT);
	float prevDist = length_of(prev - _p);
	if (length_of(calculate_at(startAtT + 0.001f) - _p) > prevDist)
	{
		step = -step;
	}
	float t = startAtT + step;
	while (true)
	{
		Point point = calculate_at(t);
		float dist = length_of(point - _p);
		if (dist > prevDist)
		{
			step = -step * 0.5f;
		}
		if (abs(step) < _accuracyT)
		{
			break;
		}
		if (t > 0.0f && t < 1.0f)
		{
			if (abs(step) > _accuracyT * 2.0f &&
				(t + step >= 1.0f || t + step <= 0.0f))
			{
				step *= 0.5f; // slow down
			}
			// allow to stop at 0 or 1
			t = clamp(t + step, 0.0f, 1.0f);
		}
		else
		{
			t += step;
			if (t >= 1.0f && step > 0.0f)
			{
				if (step > _accuracyT * 2.0f)
				{
					step = -step * 0.75f;
					continue;
				}
				t = 1.0f;
				break;
			}
			if (t <= 0.0f && step < 0.0f)
			{
				if (step > _accuracyT * 2.0f)
				{
					step = -step * 0.75f;
					continue;
				}
				t = 0.0f;
				break;
			}
		}
		prev = point;
		prevDist = dist;
	}
	if (_distance)
	{
		Point point = calculate_at(t);
		*_distance = length_of(point - _p);
	}
	return t;
}

template <typename Point>
String BezierCurve<Point>::to_string() const
{
	return String::printf(TXT("curve[p0:%S ; p1:%S ; p2:%S ; p3:%S]"),
		::to_string<Point>(p0).to_char(),
		::to_string<Point>(p1).to_char(),
		::to_string<Point>(p2).to_char(),
		::to_string<Point>(p3).to_char());
}
