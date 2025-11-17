#include "wmp_name.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
using namespace WheresMyPoint;

bool WheresMyPoint::Name::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	if (!_node->get_text().is_empty())
	{
		value = ::Name(_node->get_text());
	}
	value = _node->get_name_attribute(TXT("value"), value);

	return result;
}

bool WheresMyPoint::Name::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	_value.set(value);

	return result;
}
