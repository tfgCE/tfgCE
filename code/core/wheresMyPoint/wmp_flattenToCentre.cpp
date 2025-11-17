#include "wmp_flattenToCentre.h"

#include "..\containers\arrayStack.h"
#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool FlattenToCentre::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	Value resultValue = _value;
	if (resultValue.get_type() == type_id<Range>())
	{
		auto& r = resultValue.access_as<Range>();
		auto centre = r.centre();
		r = Range(centre);
	}
	else if (resultValue.get_type() == type_id<Range2>())
	{
		auto& r = resultValue.access_as<Range2>();
		auto centre = r.centre();
		r.x = Range(centre.x);
		r.y = Range(centre.y);
	}
	else if (resultValue.get_type() == type_id<Range3>())
	{
		auto& r = resultValue.access_as<Range3>();
		auto centre = r.centre();
		r.x = Range(centre.x);
		r.y = Range(centre.y);
		r.z = Range(centre.z);
	}
	else
	{
		error_processing_wmp_tool(this, TXT("don't know how to handle \"%S\""), RegisteredType::get_name_of(resultValue.get_type()));
		result = false;
		todo_implement;
	}

	_value = resultValue;

	return result;
}

