#include "wmp_gameDefinition.h"

#include "..\library\gameDefinition.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

bool GameDefinitionForWheresMyPoint::load_from_xml(IO::XML::Node const* _node)
{
	bool result = base::load_from_xml(_node);

	tagged.load_from_xml_attribute_or_child_node(_node, TXT("tagged"));

	return result;
}

bool GameDefinitionForWheresMyPoint::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	_value.set<bool>(TeaForGodEmperor::GameDefinition::check_chosen(tagged));

	return result;
}
