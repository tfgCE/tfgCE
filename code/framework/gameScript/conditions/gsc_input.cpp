#include "gsc_input.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\..\core\vr\iVR.h"

#include "..\..\game\gameInput.h"
#include "..\..\game\gameInputDefinition.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Conditions;

//

// game input definition
DEFINE_STATIC_NAME(game);

//

bool Input::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	gameInputDefinition = _node->get_name_attribute(TXT("gameInputDefinition"), gameInputDefinition);
	button = _node->get_name_attribute(TXT("button"), button);

	if (!gameInputDefinition.is_valid())
	{
		warn_loading_xml(_node, TXT("assuming: gameInputDefinition=\"game\""));
		gameInputDefinition = NAME(game);
	}
	error_loading_xml_on_assert(button.is_valid(), result, _node, TXT("no \"button\" provided"));

	isAvailableCheck = _node->get_bool_attribute_or_from_child_presence(TXT("isAvailable"), isAvailableCheck);

	return result;
}

#define THRESHOLD 0.5f

bool Input::check(ScriptExecution const & _execution) const
{
	bool result = false;

	if (auto* gid = Framework::GameInputDefinitions::get_definition(gameInputDefinition))
	{
		Framework::GameInput input;
		input.use(gid);
		if (VR::IVR::get())
		{
			input.use(Framework::GameInputIncludeVR::All);
		}
		if (isAvailableCheck)
		{
			if (input.is_button_available(button))
			{
				result = true;
			}
		}
		else
		{
			if (input.get_button_state(button) >= THRESHOLD)
			{
				result = true;
			}
		}
	}

	return result;
}
