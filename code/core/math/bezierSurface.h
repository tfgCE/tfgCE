#pragma once

#ifndef INCLUDED_MATH_H
#error include math.h first
#endif

namespace BezierSurfaceType
{
	enum Type
	{
		None = 0,
		Curve = 2,
		Triangle = 3,
		Quad = 4
	};
};

namespace BezierSurfaceCreationMethod
{
	enum Type
	{
		Roundly,
		AlongU, // surface goes along u (fills missing links along u, bends along v)
		AlongV, // surface goes along v (fills missing links along v, bends along u)
	};
};

struct BezierSurface
{
public:
	BezierSurface();
	BezierSurface(BezierSurfaceType::Type _type, int _controlPointCount = 0, Vector3 const * _points = nullptr);

	bool get_uv_for_border(BezierCurve<Vector3> const & _forCurve, OUT_ Vector2 & _outStart, OUT_ Vector2 & _outEnd) const; // only for borders

	Vector3 calculate_at(float const _u, float const _v = 0.0f) const; // calculate point using given coordinates
	Vector3 calculate_at(Vector2 const & _coord) const { return calculate_at(_coord.x, _coord.y); }

	Vector3 calculate_normal_at(float const _u, float const _v = 0.0f, float const _step = SMALL_STEP) const;
	Vector3 calculate_normal_at(Vector2 const & _coord, float const _step = SMALL_STEP) const { return calculate_normal_at(_coord.x, _coord.y, _step); }

	Vector2 get_centre_uv() const;
	BezierSurfaceType::Type get_type() const { return (BezierSurfaceType::Type)vertexCount; }

	static BezierSurface create(BezierCurve<Vector3> const & _curve);
	static BezierSurface create(BezierCurve<Vector3> const & _alongU, BezierCurve<Vector3> const & _opposite, BezierCurve<Vector3> const & _revAlongV); // clockwise starting along U
	static BezierSurface create(BezierCurve<Vector3> const & _u01v0, BezierCurve<Vector3> const & _u1v01, BezierCurve<Vector3> const & _u10v1, BezierCurve<Vector3> const & _u0v10, BezierSurfaceCreationMethod::Type _method = BezierSurfaceCreationMethod::Roundly); // clockwise starting along U

private:
	int vertexCount = 0; // 2 to 4, same as type
	int controlPointCount = 0; // 0 : just vertices, 1 : vertices + one control point between, 2 : vertives + two control points between and so on

	// for time being it is hardcoded to allow quad with up to 2 control points and this is default
	ArrayStatic<Vector3, 16> points; // control points. layed along first coordinate (u) then skipped to second column and so on
	/*	1  4	        1   4		1  4 9 e j o
	 *	|  3	       /   3 8		|  3 8 d i n
	 *	u  2	      u   2 7 b		u  2 7 c h m
	 *	|  1	     /	 1 6 a d	|  1 6 b g l
	 *	0  0	    0	0 5 9 c e	0  0 5 a f k
	 *
	 *					0 - v - 1	   0 - v - 1
	 */

	static int calculate_point_count(int const _vertexCount, int const _controlPointCount);

	Vector3 calculate_point_decreasing_control_points_count(float const _u, float const _v, Vector3 const * _usingPoints, int const _usingControlPointCount) const;
};
