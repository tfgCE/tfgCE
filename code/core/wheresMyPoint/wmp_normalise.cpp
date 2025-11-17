#include "wmp_normalise.h"

#include "..\math\math.h"

using namespace WheresMyPoint;

bool Normalise::update(REF_ Value & _value, Context & _context) const
{
	if (_value.get_type() != type_id<Vector3>())
	{
		error_processing_wmp(TXT("can't normalise something else than vector3"));
		return false;
	}

	_value.set(_value.get_as<Vector3>().normal());

	return true;
}
