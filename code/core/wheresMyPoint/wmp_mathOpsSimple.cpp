#include "wmp_mathOpsSimple.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\math\math.h"

//

using namespace WheresMyPoint;

//

bool MathOpSimpleOnVar::load_from_xml(IO::XML::Node const* _node)
{
	bool result = base::load_from_xml(_node);

	varName = _node->get_name_attribute(TXT("var"), varName);

	return result;
}

bool MathOpSimpleOnVar::read_var(REF_ Value& _value, Context& _context) const
{
	if (varName.is_valid())
	{
		return _context.get_owner()->restore_value_for_wheres_my_point(varName, _value);
	}
	else
	{
		return true;
	}
}

bool MathOpSimpleOnVar::write_var(REF_ Value& _value, Context& _context) const
{
	if (varName.is_valid())
	{
		return _context.get_owner()->store_value_for_wheres_my_point(varName, _value);
	}
	else
	{
		return true;
	}
}

//

bool Double::update(REF_ Value & _value, Context & _context) const
{
	if (_value.get_type() == type_id<int>())
	{
		_value.set(_value.get_as<int>() * 2);
		return true;
	}

	if (_value.get_type() == type_id<float>())
	{
		_value.set(_value.get_as<float>() * 2.0f);
		return true;
	}

	if (_value.get_type() == type_id<Vector2>())
	{
		_value.set(_value.get_as<Vector2>() * 2.0f);
		return true;
	}

	if (_value.get_type() == type_id<Vector3>())
	{
		_value.set(_value.get_as<Vector3>() * 2.0f);
		return true;
	}

	error_processing_wmp(TXT("can't double %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}

//

bool Half::update(REF_ Value & _value, Context & _context) const
{
	if (_value.get_type() == type_id<float>())
	{
		_value.set(_value.get_as<float>() * 0.5f);
		return true;
	}

	if (_value.get_type() == type_id<int>())
	{
		_value.set(_value.get_as<int>() / 2);
		return true;
	}

	if (_value.get_type() == type_id<Vector2>())
	{
		_value.set(_value.get_as<Vector2>() * 0.5f);
		return true;
	}

	if (_value.get_type() == type_id<Vector3>())
	{
		_value.set(_value.get_as<Vector3>() * 0.5f);
		return true;
	}

	error_processing_wmp(TXT("can't half %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}
//

bool Sqr::update(REF_ Value & _value, Context & _context) const
{
	if (_value.get_type() == type_id<float>())
	{
		float v = _value.get_as<float>();
		_value.set(sqr(v));
		return true;
	}

	error_processing_wmp(TXT("can't sqr %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}

//

bool Sqrt::update(REF_ Value & _value, Context & _context) const
{
	if (_value.get_type() == type_id<float>())
	{
		float v = _value.get_as<float>();
		_value.set(v >= 0.0f? sqrt(v) : -sqrt(-v));
		return true;
	}

	error_processing_wmp(TXT("can't sqrt %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}

//

bool Abs::update(REF_ Value & _value, Context & _context) const
{
	if (_value.get_type() == type_id<float>())
	{
		_value.set(abs(_value.get_as<float>()));
		return true;
	}

	if (_value.get_type() == type_id<int>())
	{
		_value.set(abs(_value.get_as<int>()));
		return true;
	}

	if (_value.get_type() == type_id<Vector2>())
	{
		Vector2 v = _value.get_as<Vector2>();
		v.x = abs(v.x);
		v.y = abs(v.y);
		_value.set(v);
		return true;
	}

	if (_value.get_type() == type_id<Vector3>())
	{
		Vector3 v = _value.get_as<Vector3>();
		v.x = abs(v.x);
		v.y = abs(v.y);
		v.z = abs(v.z);
		_value.set(v);
		return true;
	}

	error_processing_wmp(TXT("can't get absolute value %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}

//

bool Negate::update(REF_ Value & _value, Context & _context) const
{
	if (_value.get_type() == type_id<bool>())
	{
		_value.set(!_value.get_as<bool>());
		return true;
	}

	if (_value.get_type() == type_id<int>())
	{
		_value.set(-_value.get_as<int>());
		return true;
	}

	if (_value.get_type() == type_id<float>())
	{
		_value.set(-_value.get_as<float>());
		return true;
	}

	if (_value.get_type() == type_id<Vector2>())
	{
		_value.set(-_value.get_as<Vector2>());
		return true;
	}

	if (_value.get_type() == type_id<Vector3>())
	{
		_value.set(-_value.get_as<Vector3>());
		return true;
	}

	if (_value.get_type() == type_id<Range>())
	{
		Range r = _value.get_as<Range>();
		_value.set(Range(-r.max, -r.min));
		return true;
	}

	if (_value.get_type() == type_id<Range2>())
	{
		Range2 r = _value.get_as<Range2>();
		_value.set(Range2(Range(-r.x.max, -r.x.min), Range(-r.y.max, -r.y.min)));
		return true;
	}

	error_processing_wmp(TXT("can't negate %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}

//

bool Invert::update(REF_ Value & _value, Context & _context) const
{
	if (_value.get_type() == type_id<bool>())
	{
		_value.set(!_value.get_as<bool>());
		return true;
	}

	if (_value.get_type() == type_id<float>())
	{
		_value.set(1.0f / _value.get_as<float>());
		return true;
	}

	if (_value.get_type() == type_id<Transform>())
	{
		_value.set(_value.get_as<Transform>().inverted());
		return true;
	}

	error_processing_wmp(TXT("can't invert %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}

//

bool Sign::update(REF_ Value & _value, Context & _context) const
{
	if (_value.get_type() == type_id<float>())
	{
		_value.set(sign(_value.get_as<float>()));
		return true;
	}

	if (_value.get_type() == type_id<Vector2>())
	{
		Vector2 v = _value.get_as<Vector2>();
		v.x = sign(v.x);
		v.y = sign(v.y);
		_value.set(v);
		return true;
	}

	if (_value.get_type() == type_id<Vector3>())
	{
		Vector3 v = _value.get_as<Vector3>();
		v.x = sign(v.x);
		v.y = sign(v.y);
		v.z = sign(v.z);
		_value.set(v);
		return true;
	}

	error_processing_wmp(TXT("can't get sign from value %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}

//

bool Increase::update(REF_ Value & _value, Context & _context) const
{
	read_var(_value, _context);

	if (_value.get_type() == type_id<float>())
	{
		_value.set(_value.get_as<float>() + 1.0f);
		write_var(_value, _context);
		return true;
	}

	if (_value.get_type() == type_id<int>())
	{
		_value.set(_value.get_as<int>() + 1);
		write_var(_value, _context);
		return true;
	}

	error_processing_wmp(TXT("can't increase value %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}

//

bool Decrease::update(REF_ Value & _value, Context & _context) const
{
	read_var(_value, _context);

	if (_value.get_type() == type_id<float>())
	{
		_value.set(_value.get_as<float>() - 1.0f);
		write_var(_value, _context);
		return true;
	}

	if (_value.get_type() == type_id<int>())
	{
		_value.set(_value.get_as<int>() - 1);
		write_var(_value, _context);
		return true;
	}

	error_processing_wmp(TXT("can't decrease value %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}

//

bool IsZero::update(REF_ Value& _value, Context& _context) const
{
	if (_value.get_type() == type_id<float>())
	{
		_value.set<bool>(_value.get_as<float>() == 0.0f);
		return true;
	}

	if (_value.get_type() == type_id<int>())
	{
		_value.set<bool>(_value.get_as<int>() == 0);
		return true;
	}

	if (_value.get_type() == type_id<Range>())
	{
		_value.set<bool>(_value.get_as<Range>() == Range(0.0f));
		return true;
	}

	if (_value.get_type() == type_id<Vector2>())
	{
		_value.set<bool>(_value.get_as<Vector2>().is_zero());
		return true;
	}

	if (_value.get_type() == type_id<Vector3>())
	{
		_value.set<bool>(_value.get_as<Vector3>().is_zero());
		return true;
	}

	error_processing_wmp(TXT("can't check if is zero for %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}

//

bool Rotate90Right::update(REF_ Value& _value, Context& _context) const
{
	if (_value.get_type() == type_id<Vector2>())
	{
		_value.set(_value.get_as<Vector2>().rotated_right());
		return true;
	}

	if (_value.get_type() == type_id<Vector3>())
	{
		_value.set(_value.get_as<Vector3>().rotated_right());
		return true;
	}

	if (_value.get_type() == type_id<Range2>())
	{
		Range2 r = _value.get_as<Range2>();
		_value.set(Range2(Range(r.y.min, r.y.max), Range(-r.x.max, -r.x.min)));
		return true;
	}

	error_processing_wmp(TXT("can't rotate 90 right %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}

//

bool Floor::update(REF_ Value& _value, Context& _context) const
{
	if (_value.get_type() == type_id<float>())
	{
		_value.set(floor(_value.get_as<float>()));
		return true;
	}

	error_processing_wmp(TXT("can't floor %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}

//

bool Ceil::update(REF_ Value& _value, Context& _context) const
{
	if (_value.get_type() == type_id<float>())
	{
		_value.set(ceil(_value.get_as<float>()));
		return true;
	}

	error_processing_wmp(TXT("can't ceil %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}

//

