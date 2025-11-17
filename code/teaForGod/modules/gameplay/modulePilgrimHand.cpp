#include "modulePilgrimHand.h"

#include "moduleEnergyQuantum.h"
#include "moduleEXM.h"
#include "modulePilgrim.h"

#include "..\..\game\damage.h"
#include "..\..\game\gameDirector.h"
#include "..\..\game\gameSettings.h"

#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\modules.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(ModulePilgrimHand);

static Framework::ModuleGameplay* create_module(Framework::IModulesOwner* _owner)
{
	return new ModulePilgrimHand(_owner);
}

Framework::RegisteredModule<Framework::ModuleGameplay> & ModulePilgrimHand::register_itself()
{
	return Framework::Modules::gameplay.register_module(String(TXT("pilgrim hand")), create_module);
}

ModulePilgrimHand::ModulePilgrimHand(Framework::IModulesOwner* _owner)
: base( _owner )
{
}

ModulePilgrimHand::~ModulePilgrimHand()
{
}

Energy ModulePilgrimHand::calculate_total_energy_available(EnergyType::Type _type) const
{
	if (_type == EnergyType::Ammo)
	{
		if (auto* po = pilgrim.get())
		{
			if (auto* p = po->get_gameplay_as<ModulePilgrim>())
			{
				return p->get_hand_energy_storage((hand));
			}
		}
	}
	else if (_type == EnergyType::Health)
	{
		if (auto* it = IEnergyTransfer::get_energy_transfer_module_from(health.get(), EnergyType::Health))
		{
			return it->calculate_total_energy_available(_type);
		}
	}
	return Energy::zero();
}

Energy ModulePilgrimHand::calculate_total_max_energy_available(EnergyType::Type _type) const
{
	if (_type == EnergyType::Ammo)
	{
		if (auto* po = pilgrim.get())
		{
			if (auto* p = po->get_gameplay_as<ModulePilgrim>())
			{
				return p->get_hand_energy_max_storage((hand));
			}
		}
	}
	else if (_type == EnergyType::Health)
	{
		if (auto* it = IEnergyTransfer::get_energy_transfer_module_from(health.get(), EnergyType::Health))
		{
			return it->calculate_total_max_energy_available(_type);
		}
	}
	return Energy::zero();
}

bool ModulePilgrimHand::handle_ammo_energy_transfer_request(EnergyTransferRequest & _request)
{
	if (auto* po = pilgrim.get())
	{
		if (auto* p = po->get_gameplay_as<ModulePilgrim>())
		{
			switch (_request.type)
			{
			case EnergyTransferRequest::Query:
			case EnergyTransferRequest::QueryWithdraw:
				_request.priority = -100; // check below
				break;
			case EnergyTransferRequest::QueryDeposit:
				_request.priority = -100; // check below
				break;
			case EnergyTransferRequest::Deposit:
			{
				if (auto* gd = GameDirector::get())
				{
					if (gd->are_ammo_storages_unavailable())
					{
						break;
					}
				}
				MODULE_OWNER_LOCK_FOR(p, TXT("ModulePilgrimHand::handle_ammo_energy_transfer_request  Deposit"));
				Energy currentMaxStorage = p->get_hand_energy_max_storage((hand));
				Energy storage = p->get_hand_energy_storage((hand));
				if (!_request.fillOrKill || (currentMaxStorage - storage >= _request.energyRequested))
				{
					Energy addStorage = min(_request.energyRequested, currentMaxStorage - storage);
					storage += addStorage;
					_request.energyRequested -= addStorage;
					_request.energyResult += addStorage;
					p->set_hand_energy_storage((hand), storage);
				}
			}
			break;
			case EnergyTransferRequest::Withdraw:
			{
				if (auto* gd = GameDirector::get())
				{
					if (gd->are_ammo_storages_unavailable())
					{
						break;
					}
				}
				MODULE_OWNER_LOCK_FOR(p, TXT("ModulePilgrimHand::handle_ammo_energy_transfer_request  Withdraw"));
				Energy storage = p->get_hand_energy_storage((hand));
				if (!_request.fillOrKill || (storage >= _request.energyRequested))
				{
					if (!_request.energyRequested.is_zero())
					{
						Energy giveStorage = min(_request.energyRequested, storage - _request.minLeft);
						storage -= giveStorage;
						_request.energyRequested -= giveStorage;
						_request.energyResult += giveStorage;
						p->set_hand_energy_storage((hand), storage);
					}
				}
			}
			break;
			}
			return true;
		}
	}
	return false;
}

bool ModulePilgrimHand::handle_health_energy_transfer_request(EnergyTransferRequest & _request)
{
	if (auto * it = IEnergyTransfer::get_energy_transfer_module_from(health.get(), EnergyType::Health))
	{
		bool result = it->handle_health_energy_transfer_request(_request);
		_request.priority = -100; // give priority to whatever is being held in hand
		return result;
	}
	return false;
}

void ModulePilgrimHand::process_energy_quantum_health(ModuleEnergyQuantum* _eq)
{
	an_assert(_eq->is_being_processed_by_us());

	if (_eq->has_energy())
	{
		// equipment first
		if (auto* pimo = pilgrim.get())
		{
			if (auto* mp = pimo->get_gameplay_as<ModulePilgrim>())
			{
				if (auto* eqInHand = mp->get_hand_equipment(hand))
				{
					if (auto* it = IEnergyTransfer::get_energy_transfer_module_from(eqInHand, EnergyType::Health))
					{
						EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
						etr.energyRequested = _eq->get_energy();
						handle_health_energy_transfer_request(etr);
						_eq->use_energy(etr.energyResult);
					}
				}
			}
		}

		// hand second
		{
			EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
			etr.energyRequested = _eq->get_energy();
			handle_health_energy_transfer_request(etr);
			_eq->use_energy(etr.energyResult);
		}
	}
}

void ModulePilgrimHand::process_energy_quantum_ammo(ModuleEnergyQuantum* _eq)
{
	an_assert(_eq->is_being_processed_by_us());

	if (_eq->has_energy())
	{
		// equipment first
		if (auto* pimo = pilgrim.get())
		{
			if (auto* mp = pimo->get_gameplay_as<ModulePilgrim>())
			{
				if (auto* eqInHand = mp->get_hand_equipment(hand))
				{
					if (auto* it = IEnergyTransfer::get_energy_transfer_module_from(eqInHand, EnergyType::Ammo))
					{
						EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
						etr.energyRequested = _eq->get_energy();
						handle_ammo_energy_transfer_request(etr);
						_eq->use_energy(etr.energyResult);
					}
				}
			}
		}

		// hand second
		{
			EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
			etr.energyRequested = _eq->get_energy();
			handle_ammo_energy_transfer_request(etr);
			_eq->use_energy(etr.energyResult);
		}
	}
}