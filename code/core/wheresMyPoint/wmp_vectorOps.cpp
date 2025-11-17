#include "wmp_vectorOps.h"

#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool VectorOp::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= otherValueToolSet.load_from_xml(_node);

	return result;
}

bool VectorOp::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	Value resultValue = _value;
	Value otherValue;
	if (otherValueToolSet.update(otherValue, _context))
	{
		if (otherValue.get_type() == _value.get_type())
		{
			handle_operation(resultValue, otherValue);
		}
		else
		{
			error_processing_wmp(TXT("type mismatch: %S v %S"), RegisteredType::get_name_of(_value.get_type()), RegisteredType::get_name_of(otherValue.get_type()));
			result = false;
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

void Dot::handle_operation(REF_ Value & _value, Value const & _otherValue) const
{
	an_assert(_otherValue.get_type() == _value.get_type());
	if (_value.get_type() == type_id<Vector3>())
	{
		_value.set<float>(Vector3::dot(_value.get_as<Vector3>(), _otherValue.get_as<Vector3>()));
	}
	else
	{
		todo_important(TXT("add class handling for where's my point \"dot\""));
	}
}

//

void Cross::handle_operation(REF_ Value & _value, Value const & _otherValue) const
{
	an_assert(_otherValue.get_type() == _value.get_type());
	if (_value.get_type() == type_id<Vector3>())
	{
		_value.set<Vector3>(Vector3::cross(_value.get_as<Vector3>(), _otherValue.get_as<Vector3>()));
	}
	else
	{
		todo_important(TXT("add class handling for where's my point \"cross\""));
	}
}

//

void DropUsing::handle_operation(REF_ Value & _value, Value const & _otherValue) const
{
	an_assert(_otherValue.get_type() == _value.get_type());
	if (_value.get_type() == type_id<Vector3>())
	{
		_value.set<Vector3>(_value.get_as<Vector3>().drop_using(_otherValue.get_as<Vector3>()));
	}
	else
	{
		todo_important(TXT("add class handling for where's my point \"dropUsing\""));
	}
}

//

void Along::handle_operation(REF_ Value & _value, Value const & _otherValue) const
{
	an_assert(_otherValue.get_type() == _value.get_type());
	if (_value.get_type() == type_id<Vector3>())
	{
		_value.set<Vector3>(_value.get_as<Vector3>().along(_otherValue.get_as<Vector3>()));
	}
	else
	{
		todo_important(TXT("add class handling for where's my point \"along\""));
	}
}

//

bool IsZeroVector::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (_value.get_type() == type_id<Vector3>())
	{
		_value.set<bool>(_value.get_as<Vector3>().is_zero());
	}
	else
	{
		todo_important(TXT("can't check whether something that is not vector is zero vector"));
		result = false;
	}

	return result;
}

//

