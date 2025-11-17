#include "wmp_logicOps.h"

#include "..\io\xml.h"
#include "..\other\parserUtils.h"

using namespace WheresMyPoint;

bool LogicOp::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= toolSet.load_from_xml(_node);

	return result;
}

bool LogicOp::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (should_handle_operation(_value))
	{
		Value valueToOp = _value; // just to keep it consistent
		if (toolSet.update(valueToOp, _context))
		{
			result &= handle_operation(_value, valueToOp);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

//

bool And::should_handle_operation(REF_ Value & _value) const
{
	if (_value.get_type() == type_id<bool>())
	{
		return _value.get_as<bool>(); // doesn't make any sense to do "and" when false already
	}
	else
	{
		error_processing_wmp(TXT("can only work on bool values! %S"), RegisteredType::get_name_of(_value.get_type()));
		return false;
	}
}

bool And::handle_operation(REF_ Value & _value, Value const & _valueToOp) const
{
	if (_value.get_type() == type_id<bool>() &&
		_valueToOp.get_type() == type_id<bool>())
	{
		_value.set<bool>(_value.get_as<bool>() && _valueToOp.get_as<bool>());
		return true;
	}
	else
	{
		error_processing_wmp(TXT("can only work on bool values! %S, %S"), RegisteredType::get_name_of(_value.get_type()), RegisteredType::get_name_of(_valueToOp.get_type()));
		return false;
	}
}

//

bool Or::should_handle_operation(REF_ Value & _value) const
{
	if (_value.get_type() == type_id<bool>())
	{
		return !_value.get_as<bool>(); // doesn't make any sense to do "or" if true already
	}
	else
	{
		error_processing_wmp(TXT("can only work on bool values! %S"), RegisteredType::get_name_of(_value.get_type()));
		return false;
	}
}

bool Or::handle_operation(REF_ Value & _value, Value const & _valueToOp) const
{
	if (_value.get_type() == type_id<bool>() &&
		_valueToOp.get_type() == type_id<bool>())
	{
		_value.set<bool>(_value.get_as<bool>() || _valueToOp.get_as<bool>());
		return true;
	}
	else
	{
		error_processing_wmp(TXT("can only work on bool values! %S, %S"), RegisteredType::get_name_of(_value.get_type()), RegisteredType::get_name_of(_valueToOp.get_type()));
		return false;
	}

}

//

bool Xor::should_handle_operation(REF_ Value & _value) const
{
	if (_value.get_type() == type_id<bool>())
	{
		return true; // always - we have to compare
	}
	else
	{
		error_processing_wmp(TXT("can only work on bool values! %S"), RegisteredType::get_name_of(_value.get_type()));
		return false;
	}
}

bool Xor::handle_operation(REF_ Value & _value, Value const & _valueToOp) const
{
	if (_value.get_type() == type_id<bool>() &&
		_valueToOp.get_type() == type_id<bool>())
	{
		_value.set<bool>(_value.get_as<bool>() ^ _valueToOp.get_as<bool>());
		return true;
	}
	else
	{
		error_processing_wmp(TXT("can only work on bool values! %S, %S"), RegisteredType::get_name_of(_value.get_type()), RegisteredType::get_name_of(_valueToOp.get_type()));
		return false;
	}

}

//

bool Not::should_handle_operation(REF_ Value & _value) const
{
	return true;
}

bool Not::handle_operation(REF_ Value & _value, Value const & _valueToOp) const
{
	if (_valueToOp.get_type() == type_id<bool>())
	{
		_value.set<bool>(! _valueToOp.get_as<bool>());
		return true;
	}
	else
	{
		error_processing_wmp(TXT("can only work on bool values! %S"), RegisteredType::get_name_of(_valueToOp.get_type()));
		return false;
	}

}
