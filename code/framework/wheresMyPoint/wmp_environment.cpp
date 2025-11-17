#include "wmp_environment.h"

#include "..\library\library.h"
#include "..\world\region.h"
#include "..\world\room.h"

#ifdef AN_CLANG
#include "..\library\usedLibraryStored.inl"
#endif

using namespace Framework;

//

bool UsedEnvironmentFromRegion::update(REF_ WheresMyPoint::Value& _value, WheresMyPoint::Context& _context) const
{
	bool result = true;

	WheresMyPoint::IOwner* owner = _context.get_owner();
	while (owner)
	{
		if (auto* r = fast_cast<Room>(owner))
		{
			_value.set<Name>(r->access_use_environment().useFromRegion);
		}
		owner = owner->get_wmp_owner();
	}

	return result;
}

//

bool UseEnvironmentFromRegion::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	if (!_node->has_children())
	{
		String text = _node->get_text();
		if (!text.is_empty())
		{
			useFromRegion = Name(text);
		}
	}
	else
	{
		result &= toolSet.load_from_xml(_node);
	}

	return result;
}

bool UseEnvironmentFromRegion::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	WheresMyPoint::IOwner* owner = _context.get_owner();
	while (owner)
	{
		if (auto* r = fast_cast<Room>(owner))
		{
			r->access_use_environment().clear();
			if (useFromRegion.is_valid())
			{
				r->access_use_environment().useFromRegion = useFromRegion;
			}
			else
			{
				WheresMyPoint::Value v;
				if (toolSet.update(v, _context))
				{
					if (v.is_set() && v.get_type() == type_id<Name>())
					{
						r->access_use_environment().useFromRegion = v.get_as<Name>();
					}
					else
					{
						error_processing_wmp_tool(this, TXT("invalid update for useFromRegion (not name!)"));
					}
				}
				else
				{
					error_processing_wmp_tool(this, TXT("invalid update for useFromRegion"));
				}
			}
			break;
		}
		owner = owner->get_wmp_owner();
	}

	return result;
}

//

bool AddEnvironmentOverlay::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	if (!_node->has_children())
	{
		String text = _node->get_text();
		if (!text.is_empty())
		{
			environmentOverlay = LibraryName(text);
		}
	}
	else
	{
		result &= toolSet.load_from_xml(_node);
	}

	return result;
}

bool AddEnvironmentOverlay::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	WheresMyPoint::IOwner* owner = _context.get_owner();
	while (owner)
	{
		auto* room = fast_cast<Room>(owner);
		auto* region = fast_cast<Region>(owner);
		if (room || region)
		{
			LibraryName eoName;
			if (environmentOverlay.is_valid())
			{
				eoName = environmentOverlay;
			}
			else
			{
				WheresMyPoint::Value v;
				if (toolSet.update(v, _context))
				{
					if (v.is_set() && v.get_type() == type_id<LibraryName>())
					{
						eoName = v.get_as<LibraryName>();
					}
					else
					{
						error_processing_wmp_tool(this, TXT("invalid update for addEnvironmentOverlay (not LibraryName!)"));
					}
				}
				else
				{
					error_processing_wmp_tool(this, TXT("invalid update for addEnvironmentOverlay"));
				}
			}
			if (eoName.is_valid())
			{
				UsedLibraryStored<EnvironmentOverlay> eo;
				eo.set_name(eoName);
				if (eo.find(Library::get_current()))
				{
					if (room)
					{
						room->access_environment_overlays().push_back(eo);
					}
					if (region)
					{
						region->access_environment_overlays().push_back(eo);
					}
				}
			}
			else
			{
				error_processing_wmp_tool(this, TXT("no environment overlay provided"));
			}
			break;
		}
		owner = owner->get_wmp_owner();
	}

	return result;
}

//

bool ClearEnvironmentOverlays::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	WheresMyPoint::IOwner* owner = _context.get_owner();
	while (owner)
	{
		if (auto* r = fast_cast<Room>(owner))
		{
			r->access_environment_overlays().clear();
			break;
		}
		owner = owner->get_wmp_owner();
	}

	return result;
}

