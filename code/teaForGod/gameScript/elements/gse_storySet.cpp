#include "gse_storySet.h"

#include "..\..\game\gameDirector.h"
#include "..\..\story\storyExecution.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool StorySet::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	fact.load_from_xml(_node, TXT("fact"));
	if (fact.is_set())
	{
		factValue = _node->get_int_attribute_or_from_child(TXT("value"), factValue);
	}

	return result;
}

bool StorySet::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type StorySet::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* gd = TeaForGodEmperor::GameDirector::get_active())
	{
		Concurrency::ScopedMRSWLockWrite lock(gd->access_story_execution().access_facts_lock());
		gd->access_story_execution().access_facts().set_tag(fact.get(), factValue);
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
