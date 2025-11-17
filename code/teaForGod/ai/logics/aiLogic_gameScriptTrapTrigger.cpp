#include "aiLogic_gameScriptTrapTrigger.h"

#include "..\..\game\game.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_hitIndicator.h"
#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\gameScript\gameScriptExecution.h"
#include "..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// variables
DEFINE_STATIC_NAME(interactiveDeviceId);

//

REGISTER_FOR_FAST_CAST(GameScriptTrapTrigger);

GameScriptTrapTrigger::GameScriptTrapTrigger(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData, LATENT_FUNCTION_VARIABLE(_executeFunction))
: base(_mind, _logicData, _executeFunction? _executeFunction : _executeFunction)
, data(fast_cast<GameScriptTrapTriggerData>(_logicData))
{
	an_assert(data);
}

GameScriptTrapTrigger::~GameScriptTrapTrigger()
{
}

void GameScriptTrapTrigger::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (!initialised)
	{
		initialisationWait -= _deltaTime;
		if (initialisationWait <= 0.0f)
		{
			initialisationWait = Random::get_float(0.1f, 0.3f);
			if (auto* imo = get_mind()->get_owner_as_modules_owner())
			{
				if (data->buttonIdVar.is_valid())
				{
					buttonHandler.initialise(imo, data->buttonIdVar);
				}

				if (buttonHandler.is_valid())
				{
					initialised = true;
				}
			}
		}
	}

	if (buttonHandler.is_valid())
	{
		buttonHandler.advance(_deltaTime);
		if (buttonHandler.has_button_been_pressed())
		{
			Framework::GameScript::ScriptExecution::trigger_execution_trap(data->triggerGameScriptTrap);
		}
	}
}

void GameScriptTrapTrigger::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);
}

LATENT_FUNCTION(GameScriptTrapTrigger::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[tea box] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM_NOT_USED(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_BEGIN_CODE();

	while (true)
	{
		LATENT_WAIT(Random::get_float(0.1f, 0.2f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//--

REGISTER_FOR_FAST_CAST(GameScriptTrapTriggerData);

GameScriptTrapTriggerData::GameScriptTrapTriggerData()
{

}

bool GameScriptTrapTriggerData::load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	triggerGameScriptTrap = _node->get_name_attribute_or_from_child(TXT("triggerGameScriptTrap"), triggerGameScriptTrap);
	buttonIdVar = _node->get_name_attribute_or_from_child(TXT("buttonIdVar"), buttonIdVar);

	return result;
}

bool GameScriptTrapTriggerData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}
