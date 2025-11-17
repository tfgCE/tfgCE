#pragma once

#ifndef INCLUDED_MATH_H
#error include math.h first
#endif

struct Segment
{
public:
	Segment();
	Segment(Vector3 const & _start, Vector3 const & _end);

	inline bool is_point() const { return start == end; }

	inline float length() const { return startToEndLength; }

	inline void set_start_end(float _startT, float _endT);
	inline void set_end(float _endT);

	inline bool collide_at(float _t);
	inline bool would_collide_at(float _t) const;

	inline Segment& operator=(Segment const & _segment);

	inline void copy_t_from(Segment const & _segment);

	inline void update(Vector3 const & _start, Vector3 const & _end); // updates only start and end

	inline float get_t(Vector3 const & _a) const; // will be clamped
	inline float get_t_not_clamped(Vector3 const & _a) const;

	inline Vector3 get_at_t(float _t) const;
	inline Vector3 get_hit() const;

	inline Vector3 const & get_start() const { return start; }
	inline Vector3 const & get_end() const { return end; }
	inline float get_start_t() const { return startT; }
	inline float get_end_t() const { return endT; }
	inline Vector3 get_at_start_t() const { return get_at_t(startT); }
	inline Vector3 get_at_end_t() const { return get_at_t(endT); }
	inline Vector3 const & get_start_to_end() const { return startToEnd; }
	inline Vector3 const & get_start_to_end_dir() const { return startToEndDir; }

	inline float get_distance(Vector3 const & _p) const;
	inline float get_distance(Vector3 const & _p, OUT_ float & _t) const;
	inline float get_distance(Segment const & _s) const;
	inline float get_closest_t(Segment const & _s) const;

	inline Range3 const & get_bounding_box() { update_bounding_box(); return boundingBox; }

	bool check_against_triangle(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, OUT_ Vector3 & _hitNormal);
	bool check_against_range3(Range3 const & _box);

	bool get_inside(Range3 const & _box); // returns false if outside

private:
	Vector3 start;
	Vector3 end;
	float startT; // starting T (initially 0)
	float endT; // ending T (initially 1)
	CACHED_ Vector3 startToEnd;
	CACHED_ Vector3 startToEndDir;
	CACHED_ float startToEndLength;
	CACHED_ Range3 boundingBox;
	CACHED_ bool boundingBoxCalculated = false;

	friend struct Matrix44;
	friend struct Transform;

	inline void update_bounding_box();

	inline void get_closest_points_ts(Segment const& _s, OUT_ float& _thisT, OUT_ float& _sT) const;
};
