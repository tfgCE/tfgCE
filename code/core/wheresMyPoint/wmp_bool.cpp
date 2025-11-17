#include "wmp_bool.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\other\parserUtils.h"

using namespace WheresMyPoint;

bool Bool::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	value = ParserUtils::parse_bool(_node->get_text(), value);

	return result;
}

bool Bool::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	_value.set(value);

	return result;
}

//

bool True::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	_value.set(true);

	return result;
}

//

bool False::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	_value.set(false);

	return result;
}

//

bool BoolRandom::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= toolSet.load_from_xml(_node);

	floatValuePresent = !_node->has_children() && !_node->get_internal_text().trim().is_empty();
	floatValue = ParserUtils::parse_float(_node->get_internal_text(), floatValue);

	return result;
}

bool BoolRandom::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (!toolSet.is_empty())
	{
		if (toolSet.update(_value, _context) && _value.is_set())
		{
			if (_value.get_type() == type_id<float>())
			{
				_value.set<bool>(_context.access_random_generator().get_chance(_value.get_as<float>()));
			}
			else
			{
				error_processing_wmp_tool(this, TXT("type %S not handled"), RegisteredType::get_name_of(_value.get_type()));
				result = false;
			}
		}
		else
		{
			error_processing_wmp_tool(this, TXT("could not get value for boolRandom"));
			result = false;
		}
	}
	else
	{
		if (floatValuePresent)
		{
			_value.set(_context.access_random_generator().get_chance(floatValue));
		}
		else
		{
			_value.set(_context.access_random_generator().get_bool());
		}
	}

	return result;
}
