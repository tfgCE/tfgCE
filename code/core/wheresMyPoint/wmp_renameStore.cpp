#include "wmp_renameStore.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

//

using namespace WheresMyPoint;

//

bool RenameStore::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	nameFrom = _node->get_name_attribute(TXT("name"), nameFrom);
	nameFrom = _node->get_name_attribute(TXT("from"), nameFrom);
	nameTo = _node->get_name_attribute(TXT("to"), nameTo);
	nameTo = _node->get_name_attribute(TXT("as"), nameTo);

	error_loading_xml_on_assert(nameFrom.is_valid(), result, _node, TXT("requires a name of variable to rename from"));
	error_loading_xml_on_assert(nameTo.is_valid(), result, _node, TXT("requires a name of variable to rename to"));

	return result;
}

bool RenameStore::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (nameFrom.is_valid() && nameTo.is_valid())
	{
		if (_context.get_owner()->rename_value_forwheres_my_point(nameFrom, nameTo))
		{
			// ok
		}
		else
		{
			error_processing_wmp(TXT("could not rename store owner variable \"%S\""), nameFrom.to_char());
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
