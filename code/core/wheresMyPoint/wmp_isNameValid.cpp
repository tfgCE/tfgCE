#include "wmp_isNameValid.h"

using namespace WheresMyPoint;

bool IsNameValid::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	if (_value.get_type() == type_id<Name>())
	{
		_value.set<bool>(_value.get_as<Name>().is_valid());
	}
	else
	{
		error_processing_wmp(TXT("not a name!"));
		result = false;
	}

	return result;
}

//
