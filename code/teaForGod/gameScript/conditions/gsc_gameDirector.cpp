#include "gsc_gameDirector.h"

#include "..\..\game\gameDirector.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

bool Conditions::GameDirector::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	XML_LOAD(_node, narrative);
	XML_LOAD(_node, narrativeRequested);
	XML_LOAD(_node, activeScene);

	return result;
}

bool Conditions::GameDirector::check(Framework::GameScript::ScriptExecution const& _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	auto* gd = TeaForGodEmperor::GameDirector::get();
	if (narrative.is_set())
	{
		bool ok = gd && narrative.get() == gd->is_narrative_active();
		anyOk |= ok;
		anyFailed |= !ok;
	}
	if (narrativeRequested.is_set())
	{
		bool ok = gd && narrativeRequested.get() == gd->is_narrative_requested();
		anyOk |= ok;
		anyFailed |= !ok;
	}
	if (activeScene.is_set())
	{
		bool ok = gd && activeScene.get() == gd->is_active_scene();
		anyOk |= ok;
		anyFailed |= !ok;
	}

	return anyOk && !anyFailed;
}
