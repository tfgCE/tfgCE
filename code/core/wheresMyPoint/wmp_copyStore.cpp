#include "wmp_copyStore.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool CopyStore::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	from = _node->get_name_attribute(TXT("from"), from);
	as = _node->get_name_attribute(TXT("as"), as);
	as = _node->get_name_attribute(TXT("to"), as);

	from = _node->get_name_attribute(TXT("var"), from);
	as = _node->get_name_attribute(TXT("var"), as);

	onlyIfNotSet = _node->get_bool_attribute_or_from_child_presence(TXT("onlyIfNotSet"), onlyIfNotSet);
	mightBeMissing = _node->get_bool_attribute_or_from_child_presence(TXT("mightBeMissing"), mightBeMissing);

	result &= toolSet.load_from_xml(_node);

	return result;
}

bool CopyStore::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (from.is_valid())
	{
		if (_context.get_owner()->restore_value_for_wheres_my_point(from, _value))
		{
			if (toolSet.update(_value, _context))
			{
				if (as.is_valid())
				{
					if (onlyIfNotSet)
					{
						Value tempValue;
						if (_context.get_owner()->restore_value_for_wheres_my_point(as, tempValue))
						{
							return true;
						}
					}
					if (_context.get_owner()->store_value_for_wheres_my_point(as, _value))
					{
						// ok
					}
					else
					{
						error_processing_wmp(TXT("could not store owner variable \"%S\""), as.to_char());
						result = false;
					}
				}
				else
				{
					error_processing_wmp(TXT("no name"));
					result = false;
				}
			}
			else
			{
				error_processing_wmp(TXT("problem"));
			}
		}
		else if (!mightBeMissing)
		{
			error_processing_wmp(TXT("could not restore owner variable \"%S\""), from.to_char());
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
