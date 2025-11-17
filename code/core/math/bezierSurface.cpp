#include "math.h"

#include "..\containers\arrayStack.h"

BezierSurface::BezierSurface()
{
	SET_EXTRA_DEBUG_INFO(points, TXT("BezierSurface.points"));
}

BezierSurface::BezierSurface(BezierSurfaceType::Type _type, int _controlPointCount, Vector3 const * _points)
: vertexCount(_type)
, controlPointCount(_controlPointCount)
{
	SET_EXTRA_DEBUG_INFO(points, TXT("BezierSurface.points"));

	points.set_size(calculate_point_count(vertexCount, controlPointCount));
	memory_copy(points.get_data(), _points, points.get_data_size());
}

Vector3 BezierSurface::calculate_normal_at(float const _u, float const _v, float const _step) const
{
	Vector3 um = calculate_at(clamp(_u - _step, 0.0f, 1.0f), _v);
	Vector3 up = calculate_at(clamp(_u + _step, 0.0f, 1.0f), _v);
	Vector3 vm = calculate_at(_u, clamp(_v - _step, 0.0f, 1.0f));
	Vector3 vp = calculate_at(_u, clamp(_v + _step, 0.0f, 1.0f));

	Vector3 ut = up - um;
	Vector3 vt = vp - vm;

	return Vector3::cross(vt, ut).normal();
}

Vector3 BezierSurface::calculate_at(float const _u, float const _v) const
{
	return calculate_point_decreasing_control_points_count(_u, _v, points.get_data(), controlPointCount);
}

Vector3 BezierSurface::calculate_point_decreasing_control_points_count(float const _u, float const _v, Vector3 const * _usingPoints, int const _usingControlPointCount) const
{
	// lcpc - lower control points count, we use same size as it will be faster than recalculating
	allocate_stack_var(Vector3, lcpc, points.get_size());
	
	// this is based on De Casteljau's algorithm
	// idea is to go from set created for "control point count" to set created for "control point count" minus one
	// each point in new set is in the middle of section/triangle/quad - calculated as weight of vertices composing section/triangle/quad

	// note: iu <= _usingControlPointCount is actually
	//		iu
	//			<	
	//				(2 + _usingControlPointCount)	<- number of vertices in row/column
	//				- 1								<- because we are decreasing control points count
	//	which is:
	//		iu < (2 + _usingControlPointCount) - 1
	//		iu < 1 + _usingControlPointCount
	//		iu <= _usingControlPointCount (because those are integers

	if (vertexCount == 2)
	{
		Vector3 *pr = lcpc;
		Vector3 const *pp1 = &_usingPoints[0];
		Vector3 const *pp2 = &_usingPoints[1];
		for (int iu = 0; iu <= _usingControlPointCount; ++ iu)
		{
			*pr = (*pp2 - *pp1) * _u + (*pp1);
			// go to next section
			++ pr;
			++ pp1;
			++ pp2;
		}
	}

	if (vertexCount == 3)
	{
		/*        ^
		 *		 /	.      _
		 *		u  . .      \
		 *		  . . .      > control point count == 3
		 *	ppu- . . . .   _/
		 *		. . . . .
		 *	   /  |  v->
		 *	 pp  ppv	pr is in the middle
		 *	
		 */

		Vector3 *pr = lcpc;
		Vector3 const * pp = &_usingPoints[0];
		Vector3 const * ppu = &_usingPoints[1];
		Vector3 const * ppv = &_usingPoints[2 + _usingControlPointCount];
		for (int iv = 0; iv <= _usingControlPointCount; ++ iv)
		{
			for (int iu = 0; iu <= _usingControlPointCount - iv; ++ iu)
			{
				*pr = (*ppu - *pp) * _u
					+ (*ppv - *pp) * _v
					+ (*pp);
				// go to next triangle along u
				++ pr;
				++ pp;
				++ ppu;
				++ ppv;
			}
			// in current iv column that is ending we will end up this loop (iu) with:
			//	pp pointing at last point in column
			//	ppu pointing at first point in next column
			//	ppv pointing at first point in it's counterpart column (actually column + 2)
			// but we want pp to be first point in next column and ppu second (ppv is pointing at proper one)
			// ...and we're indexing with number of points in new points table (with decreased control point count)
			// so we have to make up for it in source points table
			++pp;
			++ ppu;
		}
	}

	if (vertexCount == 4)
	{
		/*
		 *		. . . . .  _
		 *		. . . . .   \
		 *		. . . . .    > dokladnosc=3
		 *	ppu-. * . . .  _/
		 *		. . . . .
		 *	   /  |
		 *	 pp  ppv     * is ppuv
		 *
		 *	pr is in the middle
		 */

		Vector3 *pr = lcpc;
		Vector3 const * pp = &_usingPoints[0];
		Vector3 const * ppu = &_usingPoints[1];
		Vector3 const * ppv = &_usingPoints[2 + _usingControlPointCount];
		Vector3 const * ppuv = &_usingPoints[2 + _usingControlPointCount + 1];

		for (int iv = 0; iv <= _usingControlPointCount; ++ iv)
		{
			for (int iu = 0; iu <= _usingControlPointCount; ++ iu)
			{
				Vector3 u0 = (*ppv - *pp) * _v + (*pp); // pp -> ppv
				Vector3 u1 = (*ppuv - *ppu) * _v + (*ppu); // ppu -> ppuv
				*pr = (u1 - u0) * _u + (u0);
				// next quad
				++ pr;
				++ pp;
				++ ppu;
				++ ppv;
				++ ppuv;
			}
			// pp will point at last point in current column and so on
			// we need to jump to next column
			// ...and we're indexing with number of points in new points table (with decreased control point count)
			// so we have to make up for it in source points table
			++pp;
			++ppu;
			++ppv;
			++ppuv;
		}
	}

	if (_usingControlPointCount == 0)
	{
		// this is our final point!
		return *lcpc;
	}

	return calculate_point_decreasing_control_points_count(_u, _v, lcpc, _usingControlPointCount - 1);
}

int BezierSurface::calculate_point_count(int const _vertexCount, int const _controlPointCount)
{
	if (_vertexCount == 2)
	{
		return 2 + _controlPointCount;
	}
	if (_vertexCount == 3)
	{
		/*
		 *	    .      _
		 *	   . .      \
		 *	  . . .      > control point count == 3
		 *	 . . . .   _/
		 *	. . . . .
		 *
		 */
		// just go in rows starting with top
		int pointCount = 0;
		for (int i = 0; i < 2 + _controlPointCount; ++i)
		{
			pointCount += i + 1;
		}
		return pointCount;
    }
	if (_vertexCount == 4)
	{
		/*
		 *	. . . . .  _
		 *	. . . . .   \
		 *	. . . . .    > control point count == 3
		 *	. . . . .  _/
		 *	. . . . .
		 *
		 */
		int pointCount = 2 + _controlPointCount;
		return sqr(pointCount);
    }

	return 0;
}

BezierSurface BezierSurface::create(BezierCurve<Vector3> const & _curve)
{
	return BezierSurface(BezierSurfaceType::Curve, 2, &_curve.p0); // p0, p1, p2, p3 are aligned
}

BezierSurface BezierSurface::create(BezierCurve<Vector3> const & _alongU, BezierCurve<Vector3> const & _opposite, BezierCurve<Vector3> const & _revAlongV)
{
	/*
	 *	?? - calculated
	 *
	 *						au.p3/op.p0
	 *
	 *					au.p2		op.p1
	 *
	 *				au.p1		??		op.p2
	 *
	 *			au.p0/	  rav.p2   rav.p1	op.p3/rav.p0
	 *			  rav.p3
	 *
	 *			for each vertex we get two neighbour control points
	 *			we calculate middle point between each such pair
	 *			then we take all three results and calculate middle point for that
	 *			and we have our final missing control point
	 */

	an_assert(_alongU.p0 == _revAlongV.p3);
	an_assert(_alongU.p3 == _opposite.p0);
	an_assert(_opposite.p3 == _revAlongV.p0);

	Vector3 p[10];
	p[0] = _alongU.p0;
	p[1] = _alongU.p1;
	p[2] = _alongU.p2;
	p[3] = _alongU.p3;

	p[4] = _revAlongV.p2;
	// use sqrt(2.0) to maintain curvature
	float const coef = sqrt(2.0f);
	p[5] = (   (_revAlongV.p0	+ ((_revAlongV.p1	- _revAlongV.p0)	+ (_opposite.p2		- _opposite.p3))	* coef)
			 + (_alongU.p0		+ ((_alongU.p1		- _alongU.p0)		+ (_revAlongV.p2	- _revAlongV.p3))	* coef)
			 + (_opposite.p0	+ ((_opposite.p1	- _opposite.p0)		+ (_alongU.p2		- _alongU.p3))		* coef)
		   ) * 0.33333333333f;
	p[6] = _opposite.p1;
	
	p[7] = _revAlongV.p1;
	p[8] = _opposite.p2;

	p[9] = _revAlongV.p0;

	return BezierSurface(BezierSurfaceType::Triangle, 2, p);
}

BezierSurface BezierSurface::create(BezierCurve<Vector3> const & _u01v0, BezierCurve<Vector3> const & _u1v01, BezierCurve<Vector3> const & _u10v1, BezierCurve<Vector3> const & _u0v10, BezierSurfaceCreationMethod::Type _method)
{
	/*
	 *	? - calculated
	 *
	 *	_u01v0.p3									_u1v01.p3
	 *	_u1v01.p0	   _u1v01.p1	 _u1v01.p2		_u10v1.p0
	 *			  *			 *			*		  *
	 *
	 *	_u01v0.p2 *			 A?			B?		  * _u10v1.p1
	 *
	 *	_u01v0.p1 *			 C?			D?		  * _u10v1.p2
	 *
	 *			  *			 *			*		  *
	 *	_u01v0.p0	   _u0v10.p2	 _u0v10.p1		_u10v1.p3
	 *	_u0v10.p3	   								_u0v10.p0
	 *
	 *
	 *	each point (A, B, C, D)
	 *	we calculate by weighting pairs
	 *		(1) along U dir
	 *		(2) along V dir
	 *	both pairs are:
	 *			2/3 (control point - vertex) closer to what we are calculating
	 *			1/3 (control point - vertex) further to what we are calculating	
	 *	added to starting point (it can be vertex of control point if we use method aligning to U or V
	 */

	an_assert(_u01v0.p3 == _u1v01.p0);
	an_assert(_u10v1.p0 == _u1v01.p3);
	an_assert(_u10v1.p3 == _u0v10.p0);
	an_assert(_u01v0.p0 == _u0v10.p3);

	Vector3 A, B, C, D;

	// calculate differences for closer points
	Vector3 Au = _u01v0.p2 - _u01v0.p3;
	Vector3 Av = _u1v01.p1 - _u1v01.p0;

	Vector3 Bu = _u10v1.p1 - _u10v1.p0;
	Vector3 Bv = _u1v01.p2 - _u1v01.p3;

	Vector3 Cu = _u01v0.p1 - _u01v0.p0;
	Vector3 Cv = _u0v10.p2 - _u0v10.p3;

	Vector3 Du = _u10v1.p2 - _u10v1.p3;
	Vector3 Dv = _u0v10.p1 - _u0v10.p0;

	// weight closer, weight further
	//float wC = 2.0f / 3.0f;
	//float wF = 1.0f - wC;
	float const coef = sqrt(2.0f);
	float wC = coef * 0.5f;
	float wF = 1.0f - wC;

	if (_method == BezierSurfaceCreationMethod::AlongU)
	{
		A = _u1v01.p1 + Au * wC + Bu * wF;
		B = _u1v01.p2 + Au * wF + Bu * wC;

		C = _u0v10.p2 + Cu * wC + Du * wF;
		D = _u0v10.p1 + Cu * wF + Du * wC;
	}
	else if (_method == BezierSurfaceCreationMethod::AlongV)
	{
		A = _u01v0.p2 + Av * wC + Bv * wF;
		B = _u10v1.p1 + Av * wF + Bv * wC;

		C = _u01v0.p1 + Cv * wC + Dv * wF;
		D = _u10v1.p2 + Cv * wF + Dv * wC;
	}
	else // roundly
	{
		A = (_u1v01.p1 + Au * wC + Bu * wF
		   + _u01v0.p2 + Av * wC + Bv * wF) * 0.5f;
		B = (_u1v01.p2 + Au * wF + Bu * wC
		   + _u10v1.p1 + Av * wF + Bv * wC) * 0.5f;

		C = (_u0v10.p2 + Cu * wC + Du * wF
		   + _u01v0.p1 + Cv * wC + Dv * wF) * 0.5f;
		D = (_u10v1.p2 + Cv * wF + Dv * wC
		   + _u0v10.p1 + Cu * wF + Du * wC) * 0.5f;
    }

	Vector3 p[16];
	p[0] = _u01v0.p0;
	p[1] = _u01v0.p1;
	p[2] = _u01v0.p2;
	p[3] = _u01v0.p3;

	p[4] = _u0v10.p2;
	p[5] = C;
	p[6] = A;
	p[7] = _u1v01.p1;
	
	p[8] = _u0v10.p1;
	p[9] = D;
	p[10] = B;
	p[11] = _u1v01.p2;

	p[12] = _u10v1.p3;
	p[13] = _u10v1.p2;
	p[14] = _u10v1.p1;
	p[15] = _u10v1.p0;

	return BezierSurface(BezierSurfaceType::Quad, 2, p);
}

Vector2 BezierSurface::get_centre_uv() const
{
	if (vertexCount == 2)
	{
		return Vector2(0.5f, 0.0f);
	}
	if (vertexCount == 3)
	{
		todo_note(TXT("are you sure?"));
		return Vector2(0.5f * 0.66667f, 0.5f * 0.66667f);
	}
	if (vertexCount == 4)
	{
		return Vector2(0.5f, 0.5f);
	}
	return Vector2::zero;
}

bool BezierSurface::get_uv_for_border(BezierCurve<Vector3> const & _forCurve, OUT_ Vector2 & _outStart, OUT_ Vector2 & _outEnd) const
{
	// yeah, hardcoded for 2 control points
	if (controlPointCount != 2)
	{
		return false;
	}
	if (vertexCount == 2 ||
		vertexCount == 3 ||
		vertexCount == 4)
	{
		for (int edge = 0; edge < vertexCount; ++edge)
		{
			Vector3 usePoints[4];
			Vector2 startUV;
			Vector2 endUV;
			if (edge == 0)
			{
				startUV = Vector2(0.0f, 0.0f);
				endUV = Vector2(1.0f, 0.0f);
				usePoints[0] = points[0];
				usePoints[1] = points[1];
				usePoints[2] = points[2];
				usePoints[3] = points[3];
			}
			else if (vertexCount == 3)
			{
				if (edge == 1)
				{
					startUV = Vector2(1.0f, 0.0f);
					endUV = Vector2(0.0f, 1.0f);
					usePoints[0] = points[3];
					usePoints[1] = points[6];
					usePoints[2] = points[8];
					usePoints[3] = points[9];
				}
				else
				{
					startUV = Vector2(0.0f, 1.0f);
					endUV = Vector2(0.0f, 0.0f);
					usePoints[0] = points[9];
					usePoints[1] = points[7];
					usePoints[2] = points[4];
					usePoints[3] = points[0];
				}
			}
			else
			{
				if (edge == 1)
				{
					startUV = Vector2(1.0f, 0.0f);
					endUV = Vector2(1.0f, 1.0f);
					usePoints[0] = points[3];
					usePoints[1] = points[7];
					usePoints[2] = points[11];
					usePoints[3] = points[15];
				}
				else if (edge == 2)
				{
					startUV = Vector2(1.0f, 1.0f);
					endUV = Vector2(0.0f, 1.0f);
					usePoints[0] = points[15];
					usePoints[1] = points[14];
					usePoints[2] = points[13];
					usePoints[3] = points[12];
				}
				else
				{
					startUV = Vector2(0.0f, 1.0f);
					endUV = Vector2(0.0f, 0.0f);
					usePoints[0] = points[12];
					usePoints[1] = points[8];
					usePoints[2] = points[4];
					usePoints[3] = points[0];
				}
			}
			if (usePoints[0] == _forCurve.p0 &&
				usePoints[1] == _forCurve.p1 &&
				usePoints[2] == _forCurve.p2 &&
				usePoints[3] == _forCurve.p3)
			{
				_outStart = startUV;
				_outEnd = endUV;
				return true;
			}
			if (usePoints[3] == _forCurve.p0 &&
				usePoints[2] == _forCurve.p1 &&
				usePoints[1] == _forCurve.p2 &&
				usePoints[0] == _forCurve.p3)
			{
				_outStart = endUV;
				_outEnd = startUV;
				return true;
			}
		}
	}
	return false;
}

