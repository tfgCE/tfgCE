#include "worldSettingCondition.h"

#include "worldSetting.h"

#include "..\library\library.h"
#include "..\world\room.h"
#include "..\world\region.h"
#include "..\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

WorldSettingCondition WorldSettingCondition::s_empty;

WorldSettingCondition::WorldSettingCondition()
{
}

WorldSettingCondition::~WorldSettingCondition()
{
}

bool WorldSettingCondition::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	settingType = _node->get_name_attribute(TXT("settingType"), settingType);
	if (!tagged.get())
	{
		tagged = new TagCondition();
	}
	result &= tagged->load_from_xml_attribute(_node, TXT("tagged"));

#ifdef AN_NAT_VIS
	update_for_nat_vis();
#endif

	return result;
}

void WorldSettingCondition::log(LogInfoContext & _context) const
{
	_context.log(to_string().to_char());
}

String WorldSettingCondition::to_string() const
{
	String result;
	if (settingType.is_valid())
	{
		result += String::printf(TXT(" setting type: %S;"), settingType.to_char());
	}
	if (tagged.is_set())
	{
		result += String::printf(TXT(" tagged: %S;"), tagged->to_string().to_char());
	}
	result = result.trim();
	if (result.is_empty())
	{
		result = TXT("<nothing>");
	}
	return result;
}

#ifdef AN_NAT_VIS
void WorldSettingCondition::update_for_nat_vis()
{
	natVis = to_string();
	if (natVis.is_empty())
	{
		natVis = TXT("--");
	}
}
#endif
//

WorldSettingConditionContext & WorldSettingConditionContext::setup_for(Room const * _room)
{
	an_assert(!setupForRoom && !setupForRegion);
	if (_room)
	{
		setupForRoom = _room;
		if (auto * rt = _room->get_room_type())
		{
			push_chain(rt->get_setting());
		}
		if (auto* oroom = _room->get_origin_room())
		{
			if (oroom != _room)
			{
				if (auto* rt = oroom->get_room_type())
				{
					push_chain(rt->get_setting());
				}
			}
		}
		if (Region * region = _room->get_in_region())
		{
			while (region)
			{
				if (auto * rt = region->get_region_type())
				{
					push_chain(rt->get_setting());
				}
				region = region->get_in_region();
			}
		}
	}

	return *this;
}

WorldSettingConditionContext & WorldSettingConditionContext::setup_for(Region const * _region)
{
	an_assert(!setupForRoom && !setupForRegion);
	setupForRegion = _region;
	if (_region)
	{
		if (auto * rt = _region->get_region_type())
		{
			push_chain(rt->get_setting());
		}
		if (Region * region = _region->get_in_region())
		{
			while (region)
			{
				if (auto * rt = region->get_region_type())
				{
					push_chain(rt->get_setting());
				}
				region = region->get_in_region();
			}
		}
	}

	return *this;
}

void WorldSettingConditionContext::update_available_variables_if_required() const
{
	if (availableVariablesSet)
	{
		return;
	}

	availableVariablesSet = true;
	if (setupForRoom)
	{
		setupForRoom->collect_variables(availableVariables);
	}
	if (setupForRegion)
	{
		setupForRegion->collect_variables(availableVariables);
	}
}

bool WorldSettingConditionContext::update_parent_for(LibraryStored const * _libraryStored, Name const & _type, REF_ Optional<bool> & _checkMainResult, int _chainIdx) const
{
	// we use it for main too, then it has _type set to invalid
	while (true)
	{
		if (_chainIdx >= chain.get_size())
		{
			if (_type.is_valid())
			{
				if (!_checkMainResult.is_set())
				{
					_checkMainResult = update_main_for(_libraryStored);
				}
				// if settingType does not work until this point, depend on main
				return _checkMainResult.get();
			}
			else
			{
				// for main, if there is nothing defined, just assume it is ok
				return true;
			}
		}
		Optional<bool> checkParentResult;
		auto* worldSetting = chain[_chainIdx];
		if (auto * wsType = worldSetting->get_type(_type))
		{
			bool thisOneIsOK = wsType->is_ok(_libraryStored, fallback);
			if (thisOneIsOK && wsType->checkParent == WorldSetting::Type::Required)
			{
				if (!checkParentResult.is_set())
				{
					checkParentResult = update_parent_for(_libraryStored, _type, REF_ _checkMainResult, _chainIdx + 1);
				}
				thisOneIsOK = checkParentResult.get();
			}
			if (!thisOneIsOK && wsType->checkParent == WorldSetting::Type::Allowed)
			{
				if (!checkParentResult.is_set())
				{
					checkParentResult = update_parent_for(_libraryStored, _type, REF_ _checkMainResult, _chainIdx + 1);
				}
				thisOneIsOK = checkParentResult.get();
			}
			if (_type.is_valid())
			{
				// check main
				if (thisOneIsOK && wsType->checkMain == WorldSetting::Type::Required)
				{
					if (!_checkMainResult.is_set())
					{
						_checkMainResult = update_main_for(_libraryStored);
					}
					thisOneIsOK = _checkMainResult.get();
				}
				if (!thisOneIsOK && wsType->checkMain == WorldSetting::Type::Allowed)
				{
					if (!_checkMainResult.is_set())
					{
						_checkMainResult = update_main_for(_libraryStored);
					}
					thisOneIsOK = _checkMainResult.get();
				}
			}

			// if it didn't work for this type, it didn't work
			return thisOneIsOK;
		}
		else
		{
			// go up
			++ _chainIdx;
		}
	}
}

bool WorldSettingConditionContext::update_main_for(LibraryStored const * _libraryStored) const
{
	Optional<bool> ob;
	return update_parent_for(_libraryStored, Name::invalid(), ob);
}

bool WorldSettingConditionContext::update_for(LibraryStored const * _libraryStored, Name const & _type, REF_ Optional<bool> & _checkMainResult) const
{
	if (!_type.is_valid())
	{
		if (!_checkMainResult.is_set())
		{
			_checkMainResult = update_main_for(_libraryStored);
		}
		return _checkMainResult.get();
	}
	Optional<bool> checkParentResult;
	for_every_ptr(worldSetting, chain)
	{
		if (auto * wsType = worldSetting->get_type(_type))
		{
			bool thisOneIsOK = wsType->is_ok(_libraryStored, fallback);
			if (thisOneIsOK && wsType->checkParent == WorldSetting::Type::Required)
			{
				if (!checkParentResult.is_set())
				{
					checkParentResult = update_parent_for(_libraryStored, _type, REF_ _checkMainResult, for_everys_index(worldSetting) + 1);
				}
				thisOneIsOK = checkParentResult.get();
			}
			if (!thisOneIsOK && wsType->checkParent == WorldSetting::Type::Allowed)
			{
				if (!checkParentResult.is_set())
				{
					checkParentResult = update_parent_for(_libraryStored, _type, REF_ _checkMainResult, for_everys_index(worldSetting) + 1);
				}
				thisOneIsOK = checkParentResult.get();
			}
			if (thisOneIsOK && wsType->checkMain == WorldSetting::Type::Required)
			{
				if (!_checkMainResult.is_set())
				{
					_checkMainResult = update_main_for(_libraryStored);
				}
				thisOneIsOK = _checkMainResult.get();
			}
			if (!thisOneIsOK && wsType->checkMain == WorldSetting::Type::Allowed)
			{
				if (!_checkMainResult.is_set())
				{
					_checkMainResult = update_main_for(_libraryStored);
				}
				thisOneIsOK = _checkMainResult.get();
			}
			if (thisOneIsOK)
			{
				return true;
			}
			// if there was for this settingType and it didn't work with parents/main, just drop it
			return false;
		}
	}
	// if we don't have a particular type, drop it, skip it
	return false;
}

void WorldSettingConditionContext::get(Library* _library, Name const & _libraryType, Array<LibraryStored*> & _into, WorldSettingCondition const & _condition) const
{
	_library->do_for_every_of_type(_libraryType,
		[&_into, _condition, this](LibraryStored * _libraryStored)
	{
		if (check(_libraryStored, _condition))
		{
			_into.push_back(_libraryStored);
		}
	});
}

bool WorldSettingConditionContext::check(LibraryStored const * _libraryStored, WorldSettingCondition const & _condition) const
{
	if (auto* tagged = _condition.get_tagged())
	{
		if (!tagged->check(_libraryStored->get_tags()))
		{
			// it won't work anyway, even with included directly
			return false;
		}
	}
	if (auto* r = fast_cast<RoomType>(_libraryStored))
	{
		if (!check_required_variables(r->get_required_variables()))
		{
#ifdef LOG_WORLD_GENERATION
			output(TXT("missing required variables"));
#endif
			return false;
		}
	}
	if (auto* r = fast_cast<RegionType>(_libraryStored))
	{
		if (!check_required_variables(r->get_required_variables()))
		{
#ifdef LOG_WORLD_GENERATION
			output(TXT("missing required variables"));
#endif
			return false;
		}
	}

	Optional<bool> checkMainResult;

	bool isOK = update_for(_libraryStored, _condition.get_setting_type(), REF_ checkMainResult);

	return isOK;
}

bool WorldSettingConditionContext::check_required_variables(SimpleVariableStorage const & _requiredVariables) const
{
	if (_requiredVariables.is_empty())
	{
		return true;
	}

	update_available_variables_if_required();
	bool allRequiredVariablesPresent = true;
	_requiredVariables.do_for_every([this, &allRequiredVariablesPresent](SimpleVariableInfo const & _info)
	{
		if (allRequiredVariablesPresent && !availableVariables.find_existing(_info.get_name(), _info.type_id()))
		{
#ifdef LOG_WORLD_GENERATION
			output(TXT("missing required variable \"%S\""), _info.get_name().to_char());
#endif
			allRequiredVariablesPresent = false;
		}
	});

	return allRequiredVariablesPresent;
}