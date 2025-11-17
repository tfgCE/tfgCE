#include "gse_showInputPrompt.h"

#include "..\..\library\library.h"
#include "..\..\overlayInfo\overlayInfoSystem.h"
#include "..\..\overlayInfo\elements\oie_inputPrompt.h"
#include "..\..\overlayInfo\elements\oie_text.h"
#include "..\..\tutorials\tutorialSystem.h"

#include "..\..\..\framework\library\usedLibraryStored.inl"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

// game input definition
DEFINE_STATIC_NAME(game);

// overlay element id
DEFINE_STATIC_NAME(inputPrompt);

//

bool ShowInputPrompt::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	id = _node->get_name_attribute(TXT("id"), id);
	text.load_from_xml(_node, _lc, id.to_char(), false); // id is something else here

	gameInputDefinition = _node->get_name_attribute(TXT("gameInputDefinition"), gameInputDefinition);
	{
		inputs.clear();
		String inputString = _node->get_string_attribute(TXT("input"));
		List<String> tokens;
		inputString.split(String::comma(), tokens);
		for_every(token, tokens)
		{
			inputs.push_back(Name(token->trim()));
		}
	}

	if (!gameInputDefinition.is_valid())
	{
		warn_loading_xml(_node, TXT("assuming: gameInputDefinition=\"game\""));
		gameInputDefinition = NAME(game);
	}
	error_loading_xml_on_assert(! inputs.is_empty(), result, _node, TXT("no \"input\" provided"));

	return result;
}

static void configure_oie_element(OverlayInfo::Element* e, Rotator3 offset)
{
	if (TutorialSystem::check_active())
	{
		TutorialSystem::configure_oie_element(e, offset);
	}
	else
	{
		float distance = 10.0f;
		e->with_location(OverlayInfo::Element::Relativity::RelativeToAnchor);
		e->with_rotation(OverlayInfo::Element::Relativity::RelativeToAnchor, offset);
		e->with_distance(distance);
	}
}

Framework::GameScript::ScriptExecutionResult::Type ShowInputPrompt::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Entered))
	{
		if (auto* ois = OverlayInfo::System::get())
		{
			Rotator3 offset(-10.0f, 0.0f, 0.0f);
			if (text.is_valid())
			{
				auto* t = new OverlayInfo::Elements::Text(text.get());

				t->with_id(NAME(inputPrompt));
				t->with_pulse();

				configure_oie_element(t, offset);
				offset = Rotator3(-7.75f, 0.0f, 0.0f);

				ois->add(t);
			}
			{
				auto* ip = new OverlayInfo::Elements::InputPrompt(gameInputDefinition, inputs);

				ip->with_id(NAME(inputPrompt));
				ip->with_pulse();
				ip->with_scale(0.7f);

				configure_oie_element(ip, offset);

				ois->add(ip);
			}
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
