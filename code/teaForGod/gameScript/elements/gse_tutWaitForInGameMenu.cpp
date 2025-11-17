#include "gse_tutWaitForInGameMenu.h"

#include "..\..\overlayInfo\overlayInfoSystem.h"
#include "..\..\overlayInfo\elements\oie_text.h"
#include "..\..\tutorials\tutorialSystem.h"
#include "..\..\game\game.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

// localised strings
DEFINE_STATIC_NAME_STR(lsInGameInputPromptNonHandTracking, TXT("tutorials; in game menu; input prompt; non hand tracking"));
DEFINE_STATIC_NAME_STR(lsInGameInputPromptHandTracking, TXT("tutorials; in game menu; input prompt; hand tracking"));

// ids
DEFINE_STATIC_NAME(inGameMenuInputPrompt);

//

bool TutWaitForInGameMenu::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	useNormalInGameMenu = _node->get_bool_attribute_or_from_child_presence(TXT("useNormalInGameMenu"), useNormalInGameMenu);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type TutWaitForInGameMenu::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (useNormalInGameMenu && is_flag_set(_flags, Framework::GameScript::ScriptExecution::Entered))
	{
		TutorialSystem::get()->open_normal_in_game_menu_if_requested(true);
	}

	bool updatePrompt = is_flag_set(_flags, Framework::GameScript::ScriptExecution::Entered);
	if (auto* vr = VR::IVR::get())
	{
		if (vr->has_controllers_iteration_changed())
		{
			updatePrompt = true;
		}
	}
	if (updatePrompt)
	{
		if (auto* ois = OverlayInfo::System::get())
		{
			ois->deactivate(NAME(inGameMenuInputPrompt));
			{
				bool handTracking = false;
				if (auto* vr = VR::IVR::get())
				{
					handTracking |= vr->is_using_hand_tracking();
				}

				auto* t = new OverlayInfo::Elements::Text(LOC_STR(handTracking? NAME(lsInGameInputPromptHandTracking) : NAME(lsInGameInputPromptNonHandTracking)));

				t->with_id(NAME(inGameMenuInputPrompt));
				t->with_pulse();
				t->with_vertical_align(0.0f);

				TutorialSystem::configure_oie_element(t, Rotator3(-10.0f, 0.0f, 0.0f));

				ois->add(t);
			}
		}
	}

	if (auto* game = Game::get_as<Game>())
	{
		if (!game->is_in_game_menu_active())
		{
			return Framework::GameScript::ScriptExecutionResult::Repeat;
		}
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
