#include "wmp_libraryName.h"

#include "..\world\room.h"

using namespace Framework;

//

bool LibraryNameWMP::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	if (!_node->has_children())
	{
		String text = _node->get_text();
		if (!text.is_empty())
		{
			libraryName = LibraryName(text);
		}
	}
	else
	{
		result &= toolSet.load_from_xml(_node);
	}

	return result;
}

bool LibraryNameWMP::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	if (libraryName.is_valid() || toolSet.is_empty())
	{
		_value.set<LibraryName>(libraryName);
	}
	else
	{
		WheresMyPoint::Value v;
		if (toolSet.update(v, _context))
		{
			if (v.is_set() && v.get_type() == type_id<LibraryName>())
			{
				_value = v;
				//_value.set<LibraryName>(v.get_as<LibraryName>());
			}
			else
			{
				error_processing_wmp_tool(this, TXT("invalid update for libraryName (not library name!)"));
			}
		}
		else
		{
			error_processing_wmp_tool(this, TXT("invalid update for libraryName"));
		}
	}

	return result;
}

//
