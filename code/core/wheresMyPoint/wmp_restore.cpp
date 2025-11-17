#include "wmp_restore.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

//

using namespace WheresMyPoint;

//

bool Restore::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	name = _node->get_name_attribute(TXT("from"), name);

	return result;
}

bool Restore::load_prefixed_from_xml(IO::XML::Node const* _node, String const& _prefix)
{
	bool result = base::load_from_xml(_node);

	String var_field = _node->get_name().get_sub(_prefix.get_length()).trim();

	List<String> tokens;
	var_field.split(String(TXT(".")), tokens);
	name = Name(tokens.get_first());

	subfields.clear();
	for (int i = 1; i < tokens.get_size(); ++i)
	{
		subfields.push_back(Name(tokens[i]));
	}

	return result;
}

bool Restore::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (name.is_valid())
	{
		if (_context.get_owner()->restore_value_for_wheres_my_point(name, _value))
		{
			// ok but get subfields
			for_every(s, subfields)
			{
				void* subFieldRawData = nullptr;
				TypeId subFieldId = type_id_none();
				{
					if (access_sub_field(*s, _value.access_raw(), _value.get_type(), subFieldRawData, subFieldId))
					{
						_value.set_raw(subFieldId, subFieldRawData);
					}
					else
					{
						error_processing_wmp(TXT("unable to find sub field \"%S\""), name.to_char());
						result = false;
					}
				}
			}
		}
		else
		{
			error_processing_wmp(TXT("could not restore owner variable \"%S\""), name.to_char());
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

bool RestoreGlobal::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	name = _node->get_name_attribute(TXT("from"), name);

	return result;
}

bool RestoreGlobal::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (name.is_valid())
	{
		if (_context.get_owner()->restore_global_value_for_wheres_my_point(name, _value))
		{
			// ok
		}
		else
		{
			error_processing_wmp(TXT("could not restore owner global variable \"%S\""), name.to_char());
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

bool RestoreTemp::load_from_xml(IO::XML::Node const* _node)
{
	bool result = base::load_from_xml(_node);

	name = _node->get_name_attribute(TXT("from"), name);

	return result;
}

bool RestoreTemp::load_prefixed_from_xml(IO::XML::Node const* _node, String const& _prefix)
{
	bool result = base::load_from_xml(_node);

	String var_field = _node->get_name().get_sub(_prefix.get_length()).trim();

	List<String> tokens;
	var_field.split(String(TXT(".")), tokens);
	name = Name(tokens.get_first());

	subfields.clear();
	for (int i = 1; i < tokens.get_size(); ++i)
	{
		subfields.push_back(Name(tokens[i]));
	}

	return result;
}

bool RestoreTemp::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (name.is_valid())
	{
		if (Value const * value = _context.get_temp_value(name))
		{
			_value = *value;

			// ok but get subfields
			for_every(s, subfields)
			{
				void* subFieldRawData = nullptr;
				TypeId subFieldId = type_id_none();
				{
					if (access_sub_field(*s, _value.access_raw(), _value.get_type(), subFieldRawData, subFieldId))
					{
						_value.set_raw(subFieldId, subFieldRawData);
					}
					else
					{
						error_processing_wmp(TXT("unable to find sub field \"%S\""), name.to_char());
						result = false;
					}
				}
			}
		}
		else
		{
			error_processing_wmp(TXT("temp value \"%S\" not found"), name.to_char());
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
