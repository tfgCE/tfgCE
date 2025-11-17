#include "math.h"

Segment::Segment()
{
}

Segment::Segment(Vector3 const & _start, Vector3 const & _end)
: startT(0.0f)
, endT(1.0f)
{
	update(_start, _end);
}

bool Segment::check_against_triangle(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, OUT_ Vector3 & _hitNormal)
{
	Plane abc = Plane(_a, _b, _c);
	if (!abc.is_valid())
	{
		return false;
	}
	Segment test = *this;
	Vector3 testNormal;
	if (abc.check_segment(REF_ test, REF_ testNormal, nullptr /* no context */) &&
		test.get_end_t() <= get_end_t())
	{
		Range3 r = Range3::empty;
		r.include(_a);
		r.include(_b);
		r.include(_c);
		Vector3 hitLoc = test.get_hit();
		r.expand_by(Vector3(0.0001f, 0.0001f, 0.0001f));
		if (!r.does_contain(hitLoc))
		{
			//warn(TXT("ray cast result beyond valid range"));
			return false;
		}

		// pairs of sides
		Vector3 a2b = (_b - _a);
		Vector3 a2c = (_c - _a);
		Vector3 b2a = (_a - _b);
		Vector3 b2c = (_c - _b);
		Vector3 c2a = (_a - _c);
		Vector3 c2b = (_b - _c);

		// normals - pointing inwards
		Vector3 nab = Vector3::cross(Vector3::cross(a2b, a2c), a2b).normal();
		Vector3 nbc = Vector3::cross(Vector3::cross(b2c, b2a), b2c).normal();
		Vector3 nca = Vector3::cross(Vector3::cross(c2a, c2b), c2a).normal();

		float const tre = -0.00001f;
		if (Vector3::dot(hitLoc - _a, nab) >= tre &&
			Vector3::dot(hitLoc - _b, nbc) >= tre &&
			Vector3::dot(hitLoc - _c, nca) >= tre)
		{
			collide_at(test.get_end_t());
			_hitNormal = testNormal;
			return true;
		}
	}

	return false;
}

bool Segment::check_against_range3(Range3 const& _box)
{
	Plane sides[] = { Plane(-Vector3::xAxis, Vector3(_box.x.min, _box.y.min, _box.z.min) ),
					  Plane(-Vector3::yAxis, Vector3(_box.x.min, _box.y.min, _box.z.min) ),
					  Plane(-Vector3::zAxis, Vector3(_box.x.min, _box.y.min, _box.z.min) ),
					  Plane( Vector3::xAxis, Vector3(_box.x.max, _box.y.max, _box.z.max) ),
					  Plane( Vector3::yAxis, Vector3(_box.x.max, _box.y.max, _box.z.max) ),
					  Plane( Vector3::zAxis, Vector3(_box.x.max, _box.y.max, _box.z.max) ) };
	bool collided = false;
	Range3 boxExpanded = _box;
	boxExpanded.expand_by(Vector3(0.0001f, 0.0001f, 0.0001f));
	for_count(int, sideIdx, 6)
	{
		Segment test = *this;
		Vector3 testNormal;
		auto& side = sides[sideIdx];
		if (side.check_segment(REF_ test, REF_ testNormal, nullptr /* no context */) &&
			test.get_end_t() <= get_end_t() &&
			boxExpanded.does_contain(test.get_at_end_t()))
		{
			collided = true;
			*this = test;
		}
	}

	return collided;
}

bool Segment::get_inside(Range3 const& _box)
{
	Plane sides[] = { Plane(-Vector3::xAxis, Vector3(_box.x.min, _box.y.min, _box.z.min) ),
					  Plane(-Vector3::yAxis, Vector3(_box.x.min, _box.y.min, _box.z.min) ),
					  Plane(-Vector3::zAxis, Vector3(_box.x.min, _box.y.min, _box.z.min) ),
					  Plane( Vector3::xAxis, Vector3(_box.x.max, _box.y.max, _box.z.max) ),
					  Plane( Vector3::yAxis, Vector3(_box.x.max, _box.y.max, _box.z.max) ),
					  Plane( Vector3::zAxis, Vector3(_box.x.max, _box.y.max, _box.z.max) ) };
	Range3 boxExpanded = _box;
	boxExpanded.expand_by(Vector3(0.0001f, 0.0001f, 0.0001f));
	for_count(int, sideIdx, 6)
	{
		auto& side = sides[sideIdx];

		float sInFront = side.get_in_front(get_at_start_t());
		float eInFront = side.get_in_front(get_at_end_t());
		if (sInFront > 0.0f && eInFront > 0.0f)
		{
			// both outside this plane, we should be empty, ignore whole segment
			return false;
		}
		if (sInFront <= 0.0f && eInFront <= 0.0f)
		{
			// both inside, keep as it is
			continue;
		}

		if (sInFront == eInFront)
		{
			// parallel
			continue;
		}

		float s2eT = (0.0f - sInFront) / (eInFront - sInFront);

		if (sInFront > 0.0f)
		{
			// cut start, it is outside
			startT = lerp(s2eT, startT, endT);
			boundingBoxCalculated = false;
		}
		else if (eInFront > 0.0f)
		{
			// cut end, it is outside
			endT = lerp(s2eT, startT, endT);
			boundingBoxCalculated = false;
		}
	}

	return true;
}
