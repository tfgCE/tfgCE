#pragma once

#ifndef INCLUDED_MATH_H
#error include math.h first
#endif

struct Segment2
{
public:
	Segment2();
	Segment2(Vector2 const & _start, Vector2 const & _end);

	inline void update(Vector2 const & _start, Vector2 const & _end);

	inline Vector2 const & get_start() const { return start; }
	inline Vector2 const & get_end() const { return end; }
	inline float length() const { return startToEndLength; }
	inline Vector2 get_right_dir() const { return startToEndDir.rotated_right(); }
	inline Vector2 const & get_start_to_end_dir() const { return startToEndDir; }
	inline bool is_on_right_side(Vector2 const & _point, float _epsilon = EPSILON) const { return Vector2::dot(get_right_dir(), _point - start) > -_epsilon; }
	inline bool is_on_left_side(Vector2 const & _point, float _epsilon = EPSILON) const { return Vector2::dot(get_right_dir(), _point - start) < _epsilon; }
	inline Vector2 calculate_centre() const { return (start + end) * 0.5f; }

	inline Vector2 get_at_t(float _t) const { return (start + startToEnd * _t); }

	inline bool does_intersect_with(Segment2 const & _other, float _epsilon = EPSILON) const; // intersect! which means that if they touch, it's fine
	inline Optional<float> calc_intersect_with(Segment2 const & _other, float _epsilon = EPSILON) const; // if intersects, returns where intersects
	inline float calculate_distance_from(Vector2 const & _point) const;
	inline float calculate_distance_from(Segment2 const & _other) const;

private:
	Vector2 start;
	Vector2 end;
	CACHED_ Vector2 startToEnd;
	CACHED_ Vector2 startToEndDir;
	CACHED_ float startToEndLength;
};
