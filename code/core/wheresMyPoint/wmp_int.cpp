#include "wmp_int.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\other\parserUtils.h"

using namespace WheresMyPoint;

bool Int::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	value = ParserUtils::parse_int(_node->get_text(), value);

	return result;
}

bool Int::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	_value.set(value);

	return result;
}

//

bool ToInt::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (_value.get_type() == type_id<float>())
	{
		_value.set((int)(round(_value.get_as<float>()) + 0.1f));
	}
	else
	{
		error_processing_wmp(TXT("can't convert \"%S\" to int"), RegisteredType::get_name_of(_value.get_type()));
		result = false;
	}

	return result;
}

//

bool IntRandom::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= toolSet.load_from_xml(_node);

	minMaxDefined = _node->has_attribute_or_child(TXT("min")) ||
					_node->has_attribute_or_child(TXT("max"));
	minValue = _node->get_int_attribute_or_from_child(TXT("min"), minValue);
	maxValue = _node->get_int_attribute_or_from_child(TXT("max"), maxValue);

	result &= randomNumber.load_from_xml(_node);

	return result;
}

bool IntRandom::update(REF_ Value & _value, Context & _context) const
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
			if (_value.get_type() == type_id<RangeInt>())
			{
				_value.set<int>(_context.access_random_generator().get(_value.get_as<RangeInt>()));
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
		_value.set(_context.access_random_generator().get_int_from_range(minValue, maxValue));
	}

	return result;
}
