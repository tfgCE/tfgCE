#include "aiLogic_tunnelLight.h"

#include "..\..\modules\custom\mc_emissiveControl.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\custom\mc_lightSources.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define AUTO_TEST

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// parameters
DEFINE_STATIC_NAME(switchCheckInterval);
DEFINE_STATIC_NAME(switchEmissive);
DEFINE_STATIC_NAME(switchLight);
DEFINE_STATIC_NAME(lightSocket);
DEFINE_STATIC_NAME(switchOnAt);
DEFINE_STATIC_NAME(switchOffAt);

//

REGISTER_FOR_FAST_CAST(TunnelLight);

TunnelLight::TunnelLight(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

TunnelLight::~TunnelLight()
{
}

void TunnelLight::advance(float _deltaTime)
{
	base::advance(_deltaTime);
}

void TunnelLight::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);

	switchCheckInterval = _parameters.get_value<Range>(NAME(switchCheckInterval), switchCheckInterval);
	switchEmissive = _parameters.get_value<Name>(NAME(switchEmissive), switchEmissive);
	switchLight = _parameters.get_value<Name>(NAME(switchLight), switchLight);
	lightSocket = _parameters.get_value<Name>(NAME(lightSocket), lightSocket);
	switchOnAt = _parameters.get_value<Vector3>(NAME(switchOnAt), switchOnAt);
	switchOffAt = _parameters.get_value<Vector3>(NAME(switchOffAt), switchOffAt);
}

LATENT_FUNCTION(TunnelLight::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai tunnel light] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(Random::Generator, rg);
	LATENT_VAR(Segment, switchSeg);

	auto * self = fast_cast<TunnelLight>(logic);
	auto * imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	ai_log(self, TXT("tunnel light, hello!"));

	switchSeg = Segment(self->switchOnAt, self->switchOffAt);

	self->lightLocOS = Vector3::zero;
	if (self->lightSocket.is_valid())
	{
		if (auto* a = imo->get_appearance())
		{
			self->lightLocOS = a->calculate_socket_os(self->lightSocket).get_translation();
		}
	}

	while (true)
	{
		if (imo)
		{
			bool shouldBeOn = false;

			if (auto* p = imo->get_presence())
			{
				Vector3 at = p->get_placement().location_to_world(self->lightLocOS);

				float t = switchSeg.get_t_not_clamped(at);
				shouldBeOn = t >= 0.0f && t <= 1.0f;
			}

			if (shouldBeOn != self->lightOn)
			{
				self->lightOn = shouldBeOn;

				if (self->switchEmissive.is_valid())
				{
					if (auto* e = imo->get_custom<CustomModules::EmissiveControl>())
					{
						if (shouldBeOn)
						{
							e->emissive_activate(self->switchEmissive);
						}
						else
						{
							e->emissive_deactivate(self->switchEmissive);
						}
					}
				}

				if (self->switchLight.is_valid())
				{
					if (auto* ls = imo->get_custom<Framework::CustomModules::LightSources>())
					{
						if (shouldBeOn)
						{
							ls->add(self->switchLight, true);
						}
						else
						{
							ls->remove(self->switchLight);
						}
					}
				}
			}
		}
		if (! self->switchCheckInterval.is_zero() &&
			! self->switchCheckInterval.is_empty())
		{
			LATENT_WAIT(rg.get_float(self->switchCheckInterval));
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

