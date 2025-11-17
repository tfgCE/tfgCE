#include "wmp_isEmpty.h"

#include "..\io\xml.h"
#include "..\math\math.h"
#include "..\other\parserUtils.h"

using namespace WheresMyPoint;

bool IsEmpty::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= toolSet.load_from_xml(_node);

	return result;
}

bool IsEmpty::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	Value valueToOp;
	Value resultValue;
	resultValue.set(true); // not handled
	if (! toolSet.is_empty() && toolSet.update(valueToOp, _context))
	{
		result &= is_empty(valueToOp, resultValue);
	}
	else
	{
		result &= is_empty(_value, resultValue);
	}

	_value = resultValue;

	return result;
}

bool IsEmpty::is_empty(Value const & _value, REF_ Value & _resultValue)
{
	if (_value.get_type() == type_id_none())
	{
		_resultValue.set(true);
		return true;
	}
	else if (_value.get_type() == type_id<float>())
	{
		_resultValue.set(_value.get_as<float>() == 0.0f);
		return true;
	}
	else if (_value.get_type() == type_id<int>())
	{
		_resultValue.set(_value.get_as<int>() == 0.0f);
		return true;
	}
	else if (_value.get_type() == type_id<Range>())
	{
		_resultValue.set(_value.get_as<Range>().is_empty());
		return true;
	}
	else if (_value.get_type() == type_id<Range2>())
	{
		_resultValue.set(_value.get_as<Range2>().is_empty());
		return true;
	}
	else if (_value.get_type() == type_id<Vector2>())
	{
		_resultValue.set(_value.get_as<Vector2>().is_zero());
		return true;
	}
	else if (_value.get_type() == type_id<Vector3>())
	{
		_resultValue.set(_value.get_as<Vector3>().is_zero());
		return true;
	}
	else if (RegisteredType::is_plain_data(_value.get_type()) &&
			 RegisteredType::get_initial_value(_value.get_type()))
	{
		auto* initialValue = RegisteredType::get_initial_value(_value.get_type());
		_resultValue.set(memory_compare(initialValue, _value.get_raw(), RegisteredType::get_size_of(_value.get_type())));
		return true;
	}
	else
	{
		todo_important(TXT("add class handling for where's my point \"isEmpty\"/\"ifIsEmpty\"/\"ifIsNotEmpty\" type \"%S\""), RegisteredType::get_name_of(_value.get_type()));
		return false;
	}
}

//

bool IfIsEmpty::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= doIfEmptyToolSet.load_from_xml(_node);

	return result;
}

bool IfIsEmpty::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	Value resultValue;
	resultValue.set(true); // not handled
	result &= IsEmpty::is_empty(_value, resultValue);
	
	if (resultValue.get_as<bool>())
	{
		result &= doIfEmptyToolSet.update(_value, _context);
	}

	return result;
}

//

bool IfIsNotEmpty::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= doIfNotEmptyToolSet.load_from_xml(_node);

	return result;
}

bool IfIsNotEmpty::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	Value resultValue;
	resultValue.set(false); // not handled
	result &= IsEmpty::is_empty(_value, resultValue);
	
	if (! resultValue.get_as<bool>())
	{
		result &= doIfNotEmptyToolSet.update(_value, _context);
	}

	return result;
}

