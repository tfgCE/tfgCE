#include "gse_voiceoverSpeak.h"

#include "..\..\sound\voiceoverSystem.h"

#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\libraryPrepareContext.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool VoiceoverSpeak::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	actor = _node->get_name_attribute(TXT("actor"), actor);
	sample.load_from_xml(_node, TXT("sample"), _lc);
	sample.load_from_xml(_node, TXT("line"), _lc);

	bool devLineExplicit = _node->get_bool_attribute_or_from_child_presence(TXT("devLine"), false);

	devLine = _node->get_text();

	shouldWait = _node->get_bool_attribute_or_from_child_presence(TXT("wait"), shouldWait);
	shouldWait = ! _node->get_bool_attribute_or_from_child_presence(TXT("dontWait"), ! shouldWait);

	audioDuck.load_from_xml(_node, TXT("audioDuck"));

	error_loading_xml_on_assert(devLine.is_empty() || devLineExplicit, result, _node, TXT("dev text provided as node's content but no <devLine/>"));
	error_loading_xml_on_assert(sample.is_name_valid() || (!devLine.is_empty() && devLineExplicit), result, _node, TXT("no sample/line provided (or at least content of the node)"));

	return result;
}

bool VoiceoverSpeak::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= sample.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

void VoiceoverSpeak::load_on_demand_if_required()
{
	base::load_on_demand_if_required();
	if (auto* s = sample.get())
	{
		s->load_on_demand_if_required();
	}
}

Framework::GameScript::ScriptExecutionResult::Type VoiceoverSpeak::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* vos = VoiceoverSystem::get())
	{
		if (_flags & Framework::GameScript::ScriptExecution::Flag::Entered)
		{
			if (sample.get())
			{
				vos->speak(actor, audioDuck, sample.get());
			}
			else
			{
				vos->speak(actor, audioDuck, devLine);
			}
		}
		if (shouldWait &&
			((sample.get() && vos->is_speaking(actor, sample.get())) ||
			 (! sample.get() && vos->is_speaking(actor, devLine))))
		{
			return Framework::GameScript::ScriptExecutionResult::Repeat;
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
