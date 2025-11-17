#include "gse_waitForHub.h"

#include "..\..\loaders\hub\loaderHub.h"
#include "..\..\loaders\hub\loaderHubWidget.h"
#include "..\..\tutorials\tutorialSystem.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool WaitForHub::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	if (auto* attr = _node->get_attribute(TXT("active")))
	{
		active = attr->get_as_bool();
	}

	screenOpen = _node->get_name_attribute(TXT("screenOpen"), screenOpen);

	screenClick = _node->get_name_attribute(TXT("screenClick"), screenClick);
	widgetClick = _node->get_name_attribute(TXT("widgetClick"), widgetClick);

	screenClick = _node->get_name_attribute(TXT("screen"), screenClick);
	widgetClick = _node->get_name_attribute(TXT("widget"), widgetClick);
	draggableSelect = _node->get_name_attribute(TXT("draggableSelect"), draggableSelect);

	if (draggableSelect.is_valid())
	{
		draggableSelectTutHubId.set(draggableSelect.to_string());
	}

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type WaitForHub::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (active.is_set())
	{
		if ((active.get() && TutorialSystem::get_active_hub()) ||
			(!active.get() && !TutorialSystem::get_active_hub()))
		{
			return Framework::GameScript::ScriptExecutionResult::Continue;
		}
		else
		{
			return Framework::GameScript::ScriptExecutionResult::Repeat;
		}
	}
	if (screenOpen.is_valid())
	{
		if (auto* hub = TutorialSystem::get_active_hub())
		{
			if (hub->get_screen(screenOpen))
			{
				return Framework::GameScript::ScriptExecutionResult::Continue;
			}
		}
		return Framework::GameScript::ScriptExecutionResult::Repeat;
	}
	if (screenClick.is_valid() || widgetClick.is_valid())
	{
		if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Flag::Entered))
		{
			TutorialSystem::get()->hub_highlight(screenClick, widgetClick, draggableSelectTutHubId);
		}
		
		if (draggableSelect.is_valid())
		{
			if (auto* hub = TutorialSystem::get_active_hub())
			{
				if (auto* s = hub->get_selected_draggable())
				{
					if (auto* d = s->data.get())
					{
						if (d->tutorialHubId == draggableSelect)
						{
							TutorialSystem::get()->clear_hub_highlight(screenClick, widgetClick, draggableSelectTutHubId);
							return Framework::GameScript::ScriptExecutionResult::Continue;
						}
					}
				}
			}
		}
		else
		{
			if (TutorialSystem::has_clicked_hub(screenClick, widgetClick)) // uses tutorial system but may be used without any tutorial active
			{
				TutorialSystem::get()->clear_hub_highlight(screenClick, widgetClick, draggableSelectTutHubId);
				TutorialSystem::clear_clicked_hub();
				return Framework::GameScript::ScriptExecutionResult::Continue;
			}
		}
		return Framework::GameScript::ScriptExecutionResult::Repeat;
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
