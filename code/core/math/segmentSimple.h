#pragma once

#ifndef INCLUDED_MATH_H
#error include math.h first
#endif

struct SegmentSimple
{
public:
	SegmentSimple();
	SegmentSimple(Vector3 const & _start, Vector3 const & _end);

	inline Vector3 const & get_start() const { return start; }
	inline Vector3 const & get_end() const { return end; }
	inline Vector3 get_half() const { return (start + end) * 0.5f; }

private:
	Vector3 start;
	Vector3 end;

	friend struct Transform;
};
