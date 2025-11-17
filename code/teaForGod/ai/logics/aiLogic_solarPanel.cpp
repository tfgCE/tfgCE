#include "aiLogic_solarPanel.h"

#include "..\..\utils.h"

#include "..\..\modules\custom\mc_powerSource.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

DEFINE_STATIC_NAME(solarPanelDir)

//

REGISTER_FOR_FAST_CAST(SolarPanel);

SolarPanel::SolarPanel(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

SolarPanel::~SolarPanel()
{
}

LATENT_FUNCTION(SolarPanel::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_BEGIN_CODE();

	while (true)
	{
		if (auto* solarPanel = mind->get_owner_as_modules_owner())
		{
			if (auto* ps = solarPanel->get_custom<CustomModules::PowerSource>())
			{
				PERFORMANCE_GUARD_LIMIT(0.005f, TXT("solar panel checks"));
				float sunAvailable = Utils::get_sun(solarPanel);
				Vector3 lightDirWS = Utils::get_light_dir(solarPanel);

				Vector3 panelDirOS = Vector3::zAxis;
				if (solarPanel->get_appearance()->has_socket(NAME(solarPanelDir)))
				{
					panelDirOS = solarPanel->get_appearance()->calculate_socket_os(NAME(solarPanelDir)).get_axis(Axis::X);
				}
				Vector3 panelDirWS = solarPanel->get_presence()->get_placement().vector_to_world(panelDirOS);

				float atLight = clamp(Vector3::dot(-lightDirWS, panelDirWS), 0.0f, 1.0f);

				float nominalPower = ps->get_nominal_power_output();
				float possiblePower = nominalPower * sunAvailable;
				float currentPower = possiblePower * atLight;

				ps->set_possible_power_output(possiblePower);
				ps->set_current_power_output(currentPower);
			}
		}
		LATENT_WAIT(Random::get_float(0.1f, 0.5f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

