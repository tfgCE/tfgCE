#include "wmp_dropOnto.h"

#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool DropOnto::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= ontoObjectToolSet.load_from_xml(_node);

	return result;
}

bool DropOnto::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	Value resultValue = _value;
	Value ontoObjectValue;
	if (ontoObjectToolSet.update(ontoObjectValue, _context))
	{
		if (resultValue.get_type() == type_id<::Vector3>() &&
			ontoObjectValue.get_type() == type_id<::Plane>())
		{
			resultValue.set(ontoObjectValue.get_as<::Plane>().get_dropped(resultValue.get_as<::Vector3>()));
		}
		else
		{
			error_processing_wmp(TXT("I don't know how to drop that onto that"));
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

