#include "wmp_chooseByIntValue.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\random\randomUtils.h"

//

using namespace WheresMyPoint;

//

bool ChooseByIntValue::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	varName = _node->get_name_attribute(TXT("var"), varName);

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
		e.value.load_from_xml(node, TXT("for"));
		e.value.load_from_xml(node, TXT("value"));
		e.doToolSet.load_from_xml(node);
		e.provideFloat.load_from_xml(node, TXT("provideFloat"));
		e.provideInt.load_from_xml(node, TXT("provideInt"));
	}

	return result;
}

bool ChooseByIntValue::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	Optional<int> val;
	if (varName.is_valid())
	{
		Value readVal;
		if (_context.get_owner()->restore_value_for_wheres_my_point(varName, readVal))
		{
			if (readVal.get_type() == type_id<int>())
			{
				val = readVal.get_as<int>();
			}
			else
			{
				error_processing_wmp_tool(this, TXT("var \"%S\" should be int"), varName.to_char());
				result = false;
			}
		}
		else
		{
			error_processing_wmp_tool(this, TXT("no var \"%S\" found"), varName.to_char());
			result = false;
		}
	}
	if (!val.is_set())
	{
		if (_value.get_type() == type_id<int>())
		{
			val = _value.get_as<int>();
		}
		else
		{
			error_processing_wmp_tool(this, TXT("expected int value provided"));
			result = false;
		}
	}
	if (val.is_set())
	{
		int chosenIdx = NONE;
		for_every(e, elements)
		{
			if (e->value.is_set() && e->value.get() == val.get())
			{
				chosenIdx = for_everys_index(e);
				break;
			}
			else if (! e->value.is_set())
			{
				// default/fallback
				chosenIdx = for_everys_index(e);
			}
		}
		if (elements.is_index_valid(chosenIdx))
		{
			auto& e = elements[chosenIdx];
			if (e.provideFloat.is_set()) _value.set<float>(e.provideFloat.get());
			if (e.provideInt.is_set()) _value.set<int>(e.provideInt.get());
			result = e.doToolSet.update(_value, _context);
		}
		else
		{
			error_processing_wmp_tool(this, TXT("no match"));
			result = false;
		}
	}
	else
	{
		error_processing_wmp_tool(this, TXT("expected int value provided"));
		result = false;
	}

	return result;
}
