#include "wmp_range2.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool WheresMyPoint::Range2::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	value.load_from_xml(_node);

	result &= toolSet.load_from_xml(_node);

	return result;
}

bool WheresMyPoint::Range2::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	::Range2 resultValue;
	result &= process_value(resultValue, value, toolSet, _context, TXT("\"range2\" expected range2 value"));
	_value.set(resultValue);

	return result;
}
