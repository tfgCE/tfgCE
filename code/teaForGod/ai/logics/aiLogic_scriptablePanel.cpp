#include "aiLogic_scriptablePanel.h"

#include "..\..\modules\custom\mc_emissiveControl.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\gameScript\gameScriptExecution.h"
#include "..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
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

// parameters
DEFINE_STATIC_NAME(interactiveDeviceId);

// var
DEFINE_STATIC_NAME(enabled);
DEFINE_STATIC_NAME(triggerGameScriptTrap);
DEFINE_STATIC_NAME(readyColour);
DEFINE_STATIC_NAME(activeColour);

// emissive
DEFINE_STATIC_NAME(active);
DEFINE_STATIC_NAME(busy);
DEFINE_STATIC_NAME(ready);

//

REGISTER_FOR_FAST_CAST(ScriptablePanel);

ScriptablePanel::ScriptablePanel(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
, scriptablePanelData(fast_cast<ScriptablePanelData>(_logicData))
{
}

ScriptablePanel::~ScriptablePanel()
{
}

void ScriptablePanel::advance(float _deltaTime)
{
	base::advance(_deltaTime);
}

void ScriptablePanel::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);
}

void ScriptablePanel::set_switches_enable(bool _enable)
{
	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		an_assert(imo->get_in_world());

		for_every(sw, switches)
		{
			if (!sw->imo.get())
			{
				continue;
			}
			if (auto * mms = fast_cast<Framework::ModuleMovementSwitch>(sw->imo->get_movement()))
			{
				mms->go_to(_enable ? 1 : 0);
			}
		}
	}
}

#define S_DISABLED 0
#define S_ENABLED 1
#define S_ACTIVE 2
#define S_BUSY 3

LATENT_FUNCTION(ScriptablePanel::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM_NOT_USED(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(int, state);

	auto * self = fast_cast<ScriptablePanel>(logic);

	LATENT_BEGIN_CODE();

	state = NONE;

	self->switches.clear();

	if (auto* imo = self->get_mind()->get_owner_as_modules_owner())
	{
		auto* world = imo->get_in_world();
		an_assert(world);

		if (auto* id = imo->get_variables().get_existing<int>(self->scriptablePanelData->interactiveIdVar))
		{
			world->for_every_object_with_id(self->scriptablePanelData->interactiveIdVar, *id,
				[self, imo](Framework::Object* _object)
			{
				if (_object != imo)
				{
					Switch sw;
					sw.imo = SafePtr<Framework::IModulesOwner>(_object);
					self->switches.push_back(sw);
				}
				return false; // keep going on
			});
		}
	}

	while (true)
	{
		{
			int newState = state;
			bool isEnabled = false;
			if (auto* imo = self->get_mind()->get_owner_as_modules_owner())
			{
				isEnabled = imo->get_variables().get_value(NAME(enabled), true);
			}
			if (!isEnabled)
			{
				newState = S_DISABLED;
			}
			else
			{
				if (newState == S_DISABLED)
				{
					newState = S_ENABLED;
				}
				bool allInactive = true;
				for_every(sw, self->switches)
				{
					if (!sw->imo.get())
					{
						continue;
					}
					if (auto* mms = fast_cast<Framework::ModuleMovementSwitch>(sw->imo->get_movement()))
					{
						if (mms->is_active_at(1))
						{
							allInactive = false;
						}
						if (mms->is_active_at(1))
						{
							if (sw->activeAt.get(0) != 1)
							{
								newState = S_ACTIVE;
								if (auto* imo = self->get_mind()->get_owner_as_modules_owner())
								{
									Name trapName = imo->get_variables().get_value(NAME(triggerGameScriptTrap), Name::invalid());
									if (trapName.is_valid())
									{
										Framework::GameScript::ScriptExecution::trigger_execution_trap(trapName);
									}
								}
							}
							sw->activeAt = 1;
						}
						else
						{
							sw->activeAt.clear();
						}
					}
				}
				if (allInactive)
				{
					newState = S_ENABLED;
				}
			}
			if (state != newState)
			{
				state = newState;
				for_every(sw, self->switches)
				{
					if (!sw->imo.get())
					{
						continue;
					}
					if (auto* ec = sw->imo->get_custom<CustomModules::EmissiveControl>())
					{
						if (auto* imo = self->get_mind()->get_owner_as_modules_owner())
						{
							if (auto* c = imo->get_variables().get_existing<Colour>(NAME(readyColour)))
							{
								ec->emissive_access(NAME(ready)).set_colour(*c);
							}
							if (auto* c = imo->get_variables().get_existing<Colour>(NAME(activeColour)))
							{
								ec->emissive_access(NAME(active)).set_colour(*c);
							}
						}
						if (state == S_DISABLED)
						{
							ec->emissive_deactivate(NAME(ready));
							ec->emissive_deactivate(NAME(active));
							ec->emissive_deactivate(NAME(busy));
						}
						if (state == S_ENABLED)
						{
							ec->emissive_activate(NAME(ready));
							ec->emissive_deactivate(NAME(active));
							ec->emissive_deactivate(NAME(busy));
						}
						if (state == S_ACTIVE)
						{
							ec->emissive_activate(NAME(ready));
							ec->emissive_activate(NAME(active));
							ec->emissive_deactivate(NAME(busy));
						}
						if (state == S_BUSY)
						{
							ec->emissive_deactivate(NAME(ready));
							ec->emissive_deactivate(NAME(active));
							ec->emissive_activate(NAME(busy));
						}
					}
				}
			}
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(ScriptablePanelData);

ScriptablePanelData::ScriptablePanelData()
{
	interactiveIdVar = NAME(interactiveDeviceId);
}

ScriptablePanelData::~ScriptablePanelData()
{
}

bool ScriptablePanelData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	interactiveIdVar = _node->get_name_attribute_or_from_child(TXT("interactiveIdVar"), interactiveIdVar);

	return result;
}

bool ScriptablePanelData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}