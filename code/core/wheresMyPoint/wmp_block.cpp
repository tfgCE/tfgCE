#include "wmp_block.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool Block::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= toolSet.load_from_xml(_node);

	return result;
}

bool Block::update(REF_ Value & _value, Context & _context) const
{
	return toolSet.update(_value, _context);
}

