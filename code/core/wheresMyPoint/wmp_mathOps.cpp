#include "wmp_mathOps.h"

#include "..\io\xml.h"
#include "..\math\math.h"
#include "..\other\parserUtils.h"
#include "..\tags\tag.h"
#include "..\tags\tagCondition.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace WheresMyPoint;

//

bool MathOp::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= toolSet.load_from_xml(_node);

	floatValuePresent = !_node->has_children() && !_node->get_internal_text().trim().is_empty();
	floatValue = ParserUtils::parse_float(_node->get_internal_text(), floatValue);

	return result;
}

bool MathOp::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	Value resultValue = _value;
	Value valueToOp;
	if (floatValuePresent)
	{
		valueToOp.set<float>(floatValue);
	}
	else
	{
		valueToOp = _value;
	}
	if (toolSet.update(valueToOp, _context))
	{
		if (! resultValue.is_set())
		{
			// just copy
			resultValue = valueToOp;
		}
		else
		{
			result &= handle_operation(resultValue, valueToOp, toolSet.is_empty());
		}
	}
	else
	{
		result = false;
	}

	_value = resultValue;

	return result;
}

//

bool Add::load_from_xml(IO::XML::Node const* _node)
{
	bool result = MathOp::load_from_xml(_node);

	addString = _node->get_string_attribute(TXT("string"), addString);

	return result;
}

bool Add::update(REF_ Value& _value, Context& _context) const
{
	bool result = true;

	if (! addString.is_empty())
	{
		_value.set<::Name>(::Name(_value.get_as<::Name>().to_string() + addString));
	}
	else
	{
		MathOp::update(_value, _context);
	}

	return result;
}

bool Add::handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const
{
	if (_value.get_type() == type_id<::Name>())
	{
		if (_valueToOp.get_type() == type_id<int>())
		{
			_value.set<::Name>(::Name(_value.get_as<::Name>().to_string() + ::String::printf(TXT("%i"), _valueToOp.get_as<int>())));
		}
		else if (_valueToOp.get_type() == type_id<Name>())
		{
			_value.set<::Name>(::Name(_value.get_as<::Name>().to_string() + _valueToOp.get_as<Name>().to_string()));
		}
		else
		{
			todo_important(TXT("add class handling for where's my point \"add\" -> \"%S\" div \"%S\""),
				RegisteredType::get_name_of(_value.get_type()), RegisteredType::get_name_of(_valueToOp.get_type()));
			return false;
		}
	}
	else
	{
		if (_value.get_type() == type_id<Range>() &&
			_valueToOp.get_type() == type_id<float>())
		{
			_value.access_as<Range>() += _valueToOp.get_as<float>();
			return true;
		}
		else
		{
			if (_value.get_type() != _valueToOp.get_type())
			{
				error_processing_wmp(TXT("type mismatch for addition! adding %S to %S"), RegisteredType::get_name_of(_valueToOp.get_type()), RegisteredType::get_name_of(_value.get_type()));
				return false;
			}

			if (_value.get_type() == type_id<int>())
			{
				_value.set<int>(_value.get_as<int>() + _valueToOp.get_as<int>());
			}
			else if (_value.get_type() == type_id<float>())
			{
				_value.set<float>(_value.get_as<float>() + _valueToOp.get_as<float>());
			}
			else if (_value.get_type() == type_id<Vector2>())
			{
				_value.set<Vector2>(_value.get_as<Vector2>() + _valueToOp.get_as<Vector2>());
			}
			else if (_value.get_type() == type_id<Vector3>())
			{
				_value.set<Vector3>(_value.get_as<Vector3>() + _valueToOp.get_as<Vector3>());
			}
			else if (_value.get_type() == type_id<::Name>())
			{
				_value.set<::Name>(::Name(_value.get_as<::Name>().to_string() + _valueToOp.get_as<::Name>().to_string()));
			}
			else
			{
				todo_important(TXT("add class handling for where's my point \"add\" -> \"%S\" div \"%S\""),
					RegisteredType::get_name_of(_value.get_type()), RegisteredType::get_name_of(_valueToOp.get_type()));
				return false;
			}
		}
	}

	return true;
}

//

bool Sub::handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const
{
	if (_value.get_type() != _valueToOp.get_type())
	{
		error_processing_wmp(TXT("type mismatch for subtraction! subtracting %S from %S"), RegisteredType::get_name_of(_valueToOp.get_type()), RegisteredType::get_name_of(_value.get_type()));
		return false;
	}

	if (_value.get_type() == type_id<int>())
	{
		_value.set<int>(_value.get_as<int>() - _valueToOp.get_as<int>());
	}
	else if (_value.get_type() == type_id<float>())
	{
		_value.set<float>(_value.get_as<float>() - _valueToOp.get_as<float>());
	}
	else if (_value.get_type() == type_id<Vector2>())
	{
		_value.set<Vector2>(_value.get_as<Vector2>() - _valueToOp.get_as<Vector2>());
	}
	else if (_value.get_type() == type_id<Vector3>())
	{
		_value.set<Vector3>(_value.get_as<Vector3>() - _valueToOp.get_as<Vector3>());
	}
	else if (_value.get_type() == type_id<Range>())
	{
		_value.set<Range>(_value.get_as<Range>() - _valueToOp.get_as<Range>());
	}
	else
	{
		todo_important(TXT("add class handling for where's my point \"sub\" -> \"%S\" div \"%S\""),
			RegisteredType::get_name_of(_value.get_type()), RegisteredType::get_name_of(_valueToOp.get_type()));
		return false;
	}

	return true;
}

//

bool Mul::handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const
{
	if (_value.get_type() != _valueToOp.get_type())
	{
		if (_value.get_type() == type_id<float>() &&
			_valueToOp.get_type() == type_id<int>())
		{
			_value.set<float>(_value.get_as<float>() * (float)_valueToOp.get_as<int>());
			return true;
		}
		if (_value.get_type() == type_id<float>() &&
			_valueToOp.get_type() == type_id<Vector3>())
		{
			_value.set<Vector3>(_value.get_as<float>() * _valueToOp.get_as<Vector3>());
			return true;
		}
		if (_value.get_type() == type_id<Vector3>() &&
			_valueToOp.get_type() == type_id<float>())
		{
			_value.set<Vector3>(_value.get_as<Vector3>() * _valueToOp.get_as<float>());
			return true;
		}
		if (_value.get_type() == type_id<Vector3>() &&
			_valueToOp.get_type() == type_id<Vector2>())
		{
			_value.set<Vector3>(_value.get_as<Vector3>() * _valueToOp.get_as<Vector2>().to_vector3(1.0f));
			return true;
		}
		if (_value.get_type() == type_id<float>() &&
			_valueToOp.get_type() == type_id<Vector2>())
		{
			_value.set<Vector2>(_value.get_as<float>() * _valueToOp.get_as<Vector2>());
			return true;
		}
		if (_value.get_type() == type_id<Vector2>() &&
			_valueToOp.get_type() == type_id<float>())
		{
			_value.set<Vector2>(_value.get_as<Vector2>() * _valueToOp.get_as<float>());
			return true;
		}
		if (_value.get_type() == type_id<Vector2>() &&
			_valueToOp.get_type() == type_id<VectorInt2>())
		{
			_value.set<Vector2>(_value.get_as<Vector2>() * _valueToOp.get_as<VectorInt2>().to_vector2());
			return true;
		}
		if (_value.get_type() == type_id<float>() &&
			_valueToOp.get_type() == type_id<Rotator3>())
		{
			_value.set<Rotator3>(_value.get_as<float>() * _valueToOp.get_as<Rotator3>());
			return true;
		}
		if (_value.get_type() == type_id<Rotator3>() &&
			_valueToOp.get_type() == type_id<float>())
		{
			_value.set<Rotator3>(_value.get_as<Rotator3>() * _valueToOp.get_as<float>());
			return true;
		}
		if (_value.get_type() == type_id<Range>() &&
			_valueToOp.get_type() == type_id<float>())
		{
			_value.access_as<Range>() *= _valueToOp.get_as<float>();
			return true;
		}
		if (_value.get_type() == type_id<Range2>() &&
			_valueToOp.get_type() == type_id<float>())
		{
			_value.access_as<Range2>() *= Vector2::one * _valueToOp.get_as<float>();
			return true;
		}
		if (_value.get_type() == type_id<Range2>() &&
			_valueToOp.get_type() == type_id<Vector2>())
		{
			_value.access_as<Range2>() *= _valueToOp.get_as<Vector2>();
			return true;
		}
	}
	else
	{
		if (_value.get_type() == type_id<int>())
		{
			_value.set<int>(_value.get_as<int>() * _valueToOp.get_as<int>());
			return true;
		}
		if (_value.get_type() == type_id<float>())
		{
			_value.set<float>(_value.get_as<float>() * _valueToOp.get_as<float>());
			return true;
		}
		if (_value.get_type() == type_id<Vector2>())
		{
			_value.set<Vector2>(_value.get_as<Vector2>() * _valueToOp.get_as<Vector2>());
			return true;
		}
		if (_value.get_type() == type_id<Vector3>())
		{
			_value.set<Vector3>(_value.get_as<Vector3>() * _valueToOp.get_as<Vector3>());
			return true;
		}
	}
	todo_important(TXT("add class handling for where's my point \"mul\" -> \"%S\" div \"%S\""),
		RegisteredType::get_name_of(_value.get_type()), RegisteredType::get_name_of(_valueToOp.get_type()));
	return false;
}

//

bool Div::handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const
{
	float valueToOpFloat = 0.0f;
	int valueToOpInt = 0;
	// get value to op
	if (_valueToOp.get_type() == type_id<float>())
	{
		valueToOpFloat = _valueToOp.get_as<float>();
		if (valueToOpFloat == 0.0f)
		{
			error_processing_wmp(TXT("division by zero!"));
			return false;
		}
	}
	if (_valueToOp.get_type() == type_id<int>())
	{
		valueToOpInt = _valueToOp.get_as<int>();
		if (valueToOpInt == 0.0f)
		{
			error_processing_wmp(TXT("division by zero!"));
			return false;
		}
	}
	if (_value.get_type() != _valueToOp.get_type())
	{
		if (_value.get_type() == type_id<Vector2>() &&
			_valueToOp.get_type() == type_id<float>())
		{
			_value.set<Vector2>(_value.get_as<Vector2>() * (1.0f / valueToOpFloat));
			return true;
		}
		if (_value.get_type() == type_id<Vector3>() &&
			_valueToOp.get_type() == type_id<float>())
		{
			_value.set<Vector3>(_value.get_as<Vector3>() * (1.0f / valueToOpFloat));
			return true;
		}
	}
	else
	{
		if (_value.get_type() == type_id<float>() &&
			_valueToOp.get_type() == type_id<float>())
		{
			_value.set<float>(_value.get_as<float>() / valueToOpFloat);
			return true;
		}
		if (_value.get_type() == type_id<int>() &&
			_valueToOp.get_type() == type_id<int>())
		{
			_value.set<int>(_value.get_as<int>() / valueToOpInt);
			return true;
		}
		if (_value.get_type() == type_id<Vector2>() &&
			_valueToOp.get_type() == type_id<Vector2>())
		{
			_value.set<Vector2>(_value.get_as<Vector2>() / _valueToOp.get_as<Vector2>());
			return true;
		}
		if (_value.get_type() == type_id<Vector3>() &&
			_valueToOp.get_type() == type_id<Vector3>())
		{
			_value.set<Vector3>(_value.get_as<Vector3>() / _valueToOp.get_as<Vector3>());
			return true;
		}
	}
	todo_important(TXT("add class handling for where's my point \"div\" -> \"%S\" div \"%S\""),
		RegisteredType::get_name_of(_value.get_type()), RegisteredType::get_name_of(_valueToOp.get_type()));
	return false;
}

//

bool Max::handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const
{
	if (_valueToOp.get_type() == type_id<float>())
	{
		_value.set<float>(max(_value.get_as<float>(), _valueToOp.get_as<float>()));
		return true;
	}
	if (_valueToOp.get_type() == type_id<int>())
	{
		_value.set<int>(max(_value.get_as<int>(), _valueToOp.get_as<int>()));
		return true;
	}
	todo_important(TXT("add class handling for where's my point \"max\" -> \"%S\""),
		RegisteredType::get_name_of(_valueToOp.get_type()));
	return false;
}

//

bool Min::handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const
{
	if (_valueToOp.get_type() == type_id<float>())
	{
		_value.set<float>(min(_value.get_as<float>(), _valueToOp.get_as<float>()));
		return true;
	}
	if (_valueToOp.get_type() == type_id<int>())
	{
		_value.set<int>(min(_value.get_as<int>(), _valueToOp.get_as<int>()));
		return true;
	}
	todo_important(TXT("add class handling for where's my point \"min\" -> \"%S\""),
		RegisteredType::get_name_of(_valueToOp.get_type()));
	return false;
}

//

bool Mod::handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const
{
	if (_valueToOp.get_type() == type_id<float>())
	{
		_value.set<float>(mod(_value.get_as<float>(), _valueToOp.get_as<float>()));
		return true;
	}
	if (_valueToOp.get_type() == type_id<int>())
	{
		_value.set<int>(mod(_value.get_as<int>(), _valueToOp.get_as<int>()));
		return true;
	}
	todo_important(TXT("add class handling for where's my point \"mod\" -> \"%S\" div \"%S\""),
		RegisteredType::get_name_of(_value.get_type()), RegisteredType::get_name_of(_valueToOp.get_type()));
	return false;
}

//

bool Round::handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const
{
	if (_valueToOp.get_type() == type_id<float>())
	{
		float roundBy = _valueToOpDefaulted? 1.0f : _valueToOp.get_as<float>();
		float value = _value.get_as<float>();
		if (roundBy == 0.0f || roundBy == 1.0f)
		{
			value = round(value);
		}
		else
		{
			value = round(value / roundBy) * roundBy;
		}
		_value.set(value);
		return true;
	}

	error_processing_wmp(TXT("can't get round from value %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}

//

bool Tan::handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const
{
	{
		float angle;
		if (_valueToOp.is_set())
		{
			angle = _valueToOp.get_as<float>();
		}
		else
		{
			angle = _value.get_as<float>();
		}
		_value.set(tan_deg(angle));
		return true;
	}

	error_processing_wmp(TXT("can't get tan from value %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}

//

bool Sin::handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const
{
	{
		float angle;
		if (_valueToOp.is_set())
		{
			angle = _valueToOp.get_as<float>();
		}
		else
		{
			angle = _value.get_as<float>();
		}
		_value.set(sin_deg(angle));
		return true;
	}

	error_processing_wmp(TXT("can't get sin from value %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}

//

bool Cos::handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const
{
	{
		float angle;
		if (_valueToOp.is_set())
		{
			angle = _valueToOp.get_as<float>();
		}
		else
		{
			angle = _value.get_as<float>();
		}
		_value.set(cos_deg(angle));
		return true;
	}

	error_processing_wmp(TXT("can't get cos from value %S"), RegisteredType::get_name_of(_value.get_type()));
	return false;
}
