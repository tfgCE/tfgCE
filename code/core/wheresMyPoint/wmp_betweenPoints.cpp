#include "wmp_betweenPoints.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool BetweenPoints::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	pointA = _node->get_name_attribute(TXT("pointA"), pointA);
	pointA = _node->get_name_attribute(TXT("a"), pointA);
	pointB = _node->get_name_attribute(TXT("pointB"), pointB);
	pointB = _node->get_name_attribute(TXT("b"), pointB);

	if (!pointA.is_valid())
	{
		error_loading_xml(_node, TXT("pointA not defined"));
		result = false;
	}
	if (!pointB.is_valid())
	{
		error_loading_xml(_node, TXT("pointB not defined"));
		result = false;
	}

	at = _node->get_float_attribute(TXT("at"), at);
	result &= atToolSet.load_from_xml(_node);

	return result;
}

bool BetweenPoints::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	float useAt = at;

	Value valueAt;
	valueAt.set(useAt);
	if (atToolSet.update(valueAt, _context))
	{
		if (valueAt.get_type() == type_id<float>())
		{
			useAt = valueAt.get_as<float>();
		}
		else
		{
			error_processing_wmp(TXT("\"betweenPoints\" expected float value"));
			result = false;
		}
	}
	else
	{
		result = false;
	}


	Value resultValue;

	Vector3 pA, pB;
	if (_context.get_owner()->get_point_for_wheres_my_point(pointA, pA))
	{
		if (_context.get_owner()->get_point_for_wheres_my_point(pointB, pB))
		{
			resultValue.set(pA * (1.0f - useAt) + pB * useAt);
		}
		else
		{
			error_processing_wmp(TXT("could not find point \"%S\""), pointB.to_char());
		}
	}
	else
	{
		error_processing_wmp(TXT("could not find point \"%S\""), pointA.to_char());
	}

	_value = resultValue;

	return result;
}

