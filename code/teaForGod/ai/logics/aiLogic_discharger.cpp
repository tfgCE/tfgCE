#include "aiLogic_discharger.h"

#include "..\..\game\gameDirector.h"
#include "..\..\library\library.h"
#include "..\..\modules\custom\health\mc_health.h"
#include "..\..\utils\lightningDischarge.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\module\moduleAppearance.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// socket names
DEFINE_STATIC_NAME(discharge);

// temporary objects
DEFINE_STATIC_NAME_STR(dischargeMiss, TXT("discharge miss"));
DEFINE_STATIC_NAME_STR(lightningHit, TXT("lightning hit"));

// params
DEFINE_STATIC_NAME(dischargeInterval);
DEFINE_STATIC_NAME(dischargeIntervalOnDischarge);
DEFINE_STATIC_NAME(maxDischargeDist);
DEFINE_STATIC_NAME(damage);

//

REGISTER_FOR_FAST_CAST(Discharger);

Discharger::Discharger(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

Discharger::~Discharger()
{
}

void Discharger::advance(float _deltaTime)
{
	set_auto_rare_advance_if_not_visible(Range(0.1f, 0.5f));

	base::advance(_deltaTime);
}

void Discharger::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);
	if (auto* di = _parameters.get_existing<float>(NAME(dischargeInterval)))
	{
		useInterval = Range(*di);
	}
	useInterval = _parameters.get_value<Range>(NAME(dischargeInterval), useInterval);
	if (auto* di = _parameters.get_existing<float>(NAME(dischargeIntervalOnDischarge)))
	{
		intervalOnDischarge = Range(*di);
	}
	intervalOnDischarge = _parameters.get_value<Range>(NAME(dischargeIntervalOnDischarge), intervalOnDischarge);
	maxDist = _parameters.get_value<float>(NAME(maxDischargeDist), maxDist);

	damage = Energy::get_from_storage(_parameters, NAME(damage), damage);
}

LATENT_FUNCTION(Discharger::execute_logic)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic discharger"));

	LATENT_PARAM_NOT_USED(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	auto* self = fast_cast<Discharger>(logic);

	LATENT_BEGIN_CODE();

	while (true)
	{
		if (self->discharge())
		{
			LATENT_WAIT(Random::get(self->intervalOnDischarge)); // hit multiple times
		}
		else
		{
			LATENT_WAIT(Random::get(self->useInterval));
		}
	}

	LATENT_ON_BREAK();
	//
	
	LATENT_ON_END();

	LATENT_END_CODE();
	LATENT_RETURN();
}

bool Discharger::discharge()
{
	if (GameDirector::is_violence_disallowed())
	{
		return false;
	}
	if (auto* mind = get_mind())
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
			LightningDischarge::Params params;

			params.for_imo(imo);
			params.with_start_placement_OS(imo->get_appearance()->calculate_socket_os(NAME(discharge)));
			params.with_max_dist(maxDist);
			params.with_setup_damage([this](Damage& dealDamage, DamageInfo& damageInfo)
				{
					dealDamage.damage = damage;
					dealDamage.cost = damage;
					damageInfo.requestCombatAuto = false;
				});
			params.with_ray_cast_search_for_objects(false); // prefer wide search first (fallback is still there)
			params.ignore_narrative();

			return LightningDischarge::perform(params);
		}
	}
	return false;
}
