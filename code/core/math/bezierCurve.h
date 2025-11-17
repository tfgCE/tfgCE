#pragma once

#ifndef INCLUDED_MATH_H
#error include math.h first
#endif

/**
 *	This encapsulates point as t based, eg instead of having just vector2 along t, we have additonal t parameter vector2 as set of t,x,y.
 *	It makes it possible to have more complex curves
 */
template <typename Point>
struct BezierCurveTBasedPoint
{
	float t;
	Point p;

	BezierCurveTBasedPoint() {}
	BezierCurveTBasedPoint(float _t, Point const & _p) : t(_t), p(_p) {}

	inline BezierCurveTBasedPoint operator +(BezierCurveTBasedPoint const & _a) const { return BezierCurveTBasedPoint(t + _a.t, p + _a.p); }
	inline BezierCurveTBasedPoint operator -(BezierCurveTBasedPoint const & _a) const { return BezierCurveTBasedPoint(t - _a.t, p - _a.p); }
	inline BezierCurveTBasedPoint operator *(BezierCurveTBasedPoint const & _a) const { return BezierCurveTBasedPoint(t * _a.t, p * _a.p); }
	inline BezierCurveTBasedPoint operator *(float _a) const { return BezierCurveTBasedPoint(t * _a, p * _a); }

	inline static float find_t_at(float _t, BezierCurve<BezierCurveTBasedPoint<Point>> const & _curve, OUT_ float * _distance = nullptr, float _accuracyT = ACCURACY, float _startingStepT = 0.1f);
};

template <typename Point>
struct BezierCurve
{
	Point p0; // start
	Point p1;
	Point p2;
	Point p3; // end

	BezierCurve();
	BezierCurve(Point const & _p0, Point const & _p3);
	BezierCurve(Point const & _p0, Point const & _p1, Point const & _p2, Point const & _p3);

	BezierCurve & operator *= (Point const & _scale) { p0 *= _scale; p1 *= _scale; p2 *= _scale; p3 *= _scale; return *this; }
	bool operator==(BezierCurve const & _other) const { return p0 == _other.p0 && p1 == _other.p1 && p2 == _other.p2 && p3 == _other.p3; }

	Point calculate_at(float _t) const;
	Point calculate_tangent_at(float _t, float _step = SMALL_STEP) const;

	bool load_from_xml(IO::XML::Node const * _node, bool _makeLinearByDefault = true, bool _autoRound = false); // _ignoreAutoMakings to read exactly as was read

	void make_linear();
	void make_constant();
	bool is_linear() const;

	bool is_degenerated() const;

	BezierCurve reversed() const { return BezierCurve(p3, p2, p1, p0); }
	BezierCurve reversed(bool _reversed) const { return _reversed? reversed() : *this; }

	float length() const { return distance(); }
	float distance(float _t0 = 0.0f, float _t1 = 1.0f, float _step = SMALL_STEP) const;
	float get_t_at_distance(float _distance, float _accuracy = ACCURACY, float _startingStep = SMALL_STEP) const;
	float get_t_at_distance_from_end(float _distance, float _accuracy = ACCURACY, float _startingStep = SMALL_STEP) const;
	float get_t_at_distance_ex(float _distance, float _startAtT, bool _forward, float _accuracy = ACCURACY, float _startingStep = SMALL_STEP) const;

	float find_t_at(Point const & _p, OUT_ float * _distance = nullptr, float _accuracyT = ACCURACY, float _startingStepT = 0.1f) const;

	float calculate_round_separation() const;
	void make_roundly_separated();

	String to_string() const;
};