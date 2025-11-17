#include "gse_tutHubSelect.h"

#include "..\..\loaders\hub\loaderHubScreen.h"
#include "..\..\loaders\hub\loaderHubWidget.h"
#include "..\..\game\game.h"
#include "..\..\tutorials\tutorialSystem.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool TutHubSelect::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	screen = _node->get_name_attribute(TXT("screen"), screen);
	widget = _node->get_name_attribute(TXT("widget"), widget);

	error_loading_xml_on_assert(screen.is_valid(), result, _node, TXT("no \"screen\""));
	error_loading_xml_on_assert(widget.is_valid(), result, _node, TXT("no \"widget\""));

	if (auto* attr = _node->get_attribute(TXT("draggable")))
	{
		draggable.set(attr->get_as_string());
	}
	else
	{
		error_loading_xml(_node, TXT("no \"draggable\""));
		result = false;
	}

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type TutHubSelect::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (TeaForGodEmperor::TutorialSystem::check_active())
	{
		if (auto* hub = TeaForGodEmperor::TutorialSystem::get()->get_active_hub())
		{
			if (auto* s = hub->get_screen(screen))
			{
				if (auto* w = s->get_widget(widget))
				{
					if (auto* d = w->tut_get_draggable(draggable))
					{
						hub->select(w, d);
					}
				}
			}
		}

	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
