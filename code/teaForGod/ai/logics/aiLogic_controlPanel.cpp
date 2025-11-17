#include "aiLogic_controlPanel.h"

#include "..\..\modules\custom\mc_powerSource.h"
#include "..\..\modules\custom\health\mc_health.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayButton.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\display\displayText.h"
#include "..\..\..\framework\display\displayUtils.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\world\world.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// parameters
DEFINE_STATIC_NAME(controlDeviceId);

// messages
DEFINE_STATIC_NAME(deviceEnable);
DEFINE_STATIC_NAME(deviceDisable);

// sounds
DEFINE_STATIC_NAME(action);

//

REGISTER_FOR_FAST_CAST(ControlPanel);

ControlPanel::ControlPanel(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
, controlPanelData(fast_cast<ControlPanelData>(_logicData))
{
}

ControlPanel::~ControlPanel()
{
	if (display)
	{
		display->use_background(nullptr);
		display->set_on_update_display(this, nullptr);
	}
}

void ControlPanel::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	advanceDisplayDeltaTime = _deltaTime;

	bool inputProvided = false;
	if (auto* mind = get_mind())
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
		}
	}
	if (!inputProvided)
	{
		useInput = Framework::GameInputProxy(); // clear
	}
}

void ControlPanel::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);

	display = nullptr;
	if (auto* mind = get_mind())
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
			if (auto* displayModule = imo->get_custom<::Framework::CustomModules::Display>())
			{
				display = displayModule->get_display();
				display->set_on_advance_display(this,
					[this](Framework::Display* _display, float _deltaTime)
				{
					_display->process_input(useInput, _deltaTime);
				}
				);
			}
		}
	}
}

void ControlPanel::set_devices_enable(bool _enable)
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

LATENT_FUNCTION(ControlPanel::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, pageTask);
	
	auto * self = fast_cast<ControlPanel>(logic);

	LATENT_BEGIN_CODE();

	self->devices.clear();

	if (auto* imo = self->get_mind()->get_owner_as_modules_owner())
	{
		auto* world = imo->get_in_world();
		an_assert(world);

		if (auto* id = imo->get_variables().get_existing<int>(self->controlPanelData->idVar))
		{
			world->for_every_object_with_id(self->controlPanelData->idVar, *id,
				[self, imo](Framework::Object* _object)
			{
				if (_object != imo)
				{
					self->devices.push_back(SafePtr<Framework::IModulesOwner>(_object));
				}
				return false; // keep going on
			});
		}
	}

	while (true)
	{
		{
			::Framework::AI::LatentTaskInfoWithParams pageTaskInfo;
			pageTaskInfo.clear();
			if (self->requestedPage == ControlPanelPage::Main &&
				pageTaskInfo.propose(AI_LATENT_TASK_FUNCTION(main_page)))
			{
				ADD_LATENT_TASK_INFO_PARAM(pageTaskInfo, ::Framework::AI::Logic*, logic);
			}
			if (pageTaskInfo.is_proposed() &&
				pageTask.can_start(pageTaskInfo))
			{
				pageTask.start_latent_task(mind, pageTaskInfo);
			}

		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(ControlPanel::main_page)
{
	LATENT_SCOPE();

	LATENT_PARAM_NOT_USED(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic);
	LATENT_END_PARAMS();

	LATENT_VAR(RefCountObjectPtr<::Framework::DisplayText>, currentPowerOutputInfo);
	LATENT_VAR(RefCountObjectPtr<::Framework::DisplayText>, currentPowerOutputControl);
	LATENT_VAR(RefCountObjectPtr<::Framework::DisplayText>, objectIntegrityInfo);
	LATENT_VAR(RefCountObjectPtr<::Framework::DisplayText>, objectIntegrityControl);

	LATENT_VAR(float, maxPowerOutput);
	LATENT_VAR(Energy, maxHealth);

	LATENT_VAR(Energy, lastHealth);
	LATENT_VAR(float, timeSinceLastHealthChange);
	LATENT_VAR(bool, healthWarning);

	auto * self = fast_cast<ControlPanel>(logic);

	timeSinceLastHealthChange += LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	maxPowerOutput = 0.0f;
	maxHealth = Energy(0);

	Framework::DisplayUtils::clear_all(self->display);
	
	{
		int x = self->display->get_char_resolution().x - 3;
		int y = self->display->get_char_resolution().y - 1;

		{
			currentPowerOutputInfo = new Framework::DisplayText();
			currentPowerOutputInfo->set_text(self->controlPanelData->lsCurrentPowerOutput.get());
			currentPowerOutputInfo
				->no_scroll()
				->at(0, y)
				->size(x);
			self->display->add(currentPowerOutputInfo.get());

			currentPowerOutputControl = new Framework::DisplayText();
			currentPowerOutputControl
				->no_scroll()
				->at(x, y)
				->size(3);
			self->display->add(currentPowerOutputControl.get());
		}
		--y;

		{
			objectIntegrityInfo = new Framework::DisplayText();
			objectIntegrityInfo->set_text(self->controlPanelData->lsObjectIntegrity.get());
			objectIntegrityInfo
				->no_scroll()
				->at(0, y)
				->size(x);
			self->display->add(objectIntegrityInfo.get());

			auto* text = new Framework::DisplayDrawCommands::TextAt();
			text->text(self->controlPanelData->lsObjectIntegrity.get());
			text->at(0, y);
			text->length(x);
			self->display->add(text);

			objectIntegrityControl = new Framework::DisplayText();
			objectIntegrityControl
				->no_scroll()
				->at(x, y)
				->size(3);
			self->display->add(objectIntegrityControl.get());
		}
		--y;
		
	}


	while (true)
	{
		if (!self->display->is_updated_continuously())
		{
			LATENT_YIELD();
		}
		if (currentPowerOutputControl.get())
		{
			float newMaxNominalPowerOutput = 0.0f;
			float currentPowerOutput = 0.0f;
			for_every_ref(device, self->devices)
			{
				if (!device)
				{
					continue;
				}
				// we're fine just grabbing device's power source. any attached would have to go through device anyway
				if (auto* ps = device->get_custom<CustomModules::PowerSource>())
				{
					newMaxNominalPowerOutput += ps->get_nominal_power_output();
					currentPowerOutput += ps->get_current_power_output();
				}
			}
			maxPowerOutput = max(maxPowerOutput, newMaxNominalPowerOutput);
			tchar newInfo[5];
			int powerPT = maxPowerOutput != 0.0f ? (int)((100.0f * currentPowerOutput / maxPowerOutput) + 0.5f) : 0;
			powerPT = clamp(powerPT, 0, 999);
			/*
			if (powerPT < 100)
			{
				tsprintf(newInfo, 4, TXT("%02i%%"), powerPT);
			}
			else
			{
				tsprintf(newInfo, 4, TXT("%03i"), powerPT);
			}
			*/
			if (powerPT > 0 || self->controlPanelData->lsCurrentPowerOutputNone.get().is_empty())
			{
				tsprintf(newInfo, 4, TXT("%03i"), clamp((int)(currentPowerOutput + 0.5f), 0, 999));
			}
			else
			{
				tsprintf(newInfo, 4, TXT("   "));
				String const & ls = self->controlPanelData->lsCurrentPowerOutputNone.get();
				int off = 3 - ls.get_length();
				for (int c = 0; c < min(3, ls.get_length()); ++c)
				{
					newInfo[off + c] = ls[c];
				}
			}
			if (currentPowerOutputControl->get_text() != newInfo)
			{
				currentPowerOutputControl->set_ink_colour(powerPT <= 0 ? self->controlPanelData->objectIntegrityNoneColour : NP);
				currentPowerOutputInfo->set_ink_colour(powerPT <= 0 ? self->controlPanelData->objectIntegrityNoneColour : NP);
				currentPowerOutputControl->set_text(newInfo);
				currentPowerOutputControl->redraw(self->display);
				currentPowerOutputInfo->redraw(self->display);
			}
		}
		if (objectIntegrityControl.get())
		{
			Energy newMaxHealth = Energy(0);
			Energy currentHealth = Energy(0);
			for_every_ref(device, self->devices)
			{
				if (!device)
				{
					continue;
				}
				if (auto* h = device->get_custom<CustomModules::Health>())
				{
					newMaxHealth += h->get_max_health();
					currentHealth += max(Energy(0), min(h->get_max_health(), h->get_health()));
				}
				// check attached as they have seperate health modules
				{
					Concurrency::ScopedSpinLock lock(device->get_presence()->access_attached_lock());
					for_every_ptr(attached, device->get_presence()->get_attached())
					{
						if (auto* h = attached->get_custom<CustomModules::Health>())
						{
							newMaxHealth += h->get_max_health();
							currentHealth += max(Energy(0), min(h->get_max_health(), h->get_health()));
						}
					}
				}
			}
			maxHealth = max(maxHealth, newMaxHealth);
			tchar newInfo[4];
			if (currentHealth < lastHealth)
			{
				timeSinceLastHealthChange = 0.0f;
			}
			bool wasHealthWarning = healthWarning;
			healthWarning = timeSinceLastHealthChange < 1.0f;

			lastHealth = currentHealth;
			int healthPT = maxHealth != Energy(0) ? (int)((100.0f * currentHealth.as_float() / maxHealth.as_float()) + 0.5f) : 0;
			healthPT = clamp(healthPT, 0, 999);
			if (healthPT < 100 || self->controlPanelData->lsObjectIntegrityOK.get().is_empty())
			{
				tsprintf(newInfo, 4, TXT("%02i%%"), healthPT);
			}
			else
			{
				tsprintf(newInfo, 4, TXT("   "));
				String const & ls = self->controlPanelData->lsObjectIntegrityOK.get();
				int off = 3 - ls.get_length();
				for (int c = 0; c < min(3, ls.get_length()); ++c)
				{
					newInfo[off + c] = ls[c];
				}
			}
			if (objectIntegrityControl->get_text() != newInfo || (healthWarning ^ wasHealthWarning))
			{
				objectIntegrityControl->set_ink_colour(healthPT <= 0 || healthWarning? self->controlPanelData->objectIntegrityNoneColour : NP);
				objectIntegrityInfo->set_ink_colour(healthPT <= 0 || healthWarning? self->controlPanelData->objectIntegrityNoneColour : NP);
				objectIntegrityControl->set_text(newInfo);
				objectIntegrityControl->redraw(self->display);
				objectIntegrityInfo->redraw(self->display);
			}
		}
		LATENT_WAIT(0.1f);
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(ControlPanelData);

ControlPanelData::ControlPanelData()
{
	idVar = NAME(controlDeviceId);
}

ControlPanelData::~ControlPanelData()
{
}

bool ControlPanelData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	idVar = _node->get_name_attribute_or_from_child(TXT("idVar"), idVar);

	result &= lsEnable.load_from_xml(_node->first_child_named(TXT("lsEnable")), _lc, TXT("enable"));
	result &= lsDisable.load_from_xml(_node->first_child_named(TXT("lsDisable")), _lc, TXT("disable"));

	result &= lsNext.load_from_xml(_node->first_child_named(TXT("lsNext")), _lc, TXT("next"));
	result &= lsPrev.load_from_xml(_node->first_child_named(TXT("lsPrev")), _lc, TXT("prev"));

	result &= lsCurrentPowerOutput.load_from_xml(_node->first_child_named(TXT("lsCurrentPowerOutput")), _lc, TXT("current power output"));
	result &= lsCurrentPowerOutputNone.load_from_xml(_node->first_child_named(TXT("lsCurrentPowerOutputNone")), _lc, TXT("current power output none"));
	
	result &= lsObjectIntegrity.load_from_xml(_node->first_child_named(TXT("lsObjectIntegrity")), _lc, TXT("object integrity"));
	result &= lsObjectIntegrityOK.load_from_xml(_node->first_child_named(TXT("lsObjectIntegrityOK")), _lc, TXT("object integrity ok"));

	result &= currentPowerOutputNoneColour.load_from_xml(_node,TXT("currentPowerOutputNoneColour"));
	result &= objectIntegrityNoneColour.load_from_xml(_node, TXT("objectIntegrityNoneColour"));

	return result;
}

bool ControlPanelData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}