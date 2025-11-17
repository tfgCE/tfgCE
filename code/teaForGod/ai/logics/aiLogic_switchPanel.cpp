#include "aiLogic_switchPanel.h"

#include "..\..\modules\custom\mc_powerSource.h"
#include "..\..\modules\custom\health\mc_health.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayButton.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\display\displayText.h"
#include "..\..\..\framework\display\displayUtils.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleMovement.h"
#include "..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\world\world.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// parameters
DEFINE_STATIC_NAME(controlDeviceId);
DEFINE_STATIC_NAME(interactiveDeviceId);

// ai messages
DEFINE_STATIC_NAME(deviceEnable);
DEFINE_STATIC_NAME(deviceDisable);
DEFINE_STATIC_NAME(queryDeviceStatus);
DEFINE_STATIC_NAME(deviceStatus);

// ai message params
DEFINE_STATIC_NAME(isDeviceEnabled);
DEFINE_STATIC_NAME(who);

// sounds
DEFINE_STATIC_NAME(action);

//

REGISTER_FOR_FAST_CAST(SwitchPanel);

SwitchPanel::SwitchPanel(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
, switchPanelData(fast_cast<SwitchPanelData>(_logicData))
{
}

SwitchPanel::~SwitchPanel()
{
}

void SwitchPanel::advance(float _deltaTime)
{
	base::advance(_deltaTime);
}

void SwitchPanel::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);
}

void SwitchPanel::set_devices_enable(bool _enable)
{
	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		auto* world = imo->get_in_world();
		an_assert(world);

		for_every_ref(device, devices)
		{
			if (!device)
			{
				continue;
			}
			if (Framework::AI::Message* message = world->create_ai_message(_enable? NAME(deviceEnable) : NAME(deviceDisable)))
			{
				message->to_ai_object(device);
			}
		}
	}
}

void SwitchPanel::set_switches_enable(bool _enable)
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

LATENT_FUNCTION(SwitchPanel::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto * self = fast_cast<SwitchPanel>(logic);

	LATENT_BEGIN_CODE();

	self->devices.clear();
	self->switches.clear();

	if (auto* imo = self->get_mind()->get_owner_as_modules_owner())
	{
		auto* world = imo->get_in_world();
		an_assert(world);

		if (auto* id = imo->get_variables().get_existing<int>(self->switchPanelData->controlledIdVar))
		{
			world->for_every_object_with_id(self->switchPanelData->controlledIdVar, *id,
				[self, imo](Framework::Object* _object)
			{
				if (_object != imo)
				{
					self->devices.push_back(SafePtr<Framework::IModulesOwner>(_object));
					if (Framework::AI::Message* message = _object->get_in_world()->create_ai_message(NAME(queryDeviceStatus)))
					{
						message->access_param(NAME(who)).access_as<SafePtr<Framework::IModulesOwner>>() = imo;
						message->to_ai_object(_object);
					}
				}
				return false; // keep going on
			});
		}
		if (auto* id = imo->get_variables().get_existing<int>(self->switchPanelData->interactiveIdVar))
		{
			world->for_every_object_with_id(self->switchPanelData->interactiveIdVar, *id,
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

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(deviceStatus), [self](Framework::AI::Message const & _message)
		{
			if (auto* isDeviceEnabledPtr = _message.get_param(NAME(isDeviceEnabled)))
			{
				bool isDeviceEnabled = isDeviceEnabledPtr->get_as<bool>();
				self->set_switches_enable(isDeviceEnabled);
			}
		}
		);
	}

	while (true)
	{
		{
			for_every(sw, self->switches)
			{
				if (!sw->imo.get())
				{
					continue;
				}
				if (auto * mms = fast_cast<Framework::ModuleMovementSwitch>(sw->imo->get_movement()))
				{
					if (mms->is_active_at(1))
					{
						if (sw->activeAt.get(0) != 1)
						{
							self->set_devices_enable(true);
						}
						sw->activeAt = 1;
					}
					else if (mms->is_active_at(0))
					{
						if (sw->activeAt.get(1) != 0)
						{
							self->set_devices_enable(false);
						}
						sw->activeAt = 0;
					}
					else
					{
						sw->activeAt.clear();
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

REGISTER_FOR_FAST_CAST(SwitchPanelData);

SwitchPanelData::SwitchPanelData()
{
	controlledIdVar = NAME(controlDeviceId);
	interactiveIdVar = NAME(interactiveDeviceId);
}

SwitchPanelData::~SwitchPanelData()
{
}

bool SwitchPanelData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	controlledIdVar = _node->get_name_attribute_or_from_child(TXT("controlledIdVar"), controlledIdVar);
	interactiveIdVar = _node->get_name_attribute_or_from_child(TXT("interactiveIdVar"), interactiveIdVar);

	return result;
}

bool SwitchPanelData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}