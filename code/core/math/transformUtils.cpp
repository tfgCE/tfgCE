#include "math.h"

#include "..\random\random.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

Transform offset_transform_by_forward_angle(Transform const & _placement, REF_ Random::Generator& _rg, float _preciseAngle)
{
	// treat angle as maximal it can be off on y axis
	float angle = clamp(_preciseAngle, 0.0f, 360.0f);
	Vector3 dir = Vector3::zero;
	dir.y = 0.0f;
	dir.x = _rg.get_float(-1.0f, 1.0f);
	dir.z = _rg.get_float(-1.0f, 1.0f);
	float cosAngle = cos_deg(angle);
	dir.normalise();
	dir *= sqrt(max(0.0f, 1.0f - sqr(cosAngle)));
	dir.y = cosAngle;

	Transform offset;
	if (abs(dir.z) < 0.95f)
	{
		offset = look_matrix(Vector3::zero, dir, Vector3::zAxis).to_transform();
	}
	else
	{
		offset = look_matrix_with_right(Vector3::zero, dir, Vector3::xAxis).to_transform();
	}
	return _placement.to_world(offset);
}

Transform offset_transform_by_forward_angle(Transform const & _placement, REF_ Random::Generator& _rg, Range const & _angleRange)
{
	// treat angle as maximal it can be off on y axis
	float angle = clamp(_rg.get(_angleRange), 0.0f, 360.0f);

	return offset_transform_by_forward_angle(_placement, _rg, angle);
}
