#include "gse_health.h"

#include "..\..\game\game.h"
#include "..\..\modules\custom\health\mc_healthRegen.h"

#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

// variables
DEFINE_STATIC_NAME(missingBit);

//

bool Health::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	var = _node->get_name_attribute(TXT("var"), var);
	var = _node->get_name_attribute(TXT("objectVar"), var);
	if (auto* attr = _node->get_attribute(TXT("regen")))
	{
		blockRegen = Option::parse(attr->get_as_string().to_char(), Option::Allow, Option::Allow | Option::Block, &result);
	}
	beImmortal.load_from_xml(_node, TXT("beImmortal"));
	clearImmortal.load_from_xml(_node, TXT("clearImmortal"));
	speed.load_from_attribute_or_from_child(_node, TXT("speed"));
	drainTo.load_from_xml(_node, TXT("drainTo"));
	drainToTotal.load_from_xml(_node, TXT("drainToTotal"));
	fillToTotal.load_from_xml(_node, TXT("fillToTotal"));
	setTotal.load_from_xml(_node, TXT("setTotal"));
	setBackup.load_from_xml(_node, TXT("setBackup"));
	performDeath.load_from_xml(_node, TXT("performDeath"));
	quickDeath.load_from_xml(_node, TXT("quickDeath"));
	peacefulDeath.load_from_xml(_node, TXT("peacefulDeath"));
	dropBy.load_from_xml(_node, TXT("dropBy"));
	dropMin.load_from_xml(_node, TXT("dropMin"));

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Health::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	bool okToContinue = true;

	if (auto* game = Game::get_as<Game>())
	{
		float deltaTime = game->get_delta_time();
		Framework::IModulesOwner* imo = nullptr;
		if (var.is_valid())
		{
			if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(var))
			{
				imo = exPtr->get();
			}
		}
		else
		{
			imo = game->access_player().get_actor();
		}
		if (imo)
		{
			auto& missingBit = _execution.access_variables().access<float>(NAME(missingBit));
			if (auto* health = imo->get_custom<CustomModules::HealthRegen>())
			{
				if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Entered))
				{
					missingBit = 0.0f;
				}
				if (blockRegen.is_set())
				{
					health->block_regen(blockRegen.get() == Option::Block);
				}
				if (setBackup.is_set())
				{
					health->ex__set_backup_health(setBackup.get());
				}
			}
			if (auto* health = imo->get_custom<CustomModules::Health>())
			{
				if (beImmortal.is_set())
				{
					health->ex__set_immortal(beImmortal);
				}
				if (clearImmortal.get(false))
				{
					health->ex__set_immortal(NP);
				}
				if (drainTo.is_set())
				{
					if (health->get_health() > drainTo.get())
					{
						health->set_health(max(drainTo.get(), health->get_health() - speed.timed(deltaTime, missingBit)));
						okToContinue = false;
					}
				}
				if (drainToTotal.is_set())
				{
					Energy energyLeft = health->calculate_total_energy_available(EnergyType::Health);
					if (energyLeft > drainToTotal.get())
					{
						EnergyTransferRequest etr(EnergyTransferRequest::Withdraw);
						etr.energyRequested = min(energyLeft - drainToTotal.get(), speed.timed(deltaTime, missingBit));
						health->handle_health_energy_transfer_request(etr);
						okToContinue = false;
					}
				}
				if (fillToTotal.is_set())
				{
					Energy energyLeft = health->calculate_total_energy_available(EnergyType::Health);
					if (energyLeft < fillToTotal.get())
					{
						EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
						etr.energyRequested = min(fillToTotal.get() - energyLeft, speed.timed(deltaTime, missingBit));
						health->handle_health_energy_transfer_request(etr);
						okToContinue = false;
					}
				}
				if (setTotal.is_set())
				{
					health->ex__set_total_health(setTotal.get());
				}
				if (peacefulDeath.is_set() &&
					peacefulDeath.get())
				{
					health->ex__set_total_health(Energy::zero());
					health->ex__set_peaceful_death(true);
					health->ex__set_lootable_death(false);
					health->perform_quick_death(nullptr);
				}
				else if (quickDeath.is_set() &&
						 quickDeath.get())
				{
					health->ex__set_total_health(Energy::zero());
					health->perform_quick_death(nullptr);
				}
				else if (performDeath.is_set() &&
						 performDeath.get())
				{
					health->ex__set_total_health(Energy::zero());
					health->perform_death(nullptr);
				}
				if (dropBy.is_set())
				{
					health->ex__drop_health_by(dropBy.get().get_random(), dropMin);
				}
			}
		}
		else
		{
			error(TXT("no object found for health script"));
		}
	}

	if (okToContinue)
	{
		return Framework::GameScript::ScriptExecutionResult::Continue;
	}
	else
	{
		return Framework::GameScript::ScriptExecutionResult::Repeat;
	}
}
