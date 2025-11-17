#include "modules.h"

#include "moduleAI.h"
#include "moduleSound.h"
//
#include "moduleMovementAirship.h"
#include "moduleMovementAirshipProxy.h"
#include "moduleMovementCentipede.h"
#include "moduleMovementH_Craft.h"
#include "moduleMovementInRoomFollower.h"
#include "moduleMovementMonorailCart.h"
#include "moduleMovementPilgrimKeeper.h"
#include "moduleMovementRam.h"
//
#include "modulePresenceBackgroundMoverBase.h"
#include "modulePresencePilgrim.h"
//
#include "custom\mc_carrier.h"
#include "custom\mc_emissiveControl.h"
#include "custom\mc_energyStorage.h"
#include "custom\mc_gameSettingsDependant.h"
#include "custom\mc_grabable.h"
#include "custom\mc_gunTemporaryObject.h"
#include "custom\mc_healthReporter.h"
#include "custom\mc_highlightObjects.h"
#include "custom\mc_hitIndicator.h"
#include "custom\mc_hubIcons.h"
#include "custom\mc_interactiveButtonHandler.h"
#include "custom\mc_investigatorInfoProvider.h"
#include "custom\mc_itemHolder.h"
#include "custom\mc_lootProvider.h"
#include "custom\mc_musicAccidental.h"
#include "custom\mc_operativeDevice.h"
#include "custom\mc_pickup.h"
#include "custom\mc_pilgrimageDevice.h"
#include "custom\mc_powerSource.h"
#include "custom\mc_schematic.h"
#include "custom\mc_switchSidesHandler.h"
#include "custom\mc_weaponLocker.h"
#include "custom\mc_weaponSetupContainer.h"
#include "custom\health\mc_health.h"
#include "custom\health\mc_healthRegen.h"
#include "custom\health\mc_healthPhysicalReaction.h"
//
#include "gameplay\moduleControllableTurret.h"
#include "gameplay\moduleCorrosionLeak.h"
#include "gameplay\moduleDamageWhileTouching.h"
#include "gameplay\moduleDuctEnergy.h"
#include "gameplay\moduleEnergyQuantum.h"
#include "gameplay\moduleEquipment.h"
#include "gameplay\moduleEXM.h"
#include "gameplay\moduleExplosion.h"
#include "gameplay\moduleMultiplePilgrimageDevice.h"
#include "gameplay\moduleOrbItem.h"
#include "gameplay\modulePilgrim.h"
#include "gameplay\modulePilgrimHand.h"
#include "gameplay\modulePortableShield.h"
#include "gameplay\moduleProjectile.h"
//
#include "gameplay\equipment\me_grenade.h"
#include "gameplay\equipment\me_gun.h"
#include "gameplay\equipment\me_mine.h"
#include "gameplay\equipment\me_shield.h"
//
#include "nav\mne_cart.h"

#include "..\..\core\casts.h"
#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\types\names.h"

#include "..\..\framework\advance\advanceContext.h"
#include "..\..\framework\module\registeredModule.h"

using namespace TeaForGodEmperor;

void Modules::initialise_static()
{
	// gameplay
	ModuleControllableTurret::register_itself();
	ModuleCorrosionLeak::register_itself();
	ModuleDamageWhileTouching::register_itself();
	ModuleDuctEnergy::register_itself();
	ModuleEnergyQuantum::register_itself();
	ModuleEquipment::register_itself();
	ModuleEXM::register_itself();
	ModuleExplosion::register_itself();
	ModuleMultiplePilgrimageDevice::register_itself();
	ModuleOrbItem::register_itself();
	ModulePilgrim::register_itself();
	ModulePilgrimHand::register_itself();
	ModulePortableShield::register_itself();
	ModuleProjectile::register_itself();
	// gameplay/equipments
	ModuleEquipments::Grenade::register_itself();
	ModuleEquipments::Gun::register_itself();
	ModuleEquipments::Mine::register_itself();
	ModuleEquipments::Shield::register_itself();

	// custom
	CustomModules::Carrier::register_itself();
	CustomModules::EmissiveControl::register_itself();
	CustomModules::EnergyStorage::register_itself();
	CustomModules::GameSettingsDependant::register_itself();
	CustomModules::Grabable::register_itself();
	CustomModules::GunTemporaryObject::register_itself();
	CustomModules::HighlightObjects::register_itself();
	CustomModules::HitIndicator::register_itself();
	CustomModules::HubIcons::register_itself();
	CustomModules::InteractiveButtonHandler::register_itself();
	CustomModules::InvestigatorInfoProvider::register_itself();
	CustomModules::ItemHolder::register_itself();
	CustomModules::LootProvider::register_itself();
	CustomModules::MusicAccidental::register_itself();
	CustomModules::OperativeDevice::register_itself();
	CustomModules::Pickup::register_itself();
	CustomModules::PilgrimageDevice::register_itself();
	CustomModules::PowerSource::register_itself();
	CustomModules::Schematic::register_itself();
	CustomModules::SwitchSidesHandler::register_itself();
	CustomModules::WeaponLocker::register_itself();
	CustomModules::WeaponSetupContainer::register_itself();
	// custom/health
	CustomModules::Health::register_itself();
	CustomModules::HealthRegen::register_itself();
	CustomModules::HealthReporter::register_itself();
	CustomModules::HealthPhysicalReaction::register_itself();

	// collision
	// ...

	// ai
	ModuleAI::register_itself().be_default();

	// movement
	ModuleMovementAirship::register_itself();
	ModuleMovementAirshipProxy::register_itself();
	ModuleMovementCentipede::register_itself();
	ModuleMovementH_Craft::register_itself();
	ModuleMovementInRoomFollower::register_itself();
	ModuleMovementMonorailCart::register_itself();
	ModuleMovementPilgrimKeeper::register_itself();
	ModuleMovementRam::register_itself();

	// nav element
	ModuleNavElements::Cart::register_itself();

	// presence
	ModulePresenceBackgroundMoverBase::register_itself();
	ModulePresencePilgrim::register_itself();

	// sound
	ModuleSound::register_itself().be_default();
}

Energy Modules::calculate_total_energy_of(Framework::IModulesOwner const * _imo, Optional<EnergyType::Type> const & _ofType, int _flags)
{
	Energy result = Energy::zero();
	if (_imo)
	{
		for_range(int, eType, EnergyType::First, EnergyType::Last)
		{
			if (!_ofType.is_set() || _ofType.get() == eType)
			{
				for_every_ptr(custom, _imo->get_customs())
				{
					if (auto* iet = fast_cast<TeaForGodEmperor::IEnergyTransfer>(custom))
					{
						result += iet->calculate_total_energy_available((EnergyType::Type)eType);
					}
				}
				if (auto* iet = _imo->get_gameplay_as<TeaForGodEmperor::IEnergyTransfer>())
				{
					result += iet->calculate_total_energy_available((EnergyType::Type)eType);
				}
				if (!is_flag_set(_flags, OnlyOwn))
				{
					if (auto* p = _imo->get_gameplay_as<TeaForGodEmperor::ModulePilgrim>())
					{
						result += p->calculate_items_energy((EnergyType::Type)eType);
					}
				}
			}
		}
	}
	return result;
}
