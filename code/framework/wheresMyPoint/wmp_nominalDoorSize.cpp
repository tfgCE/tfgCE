#include "wmp_nominalDoorSize.h"

#include "..\world\door.h"

using namespace Framework;

//

bool NominalDoorWidth::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	if (Door::get_nominal_door_width().is_set())
	{
		_value.set<float>(Door::get_nominal_door_width().get());
	}
	else
	{
		warn_processing_wmp(TXT("no nominal door width provided, providing fallback"));
		_value.set<float>(0.7f);
	}

	return result;
}

//

bool NominalDoorHeight::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	_value.set<float>(Door::get_nominal_door_height());

	return result;
}

//

bool NominalDoorHeightScale::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	_value.set<float>(Door::get_nominal_door_height() / Door::get_default_nominal_door_height());

	return result;
}

//
