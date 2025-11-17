#pragma once

#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\other\simpleVariableStorage.h"
#include "..\..\core\types\name.h"
#include "..\..\core\tags\tagCondition.h"

namespace Framework
{
	class Library;
	class LibraryStored;
	class Region;
	class Room;
	class WorldSetting;

	class WorldSettingCondition
	: public RefCountObject
	{
	public:
		WorldSettingCondition();
		virtual ~WorldSettingCondition();

		static WorldSettingCondition const & empty() { return s_empty; }

		bool load_from_xml(IO::XML::Node const * _node);

		Name const & get_setting_type() const { return settingType; }
		TagCondition const * get_tagged() const { return tagged.get(); }

		bool is_empty() const { return !settingType.is_valid() && (!tagged.is_set() || tagged->is_empty()); }

		void log(LogInfoContext & _context) const;

		String to_string() const;

	private:
		static WorldSettingCondition s_empty;

#ifdef AN_NAT_VIS
		String natVis;
		void update_for_nat_vis();
#endif

		Name settingType;
		RefCountObjectPtr<TagCondition> tagged;
	};

	struct WorldSettingConditionContext
	{
	public:
		WorldSettingConditionContext() { SET_EXTRA_DEBUG_INFO(chain, TXT("WorldSettingConditionContext.chain")); }
		//WorldSettingConditionContext(WorldSetting const * _setting) { push_chain(_setting); }
		WorldSettingConditionContext(Room const * _room) { SET_EXTRA_DEBUG_INFO(chain, TXT("WorldSettingConditionContext.chain")); setup_for(_room); }
		WorldSettingConditionContext(Region const * _region) { SET_EXTRA_DEBUG_INFO(chain, TXT("WorldSettingConditionContext.chain")); setup_for(_region); }

		// setup
		void try_fallback(bool _tryFallback = true) { fallback = _tryFallback; }

		// run
		void get(Library* _library, Name const & _libraryType, Array<LibraryStored*> & _into, WorldSettingCondition const & _condition = WorldSettingCondition()) const;
		bool check(LibraryStored const * _libraryStored, WorldSettingCondition const & _condition = WorldSettingCondition()) const;

	private:
		bool fallback = false;
		ArrayStatic<WorldSetting const *, 16> chain; // from most important to least important (from child to top parent)
		Room const * setupForRoom = nullptr;
		Region const * setupForRegion = nullptr;
		mutable bool availableVariablesSet = false;
		mutable SimpleVariableStorage availableVariables;

		// setup
		WorldSettingConditionContext & setup_for(Room const * _room);
		WorldSettingConditionContext & setup_for(Region const * _region);
		WorldSettingConditionContext & push_chain(WorldSetting const * _element) { if (_element) { chain.push_back(_element); } return *this; }

		// run
		bool update_for(LibraryStored const * _libraryStored, Name const & _type, REF_ Optional<bool> & _checkMainResult) const;
		bool update_parent_for(LibraryStored const * _libraryStored, Name const & _type, REF_ Optional<bool> & _checkMainResult, int _chainIdx = 0) const;
		bool update_main_for(LibraryStored const * _libraryStored) const;

		// utils
		void update_available_variables_if_required() const;
		bool check_required_variables(SimpleVariableStorage const & _requiredVariables) const;
	};
};

//

DECLARE_REGISTERED_TYPE(Framework::WorldSettingCondition);

//

template <>
inline Framework::WorldSettingCondition default_value<Framework::WorldSettingCondition>()
{
	return Framework::WorldSettingCondition::empty();
}
