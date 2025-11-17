#include "wmp_include.h"

#include "..\containers\arrayStack.h"
#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool Include::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= toolSet.load_from_xml(_node);

	return result;
}

bool Include::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	Value resultValue = _value;
	Value valueToInclude;
	if (toolSet.update(valueToInclude, _context))
	{
		if (resultValue.get_type() == type_id<Range>())
		{
			if (valueToInclude.get_type() == type_id<float>())
			{
				resultValue.access_as<Range>().include(valueToInclude.get_as<float>());
			}
			else
			{
				error_processing_wmp_tool(this, TXT("expected \"float\" for \"Range\""));
				result = false;
			}
		}
		else if (resultValue.get_type() == type_id<Range2>())
		{
			if (valueToInclude.get_type() == type_id<Vector2>())
			{
				resultValue.access_as<Range2>().include(valueToInclude.get_as<Vector2>());
			}
			else
			{
				error_processing_wmp_tool(this, TXT("expected \"Vector2\" for \"Range2\""));
				result = false;
			}
		}
		else if (resultValue.get_type() == type_id<Range3>())
		{
			if (valueToInclude.get_type() == type_id<Vector3>())
			{
				resultValue.access_as<Range3>().include(valueToInclude.get_as<Vector3>());
			}
			else
			{
				error_processing_wmp_tool(this, TXT("expected \"Vector3\" for \"Range3\""));
				result = false;
			}
		}
		else
		{
			error_processing_wmp_tool(this, TXT("don't know how to handle \"%S\""), RegisteredType::get_name_of(_value.get_type()));
			result = false;
			todo_implement;
		}
	}
	else
	{
		result = false;
	}

	_value = resultValue;

	return result;
}

