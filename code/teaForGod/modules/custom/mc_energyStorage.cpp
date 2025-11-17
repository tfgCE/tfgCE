#include "mc_energyStorage.h"

#include "mc_emissiveControl.h"

#include "..\..\ai\logics\exms\aiLogic_exmEnergyDispatcher.h"

#include "..\..\..\framework\appearance\controllers\ac_particlesUtils.h"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\temporaryObject.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// params
DEFINE_STATIC_NAME(amount);
DEFINE_STATIC_NAME(startingAmount);
DEFINE_STATIC_NAME(capacity);
DEFINE_STATIC_NAME(emissivePowerLevel);
DEFINE_STATIC_NAME(transferSpeed);
DEFINE_STATIC_NAME(energyType);
DEFINE_STATIC_NAME(transferTemporaryObject);

// timer
DEFINE_STATIC_NAME(clearUpdateEmissive);

//

REGISTER_FOR_FAST_CAST(EnergyStorage);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new EnergyStorage(_owner);
}

Framework::ModuleData* EnergyStorageData::create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new EnergyStorageData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleCustom> & EnergyStorage::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("energyStorage")), create_module, EnergyStorageData::create_module_data);
}

EnergyStorage::EnergyStorage(Framework::IModulesOwner* _owner)
: base( _owner )
{
}

EnergyStorage::~EnergyStorage()
{
	Framework::ParticlesUtils::desire_to_deactivate(transfertemporaryObject.get());
	transfertemporaryObject.clear();
}

void EnergyStorage::reset()
{
	base::reset();

	reset_energy();

	Framework::ParticlesUtils::desire_to_deactivate(transfertemporaryObject.get());
	transfertemporaryObject.clear();
}

void EnergyStorage::setup_with(Framework::ModuleData const* _moduleData)
{
	base::setup_with(_moduleData);

	energyStorageData = fast_cast<EnergyStorageData>(_moduleData);
	if (_moduleData)
	{
		startingAmount = EnergyRange::get_from_module_data(this, _moduleData, NAME(amount), startingAmount);
		startingAmount = EnergyRange::get_from_module_data(this, _moduleData, NAME(startingAmount), startingAmount);
		capacity = Energy::get_from_module_data(this, _moduleData, NAME(capacity), capacity);
		transferSpeed = Energy::get_from_module_data(this, _moduleData, NAME(transferSpeed), transferSpeed);

		transferTemporaryObjectId = _moduleData->get_parameter<Name>(this, NAME(transferTemporaryObject), transferTemporaryObjectId);
		
		emissivePowerLevel = _moduleData->get_parameter<Name>(this, NAME(emissivePowerLevel), emissivePowerLevel);

		energyType = EnergyType::parse(_moduleData->get_parameter<String>(this, NAME(energyType), String(EnergyType::to_char(energyType))), energyType);
	}
}

void EnergyStorage::initialise()
{
	base::initialise();

	reset_energy();
}

void EnergyStorage::reset_energy()
{
	auto rg = get_owner()->get_individual_random_generator();
	rg.advance_seed(9756, 972395);
		
	if (startingAmount.is_empty())
	{
		if (energyStorageData && !energyStorageData->startingAmount.is_empty())
		{
			amount = energyStorageData->startingAmount.get(rg);
		}
		else
		{
			amount = Energy::zero();
		}
	}
	else
	{
		amount = startingAmount.get_random(rg);
	}
	amount = clamp(amount, Energy::zero(), capacity);
}

Energy EnergyStorage::calculate_total_energy_available(EnergyType::Type _type) const
{
	if (_type == energyType)
	{
		return amount;
	}
	return Energy::zero();
}

Energy EnergyStorage::calculate_total_max_energy_available(EnergyType::Type _type) const
{
	if (_type == energyType)
	{
		return capacity;
	}
	return Energy::zero();
}

void EnergyStorage::transferring_energy()
{
	if (!transfertemporaryObject.get() &&
		transferTemporaryObjectId.is_valid())
	{
		if (auto* to = get_owner()->get_temporary_objects())
		{
			transfertemporaryObject = to->spawn(transferTemporaryObjectId);
		}
	}
}

void EnergyStorage::not_transferring_energy()
{
	if (auto* tto = transfertemporaryObject.get())
	{
		Framework::ParticlesUtils::desire_to_deactivate(transfertemporaryObject.get());
		transfertemporaryObject.clear();
	}
}

bool EnergyStorage::begin_transfer(Framework::IModulesOwner* _imo)
{
	if (!transferRecipent.get() ||
		transferRecipent == _imo)
	{
		timeToNextTransfer = 0.0f;
		transferRecipent = _imo;
		update_emissive();
		not_transferring_energy(); // will switch on on actual transfer
		return true;
	}
	else
	{
		return false;
	}
}

void EnergyStorage::end_transfer(Framework::IModulesOwner* _imo)
{
	if (transferRecipent == _imo)
	{
		update_emissive(false);
		transferRecipent.clear();
		not_transferring_energy();
	}
}

Energy EnergyStorage::transfer_energy_to(Framework::IModulesOwner* _imo, float _deltaTime)
{
	if (transferRecipent != _imo)
	{
		return Energy::zero();
	}
	timeToNextTransfer -= _deltaTime;
	
	Energy energyToTransfer = Energy(0);
	while (timeToNextTransfer <= 0.0f)
	{
		timeToNextTransfer += 1.0f / (transferSpeed.as_float());
		energyToTransfer += Energy(1);
	}
	// update (transferring or not) only if something to transfer
	Energy transferred = Energy::zero();
	if (energyToTransfer.is_positive())
	{
		if (auto* it = IEnergyTransfer::get_energy_transfer_module_from(_imo, energyType))
		{
			EnergyTransferRequest etr(EnergyTransferRequest::Deposit /*withdraw here, deposit in object*/);
			etr.energyRequested = min(energyToTransfer, amount);
			if (!etr.energyRequested.is_zero())
			{
				bool success = false;
				for_count(int, transferIdx, 2)
				{
					success = false;
					if (energyType == EnergyType::Ammo)
					{
						success = it->handle_ammo_energy_transfer_request(etr);
					}
					else if (energyType == EnergyType::Health)
					{
						success = it->handle_health_energy_transfer_request(etr);
					}
					if (transferIdx == 0 && etr.energyResult.is_zero())
					{
						// transferred nothing, check if we can redistribute energy we could transfer
						Energy toRedistribute = etr.energyRequested;
						Energy redistributed = AI::Logics::EXMEnergyDispatcher::redistribute_energy_from(it, energyType, toRedistribute);
						if (redistributed.is_positive())
						{
							// redisitributed, try again
							continue;
						}
					}
					break;
				}
				if (!etr.energyResult.is_zero() && success)
				{
					transferred = etr.energyResult;
					amount -= etr.energyResult;
				}
			}
		}
		if (transferred.is_positive())
		{
			transferring_energy();
		}
		else
		{
			not_transferring_energy();
		}
	}
	update_emissive();
	return transferred;
}

void EnergyStorage::update_emissive(Optional<bool> const& _active)
{
	if (!emissivePowerLevel.is_valid())
	{
		return;
	}

	if (auto* imo = get_owner())//transferRecipent.get())
	{
		ArrayStatic<Framework::IModulesOwner*, 6> imos;
		imos.push_back(imo);
		for (int i = 0; i < imos.get_size(); ++i)
		{
			auto* imo = imos[i];
			Concurrency::ScopedSpinLock lock(imo->get_presence()->access_attached_lock());
			for_every_ptr(attached, imo->get_presence()->get_attached())
			{
				if (imos.has_place_left())
				{
					imos.push_back(attached);
				}
				else
				{
					break;
				}
			}
		}

		for_every_ptr(imo, imos)
		{
			if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
			{
				if (_active.get(true))
				{
					em->emissive_activate(emissivePowerLevel);
					auto& emissiveLayer = em->emissive_access(emissivePowerLevel);
					emissiveLayer.set_power(0.2f + 1.3f * amount.div_to_float(capacity));
				}
				else
				{
					em->emissive_deactivate(emissivePowerLevel);
				}
			}
		}
	}

}

bool EnergyStorage::handle_energy_transfer_request(EnergyType::Type _transferEnergy, EnergyTransferRequest& _request)
{
	if (_transferEnergy == energyType)
	{
		switch (_request.type)
		{
			case EnergyTransferRequest::Query:
			case EnergyTransferRequest::QueryWithdraw:
				break;
			case EnergyTransferRequest::QueryDeposit:
				break;
			case EnergyTransferRequest::Deposit:
			{
				MODULE_OWNER_LOCK_FOR_IMO(get_owner(), TXT("EnergyStorage::handle_ammo_energy_transfer_request  Deposit"));
				if (!_request.fillOrKill || (capacity - amount >= _request.energyRequested))
				{
					Energy addStorage = min(_request.energyRequested, capacity - amount);
					amount += addStorage;
					_request.energyRequested -= addStorage;
					_request.energyResult += addStorage;
					update_emissive(true);
					manage_auto_clear_update_emissive();
				}
			}
			break;
			case EnergyTransferRequest::Withdraw:
			{
				MODULE_OWNER_LOCK_FOR_IMO(get_owner(), TXT("EnergyStorage::handle_ammo_energy_transfer_request  Withdraw"));
				if (!_request.fillOrKill || (amount >= _request.energyRequested))
				{
					if (!_request.energyRequested.is_zero())
					{
						Energy giveStorage = min(_request.energyRequested, amount - _request.minLeft);
						amount -= giveStorage;
						_request.energyRequested -= giveStorage;
						_request.energyResult += giveStorage;
						update_emissive(true);
						manage_auto_clear_update_emissive();
					}
				}
			}
			break;
		}
		return true;
	}
	return false;
}

void EnergyStorage::manage_auto_clear_update_emissive()
{
	if (!emissivePowerLevel.is_valid())
	{
		return;
	}

	// this is used to update switching off emissive layer after no action is done within a small amount of time
	get_owner()->set_timer(0.1f, NAME(clearUpdateEmissive), [](Framework::IModulesOwner* _imo)
		{
			if (auto* es = _imo->get_custom<EnergyStorage>())
			{
				es->update_emissive(false);
			}
		});
}

bool EnergyStorage::handle_ammo_energy_transfer_request(EnergyTransferRequest & _request)
{
	return handle_energy_transfer_request(EnergyType::Ammo, _request);
}

bool EnergyStorage::handle_health_energy_transfer_request(EnergyTransferRequest & _request)
{
	return handle_energy_transfer_request(EnergyType::Health, _request);
}

void EnergyStorage::store_for_game_state(SimpleVariableStorage& _variables) const
{
	_variables.access<Energy>(NAME(amount)) = amount;
}

void EnergyStorage::restore_from_game_state(SimpleVariableStorage const& _variables)
{
	if (auto* e = _variables.get_existing<Energy>(NAME(amount)))
	{
		amount = clamp(*e, Energy::zero(), capacity);
	}
}

//

REGISTER_FOR_FAST_CAST(EnergyStorageData);

EnergyStorageData::EnergyStorageData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

EnergyStorageData::~EnergyStorageData()
{
}

bool EnergyStorageData::read_parameter_from(IO::XML::Attribute const* _attr, Framework::LibraryLoadingContext& _lc)
{
	if (_attr->get_name() == TXT("amount") ||
		_attr->get_name() == TXT("startingAmount"))
	{
		return startingAmount.load_from_xml(_attr);
	}
	return base::read_parameter_from(_attr, _lc);
}

bool EnergyStorageData::read_parameter_from(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	if (_node->get_name() == TXT("amount") ||
		_node->get_name() == TXT("startingAmount"))
	{
		return startingAmount.load_from_xml(_node);
	}
	return base::read_parameter_from(_node, _lc);
}

bool EnergyStorageData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
