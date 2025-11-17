#include "wmp_length.h"

#include "..\math\math.h"

//

using namespace WheresMyPoint;

//

bool Length::update(REF_ Value & _value, Context & _context) const
{
	if (_value.get_type() == type_id<float>())
	{
		_value.set(length_of(_value.get_as<float>()));
		return true;
	}

	if (_value.get_type() == type_id<Vector2>())
	{
		_value.set(length_of(_value.get_as<Vector2>()));
		return true;
	}

	if (_value.get_type() == type_id<Vector3>())
	{
		_value.set(length_of(_value.get_as<Vector3>()));
		return true;
	}

	if (_value.get_type() == type_id<Range>())
	{
		_value.set(length_of(_value.get_as<Range>()));
		return true;
	}

	if (_value.get_type() == type_id<Range2>())
	{
		_value.set(_value.get_as<Range2>().length());
		return true;
	}

	error_processing_wmp(TXT("can't calculate length of %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}

//

bool Length2D::update(REF_ Value& _value, Context& _context) const
{
	if (_value.get_type() == type_id<Vector2>())
	{
		_value.set(length_of(_value.get_as<Vector2>()));
		return true;
	}

	if (_value.get_type() == type_id<Vector3>())
	{
		_value.set(length_of(_value.get_as<Vector3>() * Vector3::xy));
		return true;
	}

	error_processing_wmp(TXT("can't calculate length 2d of %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}
