#include "wmp_range3.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool WheresMyPoint::Range3::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	value.load_from_xml(_node);

	result &= toolSet.load_from_xml(_node);

	return result;
}

bool WheresMyPoint::Range3::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	::Range3 resultValue;
	result &= process_value(resultValue, value, toolSet, _context, TXT("\"range3\" expected range3 value"));
	_value.set(resultValue);

	return result;
}
