#include "wmp_mathComparisons.h"

#include "..\io\xml.h"
#include "..\math\math.h"
#include "..\other\parserUtils.h"

using namespace WheresMyPoint;

bool MathComparison::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= toolSet.load_from_xml(_node);

	floatValuePresent = ! _node->has_children() && !_node->get_internal_text().trim().is_empty();
	floatValue = ParserUtils::parse_float(_node->get_internal_text(), floatValue);

	return result;
}

bool MathComparison::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	Value resultValue = _value;
	if (!resultValue.is_set())
	{
		error_processing_wmp(TXT("nothing to compare to"));
		return false;
	}
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
		result &= handle_comparison(resultValue, valueToOp);
	}
	else
	{
		result = false;
	}

	_value = resultValue;

	return result;
}

//

bool EqualTo::handle_comparison(REF_ Value & _value, Value const & _valueToCompare) const
{
	if (_value.get_type() != _valueToCompare.get_type())
	{
		error_processing_wmp(TXT("type mismatch for comparison! %S v %S"), RegisteredType::get_name_of(_value.get_type()), RegisteredType::get_name_of(_valueToCompare.get_type()));
		return false;
	}

	if (_value.get_type() == type_id<int>())
	{
		_value.set<bool>(_value.get_as<int>() == _valueToCompare.get_as<int>());
	}
	else if (_value.get_type() == type_id<float>())
	{
		_value.set<bool>(_value.get_as<float>() == _valueToCompare.get_as<float>());
	}
	else if (_value.get_type() == type_id<Name>())
	{
		_value.set<bool>(_value.get_as<Name>() == _valueToCompare.get_as<Name>());
	}
	else
	{
		todo_important(TXT("add class handling for where's my point \"equalTo\""));
		return false;
	}

	return true;
}

//

bool LessThan::handle_comparison(REF_ Value & _value, Value const & _valueToCompare) const
{
	if (_value.get_type() != _valueToCompare.get_type())
	{
		error_processing_wmp(TXT("type mismatch for comparison! %S v %S"), RegisteredType::get_name_of(_value.get_type()), RegisteredType::get_name_of(_valueToCompare.get_type()));
		return false;
	}

	if (_value.get_type() == type_id<int>())
	{
		_value.set<bool>(_value.get_as<int>() < _valueToCompare.get_as<int>());
	}
	else if (_value.get_type() == type_id<float>())
	{
		_value.set<bool>(_value.get_as<float>() < _valueToCompare.get_as<float>());
	}
	else
	{
		todo_important(TXT("add class handling for where's my point \"lessThan\""));
		return false;
	}

	return true;
}

//

bool LessOrEqual::handle_comparison(REF_ Value & _value, Value const & _valueToCompare) const
{
	if (_value.get_type() != _valueToCompare.get_type())
	{
		error_processing_wmp(TXT("type mismatch for comparison! %S v %S"), RegisteredType::get_name_of(_value.get_type()), RegisteredType::get_name_of(_valueToCompare.get_type()));
		return false;
	}

	if (_value.get_type() == type_id<int>())
	{
		_value.set<bool>(_value.get_as<int>() <= _valueToCompare.get_as<int>());
	}
	else if (_value.get_type() == type_id<float>())
	{
		_value.set<bool>(_value.get_as<float>() <= _valueToCompare.get_as<float>());
	}
	else
	{
		todo_important(TXT("add class handling for where's my point \"lessOrEqual\""));
		return false;
	}

	return true;
}

//

bool GreaterThan::handle_comparison(REF_ Value & _value, Value const & _valueToCompare) const
{
	if (_value.get_type() != _valueToCompare.get_type())
	{
		error_processing_wmp(TXT("type mismatch for comparison! %S v %S"), RegisteredType::get_name_of(_value.get_type()), RegisteredType::get_name_of(_valueToCompare.get_type()));
		return false;
	}

	if (_value.get_type() == type_id<int>())
	{
		_value.set<bool>(_value.get_as<int>() > _valueToCompare.get_as<int>());
	}
	else if (_value.get_type() == type_id<float>())
	{
		_value.set<bool>(_value.get_as<float>() > _valueToCompare.get_as<float>());
	}
	else
	{
		todo_important(TXT("add class handling for where's my point \"greaterThan\""));
		return false;
	}

	return true;
}

//

bool GreaterOrEqual::handle_comparison(REF_ Value & _value, Value const & _valueToCompare) const
{
	if (_value.get_type() != _valueToCompare.get_type())
	{
		error_processing_wmp(TXT("type mismatch for comparison! %S v %S"), RegisteredType::get_name_of(_value.get_type()), RegisteredType::get_name_of(_valueToCompare.get_type()));
		return false;
	}

	if (_value.get_type() == type_id<int>())
	{
		_value.set<bool>(_value.get_as<int>() >= _valueToCompare.get_as<int>());
	}
	else if (_value.get_type() == type_id<float>())
	{
		_value.set<bool>(_value.get_as<float>() >= _valueToCompare.get_as<float>());
	}
	else
	{
		todo_important(TXT("add class handling for where's my point \"greaterOrEqual\""));
		return false;
	}

	return true;
}

//
