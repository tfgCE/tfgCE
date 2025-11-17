#include "wmp_convertStore.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

//

using namespace WheresMyPoint;

//

bool ConvertStore::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	name = _node->get_name_attribute(TXT("name"), name);

	if (auto* attr = _node->get_attribute(TXT("to")))
	{
		convertToType = RegisteredType::get_type_id_by_name(attr->get_as_string().to_char());
	}
	if (auto* attr = _node->get_attribute(TXT("toType")))
	{
		convertToType = RegisteredType::get_type_id_by_name(attr->get_as_string().to_char());
	}
	
	error_loading_xml_on_assert(name.is_valid(), result, _node, TXT("requires a name of variable to convert"));
	error_loading_xml_on_assert(convertToType != type_id_none(), result, _node, TXT("requires a valid type to convert to"));

	return result;
}

bool ConvertStore::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (name.is_valid())
	{
		if (_context.get_owner()->store_convert_value_for_wheres_my_point(name, convertToType))
		{
			// ok
		}
		else
		{
			error_processing_wmp(TXT("could not convert store owner variable \"%S\""), name.to_char());
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
