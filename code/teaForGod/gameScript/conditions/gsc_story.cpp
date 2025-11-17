#include "gsc_story.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\game\gameDirector.h"
#include "..\..\story\storyExecution.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

bool Conditions::Story::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	fact.load_from_xml(_node, TXT("fact"));
	noFact.load_from_xml(_node, TXT("noFact"));
	if (fact.is_set() || noFact.is_set())
	{
		factValue.load_from_xml(_node, TXT("value"));
	}

	error_loading_xml_on_assert(!fact.is_set() || !noFact.is_set(), result, _node, TXT("can't have more than one at a time"));

	return result;
}

bool Conditions::Story::check(Framework::GameScript::ScriptExecution const & _execution) const
{
	bool result = false;

	if (auto* gd = GameDirector::get_active())
	{
		if (fact.is_set())
		{
			int actualValue;
			{
				Concurrency::ScopedMRSWLockRead lock(gd->access_story_execution().access_facts_lock());
				actualValue = gd->access_story_execution().get_facts().get_tag_as_int(fact.get());
			}
			if (factValue.is_set())
			{
				return actualValue == factValue.get();
			}
			else
			{
				return actualValue != 0;
			}
		}
		if (noFact.is_set())
		{
			int actualValue;
			{
				Concurrency::ScopedMRSWLockRead lock(gd->access_story_execution().access_facts_lock());
				actualValue = gd->access_story_execution().get_facts().get_tag_as_int(noFact.get());
			}
			if (factValue.is_set())
			{
				return actualValue != factValue.get();
			}
			else
			{
				return actualValue == 0;
			}
		}
	}

	return result;
}
