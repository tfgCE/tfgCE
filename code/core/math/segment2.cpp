#include "math.h"

Segment2::Segment2()
{
}

Segment2::Segment2(Vector2 const & _start, Vector2 const & _end)
{
	update(_start, _end);
}
