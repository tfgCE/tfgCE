#include "aiLogicTask_shield.h"

#include "..\..\..\game\damage.h"
#include "..\..\..\game\playerSetup.h"

#include "..\..\..\modules\custom\health\mc_health.h"

#include "..\..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\ai\aiTaskHandle.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai message name
DEFINE_STATIC_NAME(dealtDamage);

// ai message params
DEFINE_STATIC_NAME(damage);
DEFINE_STATIC_NAME(damageUnadjusted);
DEFINE_STATIC_NAME(damageSource);

// shader params
DEFINE_STATIC_NAME(shieldHit);
DEFINE_STATIC_NAME(shieldHealthPt);
DEFINE_STATIC_NAME(shieldExtraOverrideColour);
DEFINE_STATIC_NAME(shieldExtraAlpha);
DEFINE_STATIC_NAME(shieldExtraAlphaAdd);

//

LATENT_FUNCTION(TeaForGodEmperor::AI::Logics::Tasks::shield)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("shield"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);
	LATENT_VAR(float, shieldTarget);
	LATENT_VAR(float, shieldState);
	LATENT_VAR(float, shieldStatePt);

	LATENT_VAR(float, extraPt);
	LATENT_VAR(float, extraActive);
	// setup
	LATENT_VAR(Colour, extraColour);
	LATENT_VAR(float, extraAlpha);
	LATENT_VAR(float, extraLowLength);
	LATENT_VAR(float, extraCriticalLength);

	auto * imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	shieldStatePt = 0.0f;
	extraPt = 0.0f;
	extraActive = 0.0f;

	extraColour = Colour::white;
	extraAlpha = 1.0f;
	extraLowLength = 1.0f;
	extraCriticalLength = 0.5f;

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(dealtDamage), [mind, &shieldTarget](Framework::AI::Message const & _message)
		{
			bool allowShieldTarget = true;
			if (auto* damageUnadjusted = _message.get_param(NAME(damageUnadjusted)))
			{
				if (auto* damage = _message.get_param(NAME(damage)))
				{
					if (damage->get_as<Damage>().damage < damageUnadjusted->get_as<Damage>().damage.halved())
					{
						allowShieldTarget = false;
					}
				}
			}
			if (auto * source = _message.get_param(NAME(damageSource)))
			{
				if (auto* damageInstigator = source->get_as<SafePtr<Framework::IModulesOwner>>().get())
				{
					if (damageInstigator == mind->get_owner_as_modules_owner())
					{
						allowShieldTarget = false;
					}
				}
			}
			if (allowShieldTarget)
			{
				shieldTarget = 1.0f;
			}
		}
		);
	}

	while (true)
	{
		{
			if (shieldTarget > 0.0f)
			{
				shieldState = blend_to_using_speed_based_on_time(shieldState, shieldTarget, 0.05f, LATENT_DELTA_TIME);
				if (shieldState >= shieldTarget)
				{
					shieldTarget = 0.0f;
				}
			}
			else
			{
				shieldState = blend_to_using_speed_based_on_time(shieldState, shieldTarget, 0.5f, LATENT_DELTA_TIME);
			}

			if (auto* h = imo->get_custom<CustomModules::Health>())
			{
				float target = clamp(h->get_health().as_float() / max(0.01f, h->get_max_health().as_float()), 0.0f, 1.0f);
				shieldStatePt = blend_to_using_speed_based_on_time(shieldStatePt, target, 0.5f, LATENT_DELTA_TIME);
				{
					bool extraInUse = false;
					float extraPtZeroAt = 0.1f;
					if (PlayerPreferences::are_currently_flickering_lights_allowed())
					{
						if (target < 0.125f)
						{
							extraPt += LATENT_DELTA_TIME / extraCriticalLength;
							extraPtZeroAt = 0.1f / extraCriticalLength;
							extraInUse = true;
						}
						else if (target < 0.25f)
						{
							extraPt += LATENT_DELTA_TIME / extraLowLength;
							extraPtZeroAt = 0.1f / extraLowLength;
							extraInUse = true;
						}
						else if (extraActive < 0.1f)
						{
							extraPt = 0.0f;
						}
						else
						{
							extraPt += LATENT_DELTA_TIME / extraLowLength;
						}
						extraPt = mod(extraPt, 1.0f);
					}
					else
					{
						extraPt = 0.0f;
					}

					float activeTarget = extraInUse ? (remap_value(abs(extraPt - 0.5f), 0.0f, extraPtZeroAt, 1.0f, 0.0f, true)) : (0.0f);
					activeTarget *= 0.75f;
					extraActive = blend_to_using_speed_based_on_time(extraActive, activeTarget, 0.1f, LATENT_DELTA_TIME);
				}
			}


			if (auto* appearance = imo->get_appearance())
			{
				auto & mi = appearance->access_mesh_instance();
				for_count(int, i, mi.get_material_instance_count())
				{
					if (auto* mat = appearance->access_mesh_instance().access_material_instance(i))
					{
						if (auto* spi = mat->get_shader_program_instance())
						{
							{
								int idx = spi->get_shader_program()->find_uniform_index(NAME(shieldHit));
								if (idx != NONE)
								{
									spi->set_uniform(idx, shieldState);
								}
							}
							{
								int idx = spi->get_shader_program()->find_uniform_index(NAME(shieldHealthPt));
								if (idx != NONE)
								{
									spi->set_uniform(idx, shieldStatePt);
								}
							}
							{
								int idx = spi->get_shader_program()->find_uniform_index(NAME(shieldExtraOverrideColour));
								if (idx != NONE)
								{
									spi->set_uniform(idx, extraColour.with_alpha(extraActive).to_vector4());
								}
							}
							{
								int idx = spi->get_shader_program()->find_uniform_index(NAME(shieldExtraAlpha));
								if (idx != NONE)
								{
									spi->set_uniform(idx, lerp(extraActive, 1.0f, clamp(extraAlpha, 0.0f, 1.0f)));
								}
							}
							{
								int idx = spi->get_shader_program()->find_uniform_index(NAME(shieldExtraAlphaAdd));
								if (idx != NONE)
								{
									spi->set_uniform(idx, lerp(extraActive, 0.0f, clamp(extraAlpha - 1.0f, 0.0f, 1.0f)));
								}
							}
						}
					}
				}
			}
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
