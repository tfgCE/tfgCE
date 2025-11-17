#include "gse_generalProgress.h"

#include "..\..\game\game.h"
#include "..\..\game\playerSetup.h"
#include "..\..\story\storyExecution.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool GeneralProgress::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	set.load_from_xml_attribute_or_child_node(_node, TXT("set"));
	setProfile.load_from_xml_attribute_or_child_node(_node, TXT("setProfile"));
	setSlot.load_from_xml_attribute_or_child_node(_node, TXT("setSlot"));
	add.load_from_xml_attribute_or_child_node(_node, TXT("add"));
	addProfile.load_from_xml_attribute_or_child_node(_node, TXT("addProfile"));
	addSlot.load_from_xml_attribute_or_child_node(_node, TXT("addSlot"));

	return result;
}

bool GeneralProgress::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type GeneralProgress::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	struct Set
	{
		static void to(Tags const& _set, Tags & _to)
		{
			_set.do_for_every_tag([&_to](Tag const& _tag)
				{
					float value = _to.get_tag(_tag.get_tag());
					value += 1.0f; // a way to track progress, at least up to a certain value
					_to.set_tag(_tag.get_tag(), value);
				});
		}
	};
	struct Add
	{
		static void to(Tags const& _set, Tags & _to)
		{
			_set.do_for_every_tag([&_to](Tag const& _tag)
				{
					float value = _to.get_tag(_tag.get_tag());
					value = min(value, 1.0f);
					_to.set_tag(_tag.get_tag(), value);
				});
		}
	};
	if (auto* ps = PlayerSetup::access_current_if_exists())
	{
		Set::to(set, ps->access_general_progress());
		Set::to(setProfile, ps->access_general_progress());
		if (auto* as = ps->access_active_game_slot())
		{
			Set::to(set, as->access_general_progress());
			Set::to(setSlot, as->access_general_progress());
		}
		Add::to(add, ps->access_general_progress());
		Add::to(addProfile, ps->access_general_progress());
		if (auto* as = ps->access_active_game_slot())
		{
			Add::to(add, as->access_general_progress());
			Add::to(addSlot, as->access_general_progress());
		}
	}
	if (auto* g = Game::get_as<Game>())
	{
		g->add_async_save_player_profile();
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
