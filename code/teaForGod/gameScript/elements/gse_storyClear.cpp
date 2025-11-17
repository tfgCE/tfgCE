#include "gse_storyClear.h"

#include "..\..\game\gameDirector.h"
#include "..\..\story\storyExecution.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool StoryClear::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	fact.load_from_xml(_node, TXT("fact"));
	factPrefix.load_from_xml(_node, TXT("factPrefix"));
	allFacts = _node->get_bool_attribute_or_from_child_presence(TXT("facts"), allFacts);

	return result;
}

bool StoryClear::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type StoryClear::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* gd = TeaForGodEmperor::GameDirector::get_active())
	{
		Concurrency::ScopedMRSWLockWrite lock(gd->access_story_execution().access_facts_lock());
		if (allFacts)
		{
			gd->access_story_execution().access_facts().clear();
		}
		if (fact.is_set())
		{
			gd->access_story_execution().access_facts().remove_tag(fact.get());
		}
		if (factPrefix.is_set())
		{
			Tags tagsToRemove;
			tchar const* prefix = factPrefix.get().to_char();
			gd->access_story_execution().get_facts().do_for_every_tag([&tagsToRemove, prefix](Tag const& _tag)
				{
					if (_tag.get_tag().to_string().has_prefix(prefix))
					{
						tagsToRemove.set_tag(_tag.get_tag());
					}
				});
			gd->access_story_execution().access_facts().remove_tags(tagsToRemove);
		}
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
