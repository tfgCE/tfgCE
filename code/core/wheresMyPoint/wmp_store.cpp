#include "wmp_store.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool Store::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	name = _node->get_name_attribute(TXT("as"), name);

	return result;
}

bool Store::load_prefixed_from_xml(IO::XML::Node const* _node, String const& _prefix)
{
	bool result = base::load_from_xml(_node);

	name = Name(_node->get_name().get_sub(_prefix.get_length()).trim());

	return result;
}

bool Store::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (name.is_valid())
	{
		if (_context.get_owner()->store_value_for_wheres_my_point(name, _value))
		{
			// ok
		}
		else
		{
			error_processing_wmp(TXT("could not store owner variable \"%S\""), name.to_char());
			result = false;
		}
	}
	else
	{
		error_processing_wmp(TXT("no name"));
		result = false;
	}

	return result;
}

//

bool StoreGlobal::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	name = _node->get_name_attribute(TXT("as"), name);

	return result;
}

bool StoreGlobal::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (name.is_valid())
	{
		if (_context.get_owner()->store_global_value_for_wheres_my_point(name, _value))
		{
			// ok
		}
		else
		{
			error_processing_wmp(TXT("could not store owner global variable \"%S\""), name.to_char());
			result = false;
		}
	}
	else
	{
		error_processing_wmp(TXT("no name"));
		result = false;
	}

	return result;
}

//

bool StoreTemp::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	name = _node->get_name_attribute(TXT("as"), name);

	return result;
}

bool StoreTemp::load_prefixed_from_xml(IO::XML::Node const* _node, String const& _prefix)
{
	bool result = base::load_from_xml(_node);

	name = Name(_node->get_name().get_sub(_prefix.get_length()).trim());

	return result;
}

bool StoreTemp::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (name.is_valid())
	{
		_context.set_temp_value(name, _value);
	}
	else
	{
		error_processing_wmp(TXT("no name"));
		result = false;
	}

	return result;
}

//

bool StoreSwap::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	what = _node->get_name_attribute(TXT("what"), what);
	with = _node->get_name_attribute(TXT("with"), with);

	return result;
}

bool StoreSwap::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (what.is_valid() && with.is_valid())
	{
		Value whatValue;
		Value withValue;
		if (_context.get_owner()->restore_value_for_wheres_my_point(what, whatValue) &&
			_context.get_owner()->restore_value_for_wheres_my_point(with, withValue))
		{
			if (_context.get_owner()->store_value_for_wheres_my_point(what, withValue) &&
				_context.get_owner()->store_value_for_wheres_my_point(with, whatValue))
			{
				// ok
			}
			else
			{
				error_processing_wmp_tool(this, TXT("could not store with and what"));
				result = false;
			}
		}
		else
		{
			error_processing_wmp_tool(this, TXT("could not restore with and what"));
			result = false;
		}
	}
	else
	{
		error_processing_wmp(TXT("no what or with"));
		result = false;
	}

	return result;
}
