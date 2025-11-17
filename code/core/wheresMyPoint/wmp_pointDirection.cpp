#include "wmp_pointDirection.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool PointDirection::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	fromId = _node->get_name_attribute(TXT("from"), fromId);
	toId = _node->get_name_attribute(TXT("to"), toId);

	if (!fromId.is_valid())
	{
		error_loading_xml(_node, TXT("from not defined"));
		result = false;
	}
	if (!toId.is_valid())
	{
		error_loading_xml(_node, TXT("to not defined"));
		result = false;
	}

	return result;
}

bool PointDirection::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	Value resultValue;

	Vector3 from, to;
	if (_context.get_owner()->get_point_for_wheres_my_point(fromId, from))
	{
		if (_context.get_owner()->get_point_for_wheres_my_point(toId, to))
		{
			resultValue.set((to - from).normal());
		}
		else
		{
			error_processing_wmp(TXT("could not find point \"%S\""), toId.to_char());
		}
	}
	else
	{
		error_processing_wmp(TXT("could not find point \"%S\""), fromId.to_char());
	}

	_value = resultValue;

	return result;
}

