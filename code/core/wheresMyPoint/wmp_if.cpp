#include "wmp_if.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"

using namespace WheresMyPoint;

bool If::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);
	for_every(node, _node->all_children())
	{
		if (node->get_name() == TXT("true"))
		{
			result &= trueToolSet.load_from_xml(node);
		}
		else if (node->get_name() == TXT("false"))
		{
			result &= falseToolSet.load_from_xml(node);
		}
		else
		{
			error_loading_xml(_node, TXT("for \"if\" expected only \"true\" and/or \"false\""));
			result = false;
		}
	}
	return result;
}

bool If::update(REF_ Value & _value, Context & _context) const
{
	if (_value.get_type() != type_id<bool>())
	{
		error_processing_wmp(TXT("input / current state should be bool! %S"), RegisteredType::get_name_of(_value.get_type()));
		return false;
	}

	bool result = true;
	bool inputValue = _value.get_as<bool>();
	if (inputValue && !trueToolSet.is_empty())
	{
		result &= trueToolSet.update(_value, _context);
	}
	if (! inputValue && !falseToolSet.is_empty())
	{
		result &= falseToolSet.update(_value, _context);
	}
	return result;
}

//

bool IfTrue::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);
	result &= trueToolSet.load_from_xml(_node);
	return result;
}

bool IfTrue::update(REF_ Value & _value, Context & _context) const
{
	if (_value.get_type() != type_id<bool>())
	{
		error_processing_wmp(TXT("input / current state should be bool! %S"), RegisteredType::get_name_of(_value.get_type()));
		return false;
	}

	bool result = true;
	bool inputValue = _value.get_as<bool>();
	if (inputValue && !trueToolSet.is_empty())
	{
		result &= trueToolSet.update(_value, _context);
	}
	return result;
}

//

bool IfFalse::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);
	result &= falseToolSet.load_from_xml(_node);
	return result;
}

bool IfFalse::update(REF_ Value & _value, Context & _context) const
{
	if (_value.get_type() != type_id<bool>())
	{
		error_processing_wmp(TXT("input / current state should be bool! %S"), RegisteredType::get_name_of(_value.get_type()));
		return false;
	}

	bool result = true;
	bool inputValue = _value.get_as<bool>();
	if (!inputValue && !falseToolSet.is_empty())
	{
		result &= falseToolSet.update(_value, _context);
	}
	return result;
}
