#include "gse_voiceoverSilence.h"

#include "..\..\sound\voiceoverSystem.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool VoiceoverSilence::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	actor = _node->get_name_attribute(TXT("actor"), actor);
	exceptOf.clear();
	{
		Name eo = _node->get_name_attribute(TXT("exceptOf"));
		if (eo.is_valid())
		{
			exceptOf.push_back(eo);
		}
	}
	for_every(node, _node->children_named(TXT("except")))
	{
		Name eo = node->get_name_attribute(TXT("of"));
		if (eo.is_valid())
		{
			exceptOf.push_back(eo);
		}
		else
		{
			error_loading_xml(node, TXT("missing \"of\": <except of=\"\"/>"));
			result = false;
		}
	}

	error_loading_xml_on_assert(!actor.is_valid() || exceptOf.is_empty(), result, _node, TXT("do not mix actor and exceptOf"));

	fadeOutTime.load_from_xml(_node, TXT("fadeOut"));

	return result;
}

bool VoiceoverSilence::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type VoiceoverSilence::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* vos = VoiceoverSystem::get())
	{
		if (actor.is_valid())
		{
			vos->silence(actor, fadeOutTime);
		}
		else
		{
			vos->silence(fadeOutTime, exceptOf);
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
