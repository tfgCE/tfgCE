#include "wmp_defaultValue.h"
#include "wmp_isEmpty.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"

using namespace WheresMyPoint;

bool DefaultValue::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	forVar = _node->get_name_attribute(TXT("for"), forVar);
	error_loading_xml_on_assert(forVar.is_valid(), result, _node, TXT("requires variables name \"for\""));

	result &= toolSet.load_from_xml(_node);

	ifEmpty = _node->get_bool_attribute_or_from_child_presence(TXT("ifEmpty"), ifEmpty);
	ifEmpty = _node->get_bool_attribute_or_from_child_presence(TXT("isEmpty"), ifEmpty);

	ifEmpty = ! _node->get_bool_attribute_or_from_child_presence(TXT("onlyIfDoesntExist"), ! ifEmpty);

	return result;
}

bool DefaultValue::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	bool provideDefaultValue = false;
	Value valueToCheck;
	if (_context.get_owner()->restore_value_for_wheres_my_point(forVar, valueToCheck))
	{
		if (ifEmpty)
		{
			Value resultValue;
			result &= IsEmpty::is_empty(valueToCheck, resultValue);
			if (resultValue.get_type() == type_id<bool>())
			{
				provideDefaultValue |= resultValue.get_as<bool>();
			}
			else
			{
				error_processing_wmp_tool(this, TXT("invalid result for checking if variable value is empty"));
				result = false;
			}

		}
	}
	else
	{
		provideDefaultValue = true;
	}

	if (result && provideDefaultValue)
	{
		Value providedValue;
		if (toolSet.is_empty())
		{
			providedValue = _value;
		}
		else
		{
			result &= toolSet.update(providedValue, _context);
		}
		
		if (result)
		{
			if (_context.get_owner()->store_value_for_wheres_my_point(forVar, providedValue))
			{
				// ok
			}
			else
			{
				error_processing_wmp_tool(this, TXT("couldn't store provided default value"));
			}
		}
	}

	return result;
}
