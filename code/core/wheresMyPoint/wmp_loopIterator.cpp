#include "wmp_loopIterator.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\other\parserUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace WheresMyPoint;

//

bool LoopIterator::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= toolSet.load_from_xml(_node);

	initVar = _node->get_name_attribute(TXT("initVar"), initVar);
	advanceVar = _node->get_name_attribute(TXT("advanceVar"), advanceVar);
	advanceVar = _node->get_name_attribute(TXT("increaseVar"), advanceVar);

	intValuePresent = (!_node->has_children() && !_node->get_internal_text().trim().is_empty())
					|| _node->has_attribute(TXT("set"))
					|| _node->has_attribute(TXT("lessThan"))
					|| _node->has_attribute(TXT("count"));
	intValue = ParserUtils::parse_int(_node->get_internal_text(), intValue);

	intValue = _node->get_int_attribute(TXT("set"), intValue);
	intValue = _node->get_int_attribute(TXT("lessThan"), intValue);
	intValue = _node->get_int_attribute(TXT("count"), intValue);

	error_loading_xml_on_assert(initVar.is_valid() || advanceVar.is_valid(), result, _node, TXT("requires \"initVar\" or \"advanceVar\"/\"increaseVar\""));

	return result;
}

bool LoopIterator::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	Value useValue;
	useValue.set<int>(0);
	if (intValuePresent)
	{
		useValue.set<int>(intValue);
	}

	if (!toolSet.is_empty())
	{
		Value iv = useValue;
		if (toolSet.update(iv, _context))
		{
			if (iv.is_set() && iv.get_type() == type_id<int>())
			{
				useValue = iv;
			}
			else
			{
				error_processing_wmp_tool(this, TXT("expected int value"));
			}
		}
	}

	if (initVar.is_valid())
	{
		if (_context.get_owner()->store_value_for_wheres_my_point(initVar, useValue))
		{
			_value = useValue;
		}
		else
		{
			error_processing_wmp_tool(this, TXT("could not init iterator var"));
		}
	}

	if (advanceVar.is_valid())
	{
		Value iteratorVar;
		if (_context.get_owner()->restore_value_for_wheres_my_point(advanceVar, iteratorVar))
		{
			if (iteratorVar.is_set() && iteratorVar.get_type() == type_id<int>())
			{
				iteratorVar.set<int>(iteratorVar.get_as<int>() + 1);
				if (_context.get_owner()->store_value_for_wheres_my_point(advanceVar, iteratorVar))
				{
					int iv = iteratorVar.get_as<int>();
					int uv = useValue.get_as<int>();
					_value.set<bool>(iv < uv);
				}
				else
				{
					error_processing_wmp_tool(this, TXT("could store iterator var"));
					_value.set<bool>(false);
				}
			}
			else
			{
				error_processing_wmp_tool(this, TXT("expected iterator var to be int"));
				_value.set<bool>(false);
			}
		}
		else
		{
			error_processing_wmp_tool(this, TXT("could not restore iterator var"));
			_value.set<bool>(false);
		}
	}

	return result;
}
