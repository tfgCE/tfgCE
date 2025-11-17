#include "wmp_chooseOne.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\random\randomUtils.h"

using namespace WheresMyPoint;

//

bool ChooseOne::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	int nodes = 0;
	for_every(node, _node->children_named(TXT("choose")))
	{
		++nodes;
	}
	elements.make_space_for_additional(nodes);
	
	for_every(node, _node->children_named(TXT("choose")))
	{
		elements.push_back(Element());
		auto& e = elements.get_last();
		e.probCoef = node->get_float_attribute(TXT("prob"), e.probCoef);
		e.probCoef = node->get_float_attribute(TXT("probCoef"), e.probCoef);
		e.doToolSet.load_from_xml(node);
	}

	return result;
}

bool ChooseOne::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	int tryIdx = RandomUtils::ChooseFromContainer<Array<Element>, Element>::choose(
		_context.access_random_generator(), elements, [](Element const & _e) { return _e.probCoef; });

	if (elements.is_index_valid(tryIdx))
	{
		result = elements[tryIdx].doToolSet.update(_value, _context);
	}

	return result;
}
