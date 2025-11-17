#include "aiLogic_solarFlower.h"

#include "..\..\utils.h"

#include "..\..\modules\custom\mc_powerSource.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\world\environment.h"
#include "..\..\..\framework\world\room.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

DEFINE_STATIC_NAME(deviceOrderedState); // hardcoded - state of a device as ordered by user
DEFINE_STATIC_NAME(deviceRequestedState); // hardcoded - requested state of a device as possible
DEFINE_STATIC_NAME(deviceTargetState); // hardcoded - target state of a device
DEFINE_STATIC_NAME(solarFlowerActive);
DEFINE_STATIC_NAME(solarFlowerAngleFromVertical);
DEFINE_STATIC_NAME(solarPanelTowards);
DEFINE_STATIC_NAME(deviceEnable);
DEFINE_STATIC_NAME(deviceDisable);
DEFINE_STATIC_NAME(queryDeviceStatus);
DEFINE_STATIC_NAME(deviceStatus);
DEFINE_STATIC_NAME(isDeviceEnabled);
DEFINE_STATIC_NAME(testDeviceEnable);
DEFINE_STATIC_NAME(testDeviceDisable);
DEFINE_STATIC_NAME(solarPanelDir);
DEFINE_STATIC_NAME(who);

// variables
DEFINE_STATIC_NAME(mapIcon);
DEFINE_STATIC_NAME(mapIconFolded);
DEFINE_STATIC_NAME(mapIconUnfolded);

//

REGISTER_FOR_FAST_CAST(SolarFlower);

SolarFlower::SolarFlower(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	solarFlowerActiveVarID = NAME(solarFlowerActive);
	solarFlowerAngleFromVerticalVarID = NAME(solarFlowerAngleFromVertical);
	solarPanelTowardsVarID = NAME(solarPanelTowards);
	mapIconUnfoldedVarID = NAME(mapIconUnfolded);
	mapIconFoldedVarID = NAME(mapIconFolded);
}

SolarFlower::~SolarFlower()
{
}

void SolarFlower::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (isValid)
	{
		if (auto* solarFlower = get_mind()->get_owner_as_modules_owner())
		{
			auto & vars = solarFlower->access_variables();
			if (!deviceTargetStateVar.is_valid())
			{
				deviceTargetStateVar = vars.find<float>(NAME(deviceTargetState));
			}
			if (!deviceOrderedStateVar.is_valid())
			{
				deviceOrderedStateVar = vars.find<float>(NAME(deviceOrderedState));
			}			
			if (!deviceRequestedStateVar.is_valid())
			{
				deviceRequestedStateVar = vars.find<float>(NAME(deviceRequestedState));
			}			
			if (!solarFlowerActiveVar.is_valid())
			{
				solarFlowerActiveVar = vars.find<float>(solarFlowerActiveVarID);
			}
			if (!solarFlowerAngleFromVerticalVar.is_valid())
			{
				solarFlowerAngleFromVerticalVar = vars.find<float>(solarFlowerAngleFromVerticalVarID);
			}
			if (!solarPanelTowardsVar.is_valid())
			{
				solarPanelTowardsVar = vars.find<Vector3>(solarPanelTowardsVarID);
			}
			if (!mapIconVar.is_valid())
			{
				mapIconVar = vars.find<Framework::LibraryName>(NAME(mapIcon));
			}
			if (!mapIconUnfoldedVar.is_valid())
			{
				mapIconUnfoldedVar = vars.find<Framework::LibraryName>(mapIconUnfoldedVarID);
			}
			if (!mapIconFoldedVar.is_valid())
			{
				mapIconFoldedVar = vars.find<Framework::LibraryName>(mapIconFoldedVarID);
			}

			requestedActive = deviceRequestedStateVar.get<float>();
			powerActive = blend_to_using_time(powerActive, requestedActive, magic_number 0.5f, _deltaTime);

			if (requestedActive > 0.0f)
			{
				Vector3 lightDir = Utils::get_light_dir(solarFlower);
				if (!lightDir.is_zero())
				{
					Vector3 lightFrom = -lightDir;
					lightFrom = solarFlower->get_presence()->get_os_to_ws_transform().vector_to_local(lightFrom);
					if (Vector3::dot(solarPanelTowards.normal(), lightFrom) < 0.995f) // if has to turn
					{
						solarPanelTowards = lightFrom * 1000.0f;
						angleFromVertical = clamp(acos_deg(clamp(lightFrom.z, 0.0f, 1.0f)) / 90.0f, 0.0f, 1.0f);
					}
				}
			}

			deviceTargetStateVar.access<float>() = requestedActive;
			solarFlowerActiveVar.access<float>() = requestedActive;
			solarFlowerAngleFromVerticalVar.access<float>() = requestedActive * angleFromVertical;
			solarPanelTowardsVar.access<Vector3>() = solarPanelTowards;

			Framework::LibraryName& mapIcon = mapIconVar.access<Framework::LibraryName>();
			Framework::LibraryName const & mapIconUnfolded = mapIconUnfoldedVar.get<Framework::LibraryName>();
			Framework::LibraryName const & mapIconFolded = mapIconFoldedVar.get<Framework::LibraryName>();
			mapIcon = requestedActive > 0.0f ? mapIconUnfolded : mapIconFolded;
		}
	}
}

void SolarFlower::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);

	isValid = true;
}

LATENT_FUNCTION(SolarFlower::update_power_source)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	auto * solarFlower = fast_cast<SolarFlower>(mind->get_logic());

	LATENT_BEGIN_CODE();

	while (true)
	{
		if (auto * imo = mind->get_owner_as_modules_owner())
		{
			float nominalPowerSource = 0.0f;
			float possiblePowerSource = 0.0f;
			float currentPowerSource = 0.0f;
			if (auto* p = imo->get_presence())
			{
				Concurrency::ScopedSpinLock lock(p->access_attached_lock());
				for_every_ptr(attachedIMO, p->get_attached())
				{
					if (auto* ps = attachedIMO->get_custom<CustomModules::PowerSource>())
					{
						nominalPowerSource += ps->get_nominal_power_output();
						possiblePowerSource += ps->get_possible_power_output();
						currentPowerSource += ps->get_current_power_output();
					}
				}
			}

			if (auto* ps = imo->get_custom<CustomModules::PowerSource>())
			{
				ps->set_nominal_power_output(nominalPowerSource);
				ps->set_possible_power_output(possiblePowerSource);
				ps->set_current_power_output(currentPowerSource * solarFlower->powerActive);
			}
		}
		LATENT_WAIT(Random::get_float(0.1f, 0.3f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(SolarFlower::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, updatePowerSourceTask);
	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto * solarFlower = fast_cast<SolarFlower>(logic);

	LATENT_BEGIN_CODE();
	
	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(deviceEnable), [solarFlower](Framework::AI::Message const & _message)
		{
			solarFlower->orderedActive = true;
		}
		);
		messageHandler.set(NAME(deviceDisable), [solarFlower](Framework::AI::Message const & _message)
		{
			solarFlower->orderedActive = false;
		}
		);

		messageHandler.set(NAME(testDeviceEnable), [solarFlower](Framework::AI::Message const & _message)
		{
			solarFlower->orderedActive = true;
		}
		);
		messageHandler.set(NAME(testDeviceDisable), [solarFlower](Framework::AI::Message const & _message)
		{
			solarFlower->orderedActive = false;
		}
		);

		messageHandler.set(NAME(queryDeviceStatus), [solarFlower](Framework::AI::Message const & _message)
		{
			if (auto* who = _message.get_param(NAME(who)))
			{
				auto* whoImo = who->get_as<SafePtr<Framework::IModulesOwner>>().get();
				if (auto * msg = _message.create_response(NAME(deviceStatus)))
				{
					msg->to_ai_object(whoImo);
					msg->access_param(NAME(isDeviceEnabled)).access_as<bool>() = solarFlower->orderedActive;
				}
			}
		}
		);
	}

	{	// setup and start look at task
		::Framework::AI::LatentTaskInfoWithParams updatePowerSourceTaskInfo;
		updatePowerSourceTaskInfo.clear();
		updatePowerSourceTaskInfo.propose(update_power_source);
		updatePowerSourceTask.start_latent_task(mind, updatePowerSourceTaskInfo);
	}

	LATENT_WAIT(Random::get_float(0.1f, 0.6f));

	while (true)
	{
		solarFlower->sunAvailable = Utils::get_sun(solarFlower->get_mind()->get_owner_as_modules_owner());
		solarFlower->deviceOrderedStateVar.access<float>() = solarFlower->orderedActive? 1.0f : 0.0f;
		if (solarFlower->orderedActive && solarFlower->sunAvailable > 0.1f) // open early, close late
		{
			solarFlower->deviceRequestedStateVar.access<float>() = 1.0f;
		}
		else
		{
			solarFlower->deviceRequestedStateVar.access<float>() = 0.0f;
		}

		LATENT_WAIT(Random::get_float(0.1f, 0.5f));
	}

	LATENT_ON_BREAK();
	//
	updatePowerSourceTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(SolarFlowerData);

SolarFlowerData::SolarFlowerData()
{
}

SolarFlowerData::~SolarFlowerData()
{
}

bool SolarFlowerData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool SolarFlowerData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
