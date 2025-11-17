#include "wmp_float.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\other\parserUtils.h"

using namespace WheresMyPoint;

//

bool Float::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= toolSet.load_from_xml(_node);

	if (_node && !_node->get_text().is_empty() && toolSet.is_empty())
	{
		value.load_from_xml(_node);
	}

	return result;
}

bool Float::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (!toolSet.is_empty())
	{
		if (toolSet.update(_value, _context) && _value.is_set())
		{
			if (_value.get_type() == type_id<float>())
			{
				// ok
			}
			else
			{
				error_processing_wmp_tool(this, TXT("expecting float"));
			}
		}
		else
		{
			error_processing_wmp_tool(this, TXT("could not get value for float"));
			result = false;
		}
	}
	else if (value.is_set())
	{
		_value.set(value.get());
	}
	else if (_value.get_type() != type_id<float>())
	{
		_value.set(0.0f);
	}

	return result;
}

//

bool ToFloat::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (_value.get_type() == type_id<int>())
	{
		_value.set((float)_value.get_as<int>());
	}
	else
	{
		error_processing_wmp(TXT("can't convert \"%S\" to int"), RegisteredType::get_name_of(_value.get_type()));
		result = false;
	}

	return result;
}

//

bool FloatRandom::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= toolSet.load_from_xml(_node);

	minMaxDefined = _node->has_attribute_or_child(TXT("min")) ||
					_node->has_attribute_or_child(TXT("max"));
	minValue = _node->get_float_attribute_or_from_child(TXT("min"), minValue);
	maxValue = _node->get_float_attribute_or_from_child(TXT("max"), maxValue);

	result &= randomNumber.load_from_xml(_node);

	return result;
}

bool FloatRandom::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (!randomNumber.is_empty())
	{
		_value.set(randomNumber.get(_context.access_random_generator()));
	}
	else if (!minMaxDefined || !toolSet.is_empty())
	{
		if (toolSet.update(_value, _context) && _value.is_set())
		{
			if (_value.get_type() == type_id<float>())
			{
				_value.set<float>(_context.access_random_generator().get_float(0.0f, _value.get_as<float>()));
			}
			else if (_value.get_type() == type_id<Range>())
			{
				_value.set<float>(_context.access_random_generator().get(_value.get_as<Range>()));
			}
			else
			{
				error_processing_wmp_tool(this, TXT("type %S not handled"), RegisteredType::get_name_of(_value.get_type()));
				result = false;
			}
		}
		else
		{
			error_processing_wmp_tool(this, TXT("could not get value for floatRandom"));
			result = false;
		}
	}
	else
	{
		_value.set(_context.access_random_generator().get_float(minValue, maxValue));
	}

	return result;
}
