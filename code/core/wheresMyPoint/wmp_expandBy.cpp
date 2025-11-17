#include "wmp_expandBy.h"

#include "..\containers\arrayStack.h"
#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool ExpandBy::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= toolSet.load_from_xml(_node);

	return result;
}

bool ExpandBy::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	Value resultValue = _value;
	Value valueToExpandBy;
	if (toolSet.update(valueToExpandBy, _context))
	{
		if (resultValue.get_type() == type_id<Range>())
		{
			if (valueToExpandBy.get_type() == type_id<float>())
			{
				resultValue.access_as<Range>().expand_by(valueToExpandBy.get_as<float>());
			}
			else
			{
				error_processing_wmp_tool(this, TXT("expected \"float\" for \"Range\""));
				result = false;
			}
		}
		else if (resultValue.get_type() == type_id<Range2>())
		{
			if (valueToExpandBy.get_type() == type_id<Vector2>())
			{
				resultValue.access_as<Range2>().expand_by(valueToExpandBy.get_as<Vector2>());
			}
			else
			{
				error_processing_wmp_tool(this, TXT("expected \"Vector2\" for \"Range2\""));
				result = false;
			}
		}
		else if (resultValue.get_type() == type_id<Range3>())
		{
			if (valueToExpandBy.get_type() == type_id<Vector3>())
			{
				resultValue.access_as<Range3>().expand_by(valueToExpandBy.get_as<Vector3>());
			}
			else
			{
				error_processing_wmp_tool(this, TXT("expected \"Vector3\" for \"Range3\""));
				result = false;
			}
		}
		else
		{
			error_processing_wmp_tool(this, TXT("don't know how to handle \"%S\""), RegisteredType::get_name_of(resultValue.get_type()));
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

