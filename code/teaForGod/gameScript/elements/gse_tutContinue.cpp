#include "gse_tutContinue.h"

#include "..\..\library\library.h"
#include "..\..\overlayInfo\overlayInfoSystem.h"
#include "..\..\overlayInfo\elements\oie_inputPrompt.h"
#include "..\..\overlayInfo\elements\oie_text.h"
#include "..\..\tutorials\tutorialSystem.h"

#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\text\localisedString.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

// game input definition
DEFINE_STATIC_NAME(tutorial);

// game input
DEFINE_STATIC_NAME(continue);

//

// localised strings
DEFINE_STATIC_NAME_STR(lsTutorialContinue, TXT("tutorials; continue"));
DEFINE_STATIC_NAME_STR(lsTutorialContinueToEnd, TXT("tutorials; continue to end"));

// ids
DEFINE_STATIC_NAME(tutContinue);

//

bool TutContinue::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	endTutorial = _node->get_bool_attribute_or_from_child_presence(TXT("endTutorial"), endTutorial);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type TutContinue::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Entered))
	{
		TutorialSystem::get()->block_input_for_continue();

		if (auto* ois = OverlayInfo::System::get())
		{
			{
				auto* t = new OverlayInfo::Elements::Text(LOC_STR(endTutorial? NAME(lsTutorialContinueToEnd) : NAME(lsTutorialContinue)));

				t->with_id(NAME(tutContinue));
				t->with_pulse();

				TutorialSystem::configure_oie_element(t, Rotator3(-10.0f, 0.0f, 0.0f));

				ois->add(t);
			}
			{
				auto* ip = new OverlayInfo::Elements::InputPrompt(NAME(tutorial), NAME(continue));

				ip->with_id(NAME(tutContinue));
				ip->with_pulse();
				ip->with_scale(0.7f);

				TutorialSystem::configure_oie_element(ip, Rotator3(-7.75f, 0.0f, 0.0f));

				ois->add(ip);
			}
		}
	}

	if (TutorialSystem::get()->has_continue_been_triggered())
	{
		if (auto* ois = OverlayInfo::System::get())
		{
			ois->deactivate(NAME(tutContinue));
		}
		TutorialSystem::get()->allow_input_after_continue();
		TutorialSystem::get()->consume_continue();
		return Framework::GameScript::ScriptExecutionResult::Continue;
	}
	else
	{
		return Framework::GameScript::ScriptExecutionResult::Repeat;
	}
}
