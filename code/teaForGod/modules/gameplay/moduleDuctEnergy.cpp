#include "moduleDuctEnergy.h"

#include "..\energyTransfer.h"

#include "..\..\ai\logics\exms\aiLogic_exmEnergyDispatcher.h"

#include "..\..\..\framework\appearance\controllers\ac_particlesUtils.h"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\custom\mc_lightSources.h"
#include "..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\framework\world\presenceLink.h"
#include "..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// variables
DEFINE_STATIC_NAME(ductBayRadius);
DEFINE_STATIC_NAME(ductBayRadiusDetect);
DEFINE_STATIC_NAME(ductEnergyTransferSpeed);

// light sources
DEFINE_STATIC_NAME(energy);

// temporary objects
DEFINE_STATIC_NAME(withdraw);

//

REGISTER_FOR_FAST_CAST(ModuleDuctEnergy);

static Framework::ModuleGameplay* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleDuctEnergy(_owner);
}

Framework::RegisteredModule<Framework::ModuleGameplay>& ModuleDuctEnergy::register_itself()
{
	return Framework::Modules::gameplay.register_module(String(TXT("ductEnergy")), create_module);
}

ModuleDuctEnergy::ModuleDuctEnergy(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

ModuleDuctEnergy::~ModuleDuctEnergy()
{
}

void ModuleDuctEnergy::be_active(Framework::IModulesOwner* imo, bool _active)
{
	if (imo)
	{
		if (auto* mde = imo->get_gameplay_as<ModuleDuctEnergy>())
		{
			mde->be_active(_active);
		}
		if (auto* ls = imo->get_custom<Framework::CustomModules::LightSources>())
		{
			if (! _active)
			{
				ls->fade_out(NAME(energy));
			}
			else if (!ls->has_active(NAME(energy)))
			{
				ls->add(NAME(energy), true);
			}
		}
	}
}

void ModuleDuctEnergy::reset()
{
	base::reset();

	temporaryObjectsRequireReset = true;
}

void ModuleDuctEnergy::setup_with(Framework::ModuleData const* _moduleData)
{
	base::setup_with(_moduleData);

	bay.transferSpeed = Energy::get_from_module_data(this, _moduleData, NAME(ductEnergyTransferSpeed), bay.transferSpeed);
	bayRadius = _moduleData->get_parameter<float>(this, NAME(ductBayRadius), bayRadius);
	bayRadius = _moduleData->get_parameter<float>(this, NAME(ductBayRadiusDetect), bayRadius);
}

void ModuleDuctEnergy::update_object_in_bay(::Framework::IModulesOwner* imo)
{
	float const bayRadiusSq = sqr(bayRadius);
	Vector3 bayLoc = imo->get_presence()->get_centre_of_presence_WS();
	float inBayDist = 0.0f;

	::Framework::RelativeToPresencePlacement& objectInBay = bay.objectInBay;

	if (auto* ib = objectInBay.get_target())
	{
		if (isActive)
		{
			if (ib->get_presence()->get_in_room() != imo->get_presence()->get_in_room())
			{
				objectInBay.clear_target();
			}
			else
			{
				float inBayDist = (bayLoc - ib->get_presence()->get_centre_of_presence_WS()).length();
				if (inBayDist > bayRadius * hardcoded magic_number 1.2f)
				{
					objectInBay.clear_target();
				}
			}
		}
		else
		{
			objectInBay.clear_target();
		}
	}

	float bestDistSq = 1000.0f;
	Framework::IModulesOwner* bestImo = nullptr;
	Optional<int> bestKindPriority;

	// check presence links
	if (isActive)
	{
		Framework::IModulesOwner* inBay = objectInBay.get_target();
		if (auto* o = get_owner())
		{
			if (auto* r = get_owner()->get_presence()->get_in_room())
			{
				auto* pl = get_owner()->get_presence()->get_in_room()->get_presence_links();
				while (pl)
				{
					if (auto* o = pl->get_modules_owner())
					{
						if (o != inBay)
						{
							Vector3 oCentre = pl->get_placement_in_room().location_to_world(o->get_presence()->get_centre_of_presence_os().get_translation());
							float distSq = (bayLoc - oCentre).length_squared();
							if (distSq < bayRadiusSq &&
								(!bestImo || distSq < bestDistSq))
							{
								Optional<int> newKindPriority = determine_priority(o);
								if (newKindPriority.is_set())
								{
									bestImo = o;
									bestDistSq = distSq;
									bestKindPriority = newKindPriority;
								}
							}
						}
					}
					pl = pl->get_next_in_room();
				}
			}
		}
	}

	if (bestKindPriority.is_set())
	{
		Optional<int> inBayKindPriority = determine_priority(objectInBay.get_target());
		if (bestKindPriority.get() > inBayKindPriority.get(bestKindPriority.get() - 1))
		{
			objectInBay.find_path(imo, bestImo);
		}
		else if (inBayKindPriority.is_set() &&
			bestKindPriority.get() == inBayKindPriority.get())
		{
			float bestDist = sqrt(bestDistSq);
			if (bestDist < inBayDist * hardcoded magic_number 0.8f)
			{
				objectInBay.find_path(imo, bestImo);
			}
		}
	}
}

Optional<int> ModuleDuctEnergy::determine_priority(::Framework::IModulesOwner* _object)
{
	if (!_object)
	{
		return NP;
	}
	if (auto* it = IEnergyTransfer::get_energy_transfer_module_from(_object, EnergyType::Ammo))
	{
		EnergyTransferRequest etr(EnergyTransferRequest::Query);
		if (it->handle_ammo_energy_transfer_request(etr))
		{
			return etr.priority;
		}
	}
	if (auto* it = IEnergyTransfer::get_energy_transfer_module_from(_object, EnergyType::Health))
	{
		EnergyTransferRequest etr(EnergyTransferRequest::Query);
		if (it->handle_health_energy_transfer_request(etr))
		{
			return etr.priority;
		}
	}
	return NP;
}

void ModuleDuctEnergy::handle_transfer_energy(float _deltaTime, ::Framework::IModulesOwner* imo)
{
	float transferSpeed = 1.0f;

	EnergyTransfer prevEnergyTransfer = energyTransfer;

	if (bay.objectInBay.is_active())
	{
		bay.timeToNextTransfer -= _deltaTime * transferSpeed;
		Energy energyToTransfer = Energy(0);
		while (bay.timeToNextTransfer <= 0.0f)
		{
			bay.timeToNextTransfer += 1.0f / (bay.transferSpeed.as_float());
			energyToTransfer += Energy(1);
		}
		if (!energyToTransfer.is_zero())
		{
			Energy energyTransferred = Energy::zero();
			for_range(int, iEnergyType, EnergyType::First, EnergyType::Last)
			{
				EnergyType::Type energyType = (EnergyType::Type)iEnergyType;
				if (auto* it = IEnergyTransfer::get_energy_transfer_module_from(bay.objectInBay.get_target(), energyType))
				{
					EnergyTransferRequest etr(EnergyTransferRequest::Deposit /*withdraw here, deposit in object*/);
					etr.energyRequested = energyToTransfer;
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
						if (success)
						{
							energyTransferred += etr.energyResult;
						}
					}
				}
			}
			if (energyTransferred.is_zero())
			{
				energyTransfer = EnergyTransfer::None;
			}
			else
			{
				energyTransfer = EnergyTransfer::Withdraw;
			}
		}
	}
	else
	{
		bay.timeToNextTransfer = 0.0f;
		energyTransfer = EnergyTransfer::None;
	}

	if (prevEnergyTransfer != energyTransfer)
	{
		energyTransferPhysSens.update(energyTransfer != EnergyTransfer::None, bay.objectInBay.get_target());
	}
}

Name ModuleDuctEnergy::get_energy_transfer_temporary_object_name(EnergyTransfer _energyTransfer)
{
	if (_energyTransfer == EnergyTransfer::Withdraw)
	{
		return NAME(withdraw);
	}
	return Name::invalid();
}

void ModuleDuctEnergy::handle_energy_transfer_temporary_objects(EnergyTransfer _prevEnergyTransfer, EnergyTransfer _energyTransfer, ::Framework::IModulesOwner* imo, bool _force)
{
	if (_prevEnergyTransfer == _energyTransfer && ! _force)
	{
		return;
	}


	for_every_ref(o, energyTransferTemporaryObjects)
	{
		if (auto* to = fast_cast<Framework::TemporaryObject>(o))
		{
			Framework::ParticlesUtils::desire_to_deactivate(to);
		}
	}
	energyTransferTemporaryObjects.clear();

	Name toName = get_energy_transfer_temporary_object_name(_energyTransfer);
	if (toName.is_valid())
	{
		if (auto* to = imo->get_temporary_objects())
		{
			if (auto* attachTo = bay.objectInBay.get_target())
			{
				if (auto* spawned = to->spawn_attached_to(get_energy_transfer_temporary_object_name(_energyTransfer), attachTo, Framework::ModuleTemporaryObjects::SpawnParams().at_relative_placement(attachTo->get_presence()->get_centre_of_presence_os())))
				{
					energyTransferTemporaryObjects.push_back(SafePtr<Framework::IModulesOwner>(spawned));
				}
			}
		}
	}
}

void ModuleDuctEnergy::advance_post_move(float _deltaTime)
{
	base::advance_post_move(_deltaTime);

	if (temporaryObjectsRequireReset)
	{
		temporaryObjectsRequireReset = false;
		handle_energy_transfer_temporary_objects(energyTransfer, energyTransfer, get_owner(), true);
	}

	update_object_in_bay(get_owner());
	// check state of switches and transfer energy
	{
		EnergyTransfer prevEnergyTransfer = energyTransfer;
		handle_transfer_energy(_deltaTime, get_owner());
		handle_energy_transfer_temporary_objects(prevEnergyTransfer, energyTransfer, get_owner(), false);
	}
}
