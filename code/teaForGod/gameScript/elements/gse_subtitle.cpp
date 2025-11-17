#include "gse_subtitle.h"

#include "..\..\sound\subtitleSystem.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool Elements::Subtitle::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	useTextId = _node->get_name_attribute_or_from_child(TXT("useTextId"), useTextId);
	text.load_from_xml(_node, _lc, true);
	colour.load_from_xml(_node, TXT("colour"));
	backgroundColour.load_from_xml(_node, TXT("backgroundColour"));
	time = _node->get_float_attribute_or_from_child(TXT("time"), time);

	error_loading_xml_on_assert(text.is_valid() || useTextId.is_valid(), result, _node, TXT("provide text or useTextId"));

	return result;
}

bool Elements::Subtitle::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Elements::Subtitle::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* ss = SubtitleSystem::get())
	{
		SubtitleId sid = NONE;
		if (useTextId.is_valid())
		{
			sid = ss->add(useTextId, NP, true, colour, backgroundColour);
		}
		else
		{
			sid = ss->add(text.get(), NP, true, colour, backgroundColour);
		}
		ss->remove(sid, time);
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
