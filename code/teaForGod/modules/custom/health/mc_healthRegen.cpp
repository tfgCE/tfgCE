#include "mc_healthRegen.h"

#include "..\..\gameplay\moduleEnergyQuantum.h"

#include "..\..\..\game\damage.h"
#include "..\..\..\game\gameDirector.h"
#include "..\..\..\game\gameLog.h"
#include "..\..\..\game\gameSettings.h"
#include "..\..\..\pilgrim\pilgrimBlackboard.h"

#include "..\..\..\..\framework\ai\aiMessage.h"
#include "..\..\..\..\framework\game\gameUtils.h"
#include "..\..\..\..\framework\module\moduleDataImpl.inl"
#include "..\..\..\..\framework\module\modules.h"
#include "..\..\..\..\framework\world\world.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// params
DEFINE_STATIC_NAME(regenRate);
DEFINE_STATIC_NAME(regenCooldown);
DEFINE_STATIC_NAME(healthBackup);
DEFINE_STATIC_NAME(maxHealthBackup);
DEFINE_STATIC_NAME(startingHealthBackup);

// ai message name
DEFINE_STATIC_NAME(rotateOnDamage);

// ai message params
DEFINE_STATIC_NAME(damage);
DEFINE_STATIC_NAME(damageUnadjusted);
DEFINE_STATIC_NAME(damageSource);

// game log
DEFINE_STATIC_NAME(healthRegenOnDeathStatus);

//

REGISTER_FOR_FAST_CAST(HealthRegen);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new HealthRegen(_owner);
}

Framework::RegisteredModule<Framework::ModuleCustom> & HealthRegen::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("healthRegen")), create_module, HealthData::create_module_data);
}

HealthRegen::HealthRegen(Framework::IModulesOwner* _owner)
: base( _owner )
{
}

HealthRegen::~HealthRegen()
{
}

Energy HealthRegen::read_max_health_backup_from(Framework::ModuleData const* _moduleData)
{
	Energy maxHealthBackup = Energy::zero();
	if (!GameSettings::get().difficulty.regenerateEnergy)
	{
		maxHealthBackup = Energy::get_from_module_data(nullptr, _moduleData, NAME(healthBackup), maxHealthBackup);
		maxHealthBackup = Energy::get_from_module_data(nullptr, _moduleData, NAME(maxHealthBackup), maxHealthBackup);
	}
	return maxHealthBackup;
}

Energy HealthRegen::read_regen_rate_from(Framework::ModuleData const* _moduleData)
{
	return Energy::get_from_module_data(nullptr, _moduleData, NAME(regenRate));
}

float HealthRegen::read_regen_cooldown_from(Framework::ModuleData const* _moduleData)
{
	return _moduleData->get_parameter<float>(nullptr, NAME(regenCooldown));
}

void HealthRegen::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		regenRate = Energy::get_from_module_data(this, _moduleData, NAME(regenRate), regenRate);
		regenCooldown = _moduleData->get_parameter<float>(this, NAME(regenCooldown), regenCooldown);
		healthBackup = Energy::get_from_module_data(this, _moduleData, NAME(healthBackup), healthBackup);
		if (GameSettings::get().difficulty.regenerateEnergy)
		{
			maxHealthBackup = Energy::zero();
		}
		else
		{
			maxHealthBackup = Energy::get_from_module_data(this, _moduleData, NAME(maxHealthBackup), maxHealthBackup);
		}
		startingHealthBackup = Energy::get_from_module_data(this, _moduleData, NAME(startingHealthBackup), startingHealthBackup);
		if (startingHealthBackup.is_zero())
		{
			startingHealthBackup = healthBackup;
		}
	}
}

Energy HealthRegen::get_max_health_backup_internal() const
{
	if (GameSettings::get().difficulty.regenerateEnergy)
	{
		return Energy::zero();
	}
	else
	{
		return maxHealthBackup + PilgrimBlackboard::get_health_backup_modifier_for(get_owner());
	}
}

Energy HealthRegen::get_regen_rate() const
{
	return regenRate + PilgrimBlackboard::get_health_regen_rate_modifier_for(get_owner());
}

float HealthRegen::get_regen_cooldown() const
{
	return max(0.0f, PilgrimBlackboard::get_health_regen_cooldown_coef_modifier_for(get_owner()).adjust_plus_one(regenCooldown));
}

void HealthRegen::reset_health(Optional<Energy> const& _to, Optional<bool> const& _clearEffects)
{
	base::reset_health(_to, _clearEffects);
	if (_to.is_set())
	{
		healthBackup = max(Energy::zero(), _to.get() - health);
	}
	else
	{
		healthBackup = startingHealthBackup;
	}
	healthBackup = clamp(healthBackup, Energy::zero(), get_max_health_backup());
}

void HealthRegen::reset()
{
	base::reset();

	mark_requires(all_customs__advance_post, HealthPostMoveReason::Regen);

	timeAfterDamage = 0.0f;
}

void HealthRegen::add_energy(REF_ Energy& _addEnergy)
{
	base::add_energy(_addEnergy);

	Energy currentMaxHealthBackup = get_max_health_backup();
	Energy addNow = min(_addEnergy, currentMaxHealthBackup - healthBackup);
	healthBackup += addNow;
	_addEnergy -= addNow;
}

void HealthRegen::redistribute_energy(REF_ Energy & _excessEnergy)
{
	Energy currentMaxHealthBackup = get_max_health_backup();
	if (healthBackup > currentMaxHealthBackup)
	{
		_excessEnergy += healthBackup - currentMaxHealthBackup;
		healthBackup = currentMaxHealthBackup;
	}
	base::redistribute_energy(REF_ _excessEnergy);
	if (healthBackup < currentMaxHealthBackup &&
		_excessEnergy.is_positive())
	{
		Energy use = min(_excessEnergy, currentMaxHealthBackup - healthBackup);
		healthBackup += use;
		_excessEnergy -= use;
	}
}

void HealthRegen::initialise()
{
	base::initialise();

	mark_requires(all_customs__advance_post, HealthPostMoveReason::Regen);

	timeAfterDamage = 0.0f;
}

void HealthRegen::immortal_health_reached_below_minimum_by(Energy const& _belowMinimum)
{
	healthBackup += _belowMinimum;
	healthBackup = max(Energy::zero(), healthBackup);
}

void HealthRegen::heal(REF_ Energy& _amountLeft)
{
	base::heal(REF_ _amountLeft);

	Energy healUp = min(_amountLeft, get_max_health_backup() - healthBackup);
	healthBackup += healUp;
	_amountLeft -= healUp;
}

void HealthRegen::on_deal_damage(Damage const & _damage, Damage const & _unadjustedDamage, DamageInfo & _info)
{
	base::on_deal_damage(_damage, _unadjustedDamage, _info);

	timeAfterDamage = 0.0f;
}

void HealthRegen::advance_post(float _deltaTime)
{
	base::advance_post(_deltaTime);
	if (!isDying && !isPerformingDeath)
	{
		timeAfterDamage += _deltaTime;
		healthBackup = max(Energy(0), healthBackup);
		Energy currentMaxHealth = get_max_health();
		bool didRegen = false;
		if (health < currentMaxHealth && ! regenBlocked)
		{
			float currentRegenCooldown = get_regen_cooldown();
			if (is_invincible())
			{
				// to enforce
				timeAfterDamage = max(timeAfterDamage, currentRegenCooldown + _deltaTime);
			}
			if (timeAfterDamage >= currentRegenCooldown)
			{
				Energy currentRegenRate = get_regen_rate();
				Energy restoreBy = min(currentRegenRate.timed(min(timeAfterDamage - currentRegenCooldown, _deltaTime), regenMB), currentMaxHealth - health);
				if (!GameSettings::get().difficulty.regenerateEnergy)
				{
					if (auto* imo = get_owner())
					{
						if (GameSettings::get().difficulty.playerImmortal &&
							! GameDirector::get()->is_immortal_health_regen_blocked())
						{
							if (Framework::GameUtils::is_controlled_by_player(imo))
							{
								// always have some health backup to allow restoring to full health when immortal (but no backup then)
								healthBackup = max(restoreBy, healthBackup);
							}
						}
					}
					restoreBy = min(restoreBy, healthBackup);
					healthBackup -= restoreBy;
				}
				health += restoreBy;
				didRegen = true;
			}
		}
		if (!didRegen)
		{
			regenMB = 0.0f;
		}	
		healthBackup = max(Energy(0), healthBackup);
	}
}

void HealthRegen::process_energy_quantum_health(ModuleEnergyQuantum* _eq)
{
	an_assert(_eq->is_being_processed_by_us());

	if (_eq->has_energy())
	{
		MODULE_OWNER_LOCK(TXT("HealthRegen::process_energy_quantum_health"));

		Energy addHealth = min(_eq->get_energy(), get_max_health_backup() - healthBackup);
		healthBackup += addHealth;

		_eq->use_energy(addHealth);
	}

	// don't process directly! has to regen. base::process_energy_quantum_health(_eq);
}

bool HealthRegen::handle_health_energy_transfer_request(EnergyTransferRequest & _request)
{
	if (noEnergyTransfer)
	{
		return false;
	}

	switch (_request.type)
	{
	case EnergyTransferRequest::Query:
	case EnergyTransferRequest::QueryWithdraw:
	case EnergyTransferRequest::QueryDeposit:
		base::handle_health_energy_transfer_request(_request);
		break;
	case EnergyTransferRequest::Deposit:
		{
			{
				MODULE_OWNER_LOCK(TXT("HealthRegen::handle_health_energy_transfer_request  Deposit"));
				if (!_request.fillOrKill || (calculate_total_energy_limit() - calculate_total_energy_available(EnergyType::Health) >= _request.energyRequested))
				{
					Energy addHealth = min(_request.energyRequested, get_max_health_backup() - healthBackup);
					healthBackup += addHealth;
					_request.energyRequested -= addHealth;
					_request.energyResult += addHealth;
				}
			}
			// if we're full, allow filling up the normal health (if it's "fill or kill" this will work too)
			base::handle_health_energy_transfer_request(_request);
		}
		break;
	case EnergyTransferRequest::Withdraw:
		{
			bool callBase = false;
			{
				MODULE_OWNER_LOCK(TXT("HealthRegen::handle_health_energy_transfer_request  Withdraw"));
				if (!_request.fillOrKill || (calculate_total_energy_available(EnergyType::Health) > _request.energyRequested))
				{
					Energy giveHealth = min(_request.energyRequested, healthBackup);
					healthBackup -= giveHealth;
					_request.energyRequested -= giveHealth;
					_request.energyResult += giveHealth;
					if (healthBackup <= Energy(0))
					{
						callBase = true;
					}
				}
			}
			if (callBase)
			{
				// if no at backup, get from our main
				base::handle_health_energy_transfer_request(_request);
			}
		}
		break;
	}
	return true;
}

void HealthRegen::on_death(Damage const& _damage, DamageInfo const& _damageInfo)
{
	base::on_death(_damage, _damageInfo);

	if (Framework::GameUtils::is_local_player(get_owner()))
	{
		GameLog::get().add_entry(
			GameLog::Entry(NAME(healthRegenOnDeathStatus))
			.set_as_int(HEALTH_REGEN_ON_DEATH_STATUS__ENERGY_IN_BACKUP, healthBackup.get_pure()));
	}
}

void HealthRegen::ex__set_total_health(Energy const& _health)
{
	Energy left = _health;
	health = min(left, maxHealth); left -= health;
	healthBackup = min(left, get_max_health_backup()); left -= healthBackup;
}

void HealthRegen::ex__drop_health_by(Energy const& _dropBy, Optional<Energy> const& _min)
{
	Energy left = _dropBy;
	{
		Energy l = min(left, health - _min.get(Energy::zero()));
		health -= l;
		left -= l;
	}
	{
		Energy l = min(left, healthBackup);
		healthBackup -= l;
		left -= l;
	}
}

void HealthRegen::handle_excess_continuous_damage(REF_ Energy& _excessDamage)
{
	Energy sub = min(_excessDamage, healthBackup);
	healthBackup -= sub;
	_excessDamage -= sub;
}

void HealthRegen::store_for_game_state(SimpleVariableStorage& _variables) const
{
	base::store_for_game_state(_variables);
	_variables.access<Energy>(NAME(healthBackup)) = healthBackup;
}

void HealthRegen::restore_from_game_state(SimpleVariableStorage const& _variables)
{
	base::restore_from_game_state(_variables);
	if (auto* e = _variables.get_existing<Energy>(NAME(healthBackup)))
	{
		healthBackup = clamp(*e, Energy::zero(), get_max_health_backup());
	}
}
