#include "plane.h"

String Plane::to_string() const
{
	return String::printf(TXT("norm:(%S) anchor:(%S)"), normal.to_string().to_char(), anchor.to_string().to_char());
}

void Plane::set(Vector3 const & _normal, Vector3 const & _point)
{
	normal = _normal.normal();
	anchor = _point;
}

void Plane::set(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c)
{
	Vector3 ab = _b - _a;
	Vector3 ac = _c - _a;
	normal = Vector3::cross(ac, ab).normal();
	anchor = _a;
}

Vector4 Plane::to_vector4_for_rendering() const
{
	return Vector4(normal.x, normal.y, normal.z, calculate_d());
}
