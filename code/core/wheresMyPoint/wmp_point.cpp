#include "wmp_point.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool Point::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	pointId = _node->get_name_attribute(TXT("id"), pointId);

	return result;
}

bool Point::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (pointId.is_valid())
	{
		Vector3 point;
		if (_context.get_owner()->get_point_for_wheres_my_point(pointId, point))
		{
			_value.set(point);
		}
		else
		{
			error_processing_wmp(TXT("could not find point \"%S\""), pointId.to_char());
			result = false;
		}
	}
	else
	{
		error_processing_wmp(TXT("nothing set"));
		result = false;
	}

	return result;
}
