#include "energyTransfer.h"

#include "..\modules\custom\health\mc_health.h"

#include "..\..\framework\module\moduleCustom.h"
#include "..\..\framework\module\moduleGameplay.h"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(IEnergyTransfer);

bool IEnergyTransfer::handle_energy_transfer_request(EnergyType::Type _energy, EnergyTransferRequest& _request)
{
	bool result = false;
	if (_energy == EnergyType::Ammo)
	{
		result |= handle_ammo_energy_transfer_request(_request);
	}
	if (_energy == EnergyType::Health)
	{
		result |= handle_health_energy_transfer_request(_request);
	}
	return result;
}

bool IEnergyTransfer::handle_any_energy_transfer_request(EnergyTransferRequest& _request)
{
	bool result = false;
	an_assert(_request.type == EnergyTransferRequest::Withdraw || _request.type == EnergyTransferRequest::Deposit, TXT("only deposit or withdraw can be handled with this call"));
	result |= handle_ammo_energy_transfer_request(_request);
	result |= handle_health_energy_transfer_request(_request);
	return result;
}

IEnergyTransfer* IEnergyTransfer::get_any_energy_transfer_module_from(Framework::IModulesOwner* _owner)
{
	IEnergyTransfer* et = nullptr;
	if (!et) et = get_energy_transfer_module_from(_owner, EnergyType::Ammo);
	if (!et) et = get_energy_transfer_module_from(_owner, EnergyType::Health);
	return et;
}

IEnergyTransfer* IEnergyTransfer::get_energy_transfer_module_from(Framework::IModulesOwner* _owner, EnergyType::Type _energyType)
{
	if (_owner)
	{
		if (auto* it = _owner->get_gameplay_as<IEnergyTransfer>())
		{
			switch (_energyType)
			{
			case EnergyType::Health: if (does_accept_energy_transfer_health(it)) return it; break;
			case EnergyType::Ammo: if (does_accept_energy_transfer_ammo(it)) return it; break;
			case EnergyType::None: break;
			default: break;
			}
		}
		for_every_ptr(c, _owner->get_customs())
		{
			if (auto* it = fast_cast<IEnergyTransfer>(c))
			{
				switch (_energyType)
				{
				case EnergyType::Health: if (does_accept_energy_transfer_health(it)) return it; break;
				case EnergyType::Ammo: if (does_accept_energy_transfer_ammo(it)) return it; break;
				case EnergyType::None: break;
				default: break;
				}
			}
		}
	}
	return nullptr;
}

Energy IEnergyTransfer::calculate_total_energy_available_for(Framework::IModulesOwner* _owner)
{
	Energy result = Energy::zero();
	for (int energyType = EnergyType::First; energyType <= EnergyType::Last; ++energyType)
	{
		if (auto * et = get_energy_transfer_module_from(_owner, (EnergyType::Type)energyType))
		{
			result += et->calculate_total_energy_available((EnergyType::Type)energyType);
		}
	}
	return result;
}

bool IEnergyTransfer::does_accept_energy_transfer(EnergyType::Type _energyType, IEnergyTransfer* _iet, Optional<bool> const & _deposit)
{
	if (_energyType == EnergyType::Ammo)
	{
		return does_accept_energy_transfer_ammo(_iet, _deposit);
	}
	if (_energyType == EnergyType::Health)
	{
		return does_accept_energy_transfer_health(_iet, _deposit);
	}
	return false;
}

bool IEnergyTransfer::does_accept_energy_transfer_ammo(IEnergyTransfer* _iet, Optional<bool> const & _deposit)
{
	EnergyTransferRequest etr(_deposit.is_set()? (_deposit.get()? EnergyTransferRequest::QueryDeposit : EnergyTransferRequest::QueryWithdraw) : EnergyTransferRequest::Query);
	return _iet && _iet->handle_ammo_energy_transfer_request(etr);
}

bool IEnergyTransfer::does_accept_energy_transfer_health(IEnergyTransfer* _iet, Optional<bool> const & _deposit)
{
	EnergyTransferRequest etr(_deposit.is_set() ? (_deposit.get() ? EnergyTransferRequest::QueryDeposit : EnergyTransferRequest::QueryWithdraw) : EnergyTransferRequest::Query);
	return _iet && _iet->handle_health_energy_transfer_request(etr);
}

bool IEnergyTransfer::does_accept_energy_transfer(EnergyType::Type _energyType, Framework::IModulesOwner* _owner, Optional<bool> const & _deposit)
{
	if (_energyType == EnergyType::Ammo)
	{
		return does_accept_energy_transfer_ammo(get_energy_transfer_module_from(_owner, _energyType), _deposit);
	}
	if (_energyType == EnergyType::Health)
	{
		return does_accept_energy_transfer_health(get_energy_transfer_module_from(_owner, _energyType), _deposit);
	}
	return false;
}

bool IEnergyTransfer::does_accept_energy_transfer_ammo(Framework::IModulesOwner* _owner, Optional<bool> const & _deposit)
{
	return does_accept_energy_transfer_ammo(get_energy_transfer_module_from(_owner, EnergyType::Ammo), _deposit);
}

bool IEnergyTransfer::does_accept_energy_transfer_health(Framework::IModulesOwner* _owner, Optional<bool> const & _deposit)
{
	return does_accept_energy_transfer_health(get_energy_transfer_module_from(_owner, EnergyType::Health), _deposit);
}

bool IEnergyTransfer::transfer_energy_between_two(Framework::IModulesOwner* _instigator, Energy const& _amount, IEnergyTransfer* _from, EnergyType::Type _fromType, IEnergyTransfer* _to, EnergyType::Type _toType)
{
	Energy amount = _amount;
	return transfer_energy_between_two_monitor_energy(_instigator, amount, _from, _fromType, _to, _toType);
}

bool IEnergyTransfer::transfer_energy_between_two_monitor_energy(Framework::IModulesOwner* _instigator, Energy & _amount, IEnergyTransfer* _from, EnergyType::Type _fromType, IEnergyTransfer* _to, EnergyType::Type _toType)
{
	Energy transferAmount = _amount;
	bool depositFillOrKill = false;
	Energy energyToTransfer;
	{
		EnergyTransferRequest etr(EnergyTransferRequest::Withdraw);
		etr.energyRequested = transferAmount;
		etr.instigator = _instigator;
		_from->handle_energy_transfer_request(_fromType, etr);
		energyToTransfer = etr.energyResult;
	}
	Energy energyLeftOver;
	Energy transferredEnergy;
	{
		EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
		etr.energyRequested = energyToTransfer;
		etr.instigator = _instigator;
		etr.fillOrKill = depositFillOrKill;
		_to->handle_energy_transfer_request(_toType, etr);
		energyLeftOver = etr.energyRequested;
		transferredEnergy = etr.energyResult;
	}
	if (!energyLeftOver.is_zero())
	{
		EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
		etr.energyRequested = energyLeftOver;
		etr.instigator = _instigator;
		_from->handle_energy_transfer_request(_fromType, etr);
		an_assert(etr.energyRequested.is_zero());
	}
	_amount -= transferredEnergy;
	return !transferredEnergy.is_zero();
}
