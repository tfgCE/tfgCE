#include "gse_tutHubHighlight.h"

#include "..\..\loaders\hub\loaderHub.h"
#include "..\..\tutorials\tutorialSystem.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool TutHubHighlight::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	screen = _node->get_name_attribute(TXT("screen"), screen);
	widget = _node->get_name_attribute(TXT("widget"), widget);
	if (auto* attr = _node->get_attribute(TXT("draggable")))
	{
		draggable.set(attr->get_as_string());
	}

	if (auto* attr = _node->get_attribute(TXT("force")))
	{
		if (attr->get_as_string() == TXT("true"))
		{
			force = true;
		}
		else if (attr->get_as_string() == TXT("false"))
		{
			force = false;
		}
		else
		{
			error_loading_xml(_node, TXT("invalid \"force\" value"));
			result = false;
		}
	}

	noInteractions = _node->get_bool_attribute(TXT("noInteractions"), noInteractions);
	noInteractions = ! _node->get_bool_attribute(TXT("interactions"), ! noInteractions);

	allowDrops = _node->get_bool_attribute(TXT("allowDrops"), allowDrops);

	if (_node->get_bool_attribute(TXT("allowDropsOnly")))
	{
		allowDrops = true;
		noInteractions = false;
	}
	warn_loading_xml_on_assert(! (allowDrops && noInteractions), _node, TXT("\"allowDrops\" makes sense only if \"noInteractions\" is set"));

	noPulse = _node->get_bool_attribute(TXT("noPulse"), noPulse);
	noPulse = ! _node->get_bool_attribute(TXT("pulse"), !noPulse);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type TutHubHighlight::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (screen.is_valid() || widget.is_valid() || draggable.is_set())
	{
		TutorialSystem::get()->hub_highlight(screen, widget, draggable, noInteractions, allowDrops, noPulse);
	}
	if (force.is_set())
	{
		TutorialSystem::get()->force_hub_highlights(force.get());
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
