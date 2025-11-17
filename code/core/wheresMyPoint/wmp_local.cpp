#include "wmp_local.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"

using namespace WheresMyPoint;

bool Local::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= toolSet.load_from_xml(_node);

	return result;
}

bool Local::update(REF_ Value & _value, Context & _context) const
{
	Value local = _value;
	return toolSet.update(local, _context);
}

