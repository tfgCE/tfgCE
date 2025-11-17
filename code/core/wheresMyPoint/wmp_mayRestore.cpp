#include "wmp_mayRestore.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool MayRestore::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	name = _node->get_name_attribute(TXT("from"), name);

	return result;
}

bool MayRestore::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (name.is_valid())
	{
		Value tempValue;
		if (_context.get_owner()->restore_value_for_wheres_my_point(name, tempValue))
		{
			_value.set(true);
		}
		else
		{
			_value.set(false);
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

bool MayRestoreGlobal::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	name = _node->get_name_attribute(TXT("from"), name);

	return result;
}

bool MayRestoreGlobal::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (name.is_valid())
	{
		Value tempValue;
		if (_context.get_owner()->restore_global_value_for_wheres_my_point(name, tempValue))
		{
			_value.set(true);
		}
		else
		{
			_value.set(false);
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

bool MayRestoreTemp::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	name = _node->get_name_attribute(TXT("from"), name);

	return result;
}

bool MayRestoreTemp::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (name.is_valid())
	{
		if (Value const * value = _context.get_temp_value(name))
		{
			_value.set(true);
		}
		else
		{
			_value.set(false);
		}
	}
	else
	{
		error_processing_wmp(TXT("no name"));
		result = false;
	}

	return result;
}
