#include "pilgrimBlackboard.h"

#include "..\teaForGod.h"

#include "..\game\gameLog.h"
#include "..\game\gameSettings.h"
#include "..\game\persistence.h"

#include "..\library\exmType.h"
#include "..\library\gameDefinition.h"

#include "..\modules\gameplay\moduleEXM.h"
#include "..\modules\gameplay\modulePilgrim.h"

#include "..\..\core\containers\arrayStack.h"
#include "..\..\framework\object\actorType.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

int PilgrimBlackboard::calculate_passive_exm_slot_count(PilgrimSetup const& _setup, OPTIONAL_ PilgrimInventory const* _inventory)
{
	int slotCount = GameDefinition::get_chosen()->get_default_passive_exm_slot_count();

	ARRAY_PREALLOC_SIZE(TeaForGodEmperor::EXMType const*, exmTypes, 60);

	// check only permanent EXMs, passive and active EXMs should not affect slot count

	if (_inventory)
	{
		Concurrency::ScopedMRSWLockWrite lock(_inventory->exmsLock);
		for_every(exm, _inventory->permanentEXMs)
		{
			if (auto* exmType = TeaForGodEmperor::EXMType::find(*exm))
			{
				exmTypes.push_back(exmType);
			}
		}
	}

	for_every_ptr(exmType, exmTypes)
	{
		if (auto* mexm = fast_cast<ModuleEXMData>(exmType->get_actor_type()->get_gameplay()->get_data()))
		{
			slotCount += mexm->get_additional_passive_exm_slot_count();
		}
	}

	an_assert_log_always(slotCount <= MAX_PASSIVE_EXMS, TXT("slot count : %i <= %i"), slotCount, MAX_PASSIVE_EXMS);
	slotCount = clamp(slotCount, 0, MAX_PASSIVE_EXMS);

	return slotCount;
}

void PilgrimBlackboard::update(PilgrimSetup const& _setup, OPTIONAL_ PilgrimInventory const* _inventory, OUT_ AdditionalInfos* _ai)
{
	if (_ai)
	{
		assign_optional_out_param(_ai->body, String::empty());
		assign_optional_out_param(_ai->hands[Hand::Left], String::empty());
		assign_optional_out_param(_ai->hands[Hand::Right], String::empty());
	}
	health = Energy::zero();
	healthBackup = Energy::zero();
	healthRegenRate = Energy::zero();
	healthRegenCooldownCoef = EnergyCoef::zero();
	healthDamageReceivedCoef = EnergyCoef::zero();
	explosionResistance = EnergyCoef::zero();
	passiveEXMSlots = GameSettings::get().difficulty.passiveEXMSlots;
	pocketLevel = 0;
	healthForMeleeKill = GameplayBalance::health_for_melee_kill();
	ammoForMeleeKill = GameplayBalance::ammo_for_melee_kill();

	for_count(int, iHand, Hand::MAX)
	{
		Hand::Type iHandType = (Hand::Type)iHand;
		auto& hand = hands[iHand];
		auto& handSetup = _setup.get_hand((Hand::Type)iHand);
		auto& otherHandSetup = _setup.get_hand((Hand::Type)(1 - iHand));
		hand.handEnergyStorage = Energy::zero();
		hand.mainEquipment.chamberSizeCoef = EnergyCoef::zero();
		hand.mainEquipment.damageCoef = EnergyCoef::zero();
		hand.mainEquipment.armourPiercing = EnergyCoef::zero();
		hand.mainEquipment.magazineSizeCoef = EnergyCoef::zero();
		hand.mainEquipment.magazineMinSize = Energy::zero();
		hand.mainEquipment.magazineMinShotCount = 0;
		hand.mainEquipment.magazineCooldownCoef = 0.0f;
		hand.mainEquipment.storageOutputSpeedCoef = EnergyCoef::zero();
		hand.mainEquipment.magazineOutputSpeedCoef = EnergyCoef::zero();
		hand.mainEquipment.storageOutputSpeedAdd = Energy::zero();
		hand.mainEquipment.magazineOutputSpeedAdd = Energy::zero();
		hand.mainEquipment.antiDeflection = 0.0f;
		hand.mainEquipment.maxDist = 0.0f;
		hand.pullEnergyDistanceRecentlyRenderedRoom = 0.0f;
		hand.pullEnergyDistanceRecentlyNotRenderedRoom = 0.0f;
		hand.energyInhaleCoef = EnergyCoef::zero();
		hand.physicalViolence = PhysicalViolence();
		struct EXMRef
		{
			int priority = 0;
			TeaForGodEmperor::EXMType const* exmType = nullptr;
			bool otherHand = false;
			EXMRef() {}
			EXMRef(TeaForGodEmperor::EXMType const* _exmType, bool _otherHand)
			: exmType(_exmType)
			, otherHand(_otherHand)
			{
				if (exmType)
				{
					if (auto* mexm = fast_cast<ModuleEXMData>(exmType->get_actor_type()->get_gameplay()->get_data()))
					{
						priority = mexm->get_priority();
					}
				}
			}

			static int compare(void const* _a, void const* _b)
			{
				// lowest priority first, we're overriding
				EXMRef const* b = plain_cast<EXMRef>(_b);
				EXMRef const* a = plain_cast<EXMRef>(_a);
				if (a->priority < b->priority)
				{
					return A_BEFORE_B;
				}
				if (a->priority > b->priority)
				{
					return B_BEFORE_A;
				}
				return A_AS_B;
			}
		};
		ARRAY_PREALLOC_SIZE(EXMRef, exms, 60);
		for_every(exm, otherHandSetup.passiveEXMs)
		{
			if (exm->exm.is_valid())
			{
				if (auto* exmType = TeaForGodEmperor::EXMType::find(exm->exm))
				{
					exms.push_back(EXMRef(exmType, true));
				}
			}
		}
		if (otherHandSetup.activeEXM.exm.is_valid())
		{
			if (auto* exmType = TeaForGodEmperor::EXMType::find(otherHandSetup.activeEXM.exm))
			{
				exms.push_back(EXMRef(exmType, true));
			}
		}
		for_every(exm, handSetup.passiveEXMs)
		{
			if (exm->exm.is_valid())
			{
				if (auto* exmType = TeaForGodEmperor::EXMType::find(exm->exm))
				{
					exms.push_back(EXMRef(exmType, false));
				}
			}
		}
		if (handSetup.activeEXM.exm.is_valid())
		{
			if (auto* exmType = TeaForGodEmperor::EXMType::find(handSetup.activeEXM.exm))
			{
				exms.push_back(EXMRef(exmType, false));
			}
		}
		if (_inventory)
		{
			Concurrency::ScopedMRSWLockWrite lock(_inventory->exmsLock);
			for_every(exm, _inventory->permanentEXMs)
			{
				if (auto* exmType = TeaForGodEmperor::EXMType::find(*exm))
				{
					exms.push_back(EXMRef(exmType, iHand == 0 /* it has to be on any hand, doesn't matter which one */));
				}
			}			
		}
		sort(exms);
		for_every(exmRef, exms)
		{
			if (auto* exmType = exmRef->exmType)
			{
				// exm infos?

				if (auto* mexm = fast_cast<ModuleEXMData>(exmType->get_actor_type()->get_gameplay()->get_data()))
				{
					// we process exms from both hands, otherHand param below means that EXM comes from the other hand
					// some results affect whole body, some affect only equipment in hand
					// otherHand EXMs should be considered only to this very hand (if to body, they are processed via their own hand)

					// general (use only current hand as using other would double the results)
					if (!exmRef->otherHand)
					{
						health += mexm->get_health();
						healthBackup += mexm->get_health_backup();
						healthRegenRate += mexm->get_health_regen_rate();
						healthRegenCooldownCoef += mexm->get_health_regen_cooldown_coef();
						healthDamageReceivedCoef += mexm->get_health_damage_received_coef();
						explosionResistance += mexm->get_explosion_resistance();
						passiveEXMSlots += mexm->get_additional_passive_exm_slot_count();
						pocketLevel += mexm->get_additional_pocket_level();
						healthForMeleeKill = max(healthForMeleeKill, mexm->get_health_for_melee_kill());
						ammoForMeleeKill = max(ammoForMeleeKill, mexm->get_ammo_for_melee_kill());
					}

					// per hand (we use both, placement is just where we put it in, affects both hands)
					if (!GameSettings::get().difficulty.commonHandEnergyStorage || GameSettings::any_use_hand(iHandType) == GameSettings::actual_hand(iHandType))
					{
						hand.handEnergyStorage += mexm->get_hand_energy_storage();
					}
					hand.mainEquipment.chamberSizeCoef += mexm->get_main_equipment_chamber_size_coef();
					hand.mainEquipment.damageCoef += mexm->get_main_equipment_damage_coef();
					hand.mainEquipment.armourPiercing += mexm->get_main_equipment_armour_piercing();
					hand.mainEquipment.magazineSizeCoef += mexm->get_main_equipment_magazine_size_coef();
					hand.mainEquipment.magazineMinSize = max(hand.mainEquipment.magazineMinSize, mexm->get_main_equipment_magazine_min_size());
					hand.mainEquipment.magazineMinShotCount = max(hand.mainEquipment.magazineMinShotCount, mexm->get_main_equipment_magazine_min_shot_count());
					hand.mainEquipment.magazineCooldownCoef += mexm->get_main_equipment_magazine_cooldown_coef();
					hand.mainEquipment.storageOutputSpeedCoef += mexm->get_main_equipment_storage_output_speed_coef();
					hand.mainEquipment.magazineOutputSpeedCoef += mexm->get_main_equipment_magazine_output_speed_coef();
					hand.mainEquipment.storageOutputSpeedAdd += mexm->get_main_equipment_storage_output_speed_add();
					hand.mainEquipment.magazineOutputSpeedAdd += mexm->get_main_equipment_magazine_output_speed_add();
					hand.mainEquipment.antiDeflection += mexm->get_main_equipment_anti_deflection();
					hand.mainEquipment.maxDist += mexm->get_main_equipment_max_dist();
					hand.pullEnergyDistanceRecentlyRenderedRoom = max(hand.pullEnergyDistanceRecentlyRenderedRoom, mexm->get_pull_energy_distance_recently_rendered_room());
					hand.pullEnergyDistanceRecentlyNotRenderedRoom = max(hand.pullEnergyDistanceRecentlyNotRenderedRoom, mexm->get_pull_energy_distance_recently_not_rendered_room());
					hand.energyInhaleCoef += mexm->get_energy_inhale_coef();
					hand.physicalViolence.override_with(mexm->get_physical_violence());
				}
			}
		}
	}

	// minimums
	healthRegenCooldownCoef = max(healthRegenCooldownCoef, EnergyCoef::percent(-100 + 0));
	healthDamageReceivedCoef = max(healthDamageReceivedCoef, EnergyCoef::percent(-100 + 25));
	for_count(int, iHand, Hand::MAX)
	{
		auto& hand = hands[iHand];
		// those with asterisk are not really required, as should never go down
		// remember that we start at 0
		hand.mainEquipment.chamberSizeCoef = max(hand.mainEquipment.chamberSizeCoef, EnergyCoef::percent(-100 + 10));
		hand.mainEquipment.damageCoef = max(hand.mainEquipment.damageCoef, EnergyCoef::percent(-100 + 10)); // *
		hand.mainEquipment.armourPiercing = clamp(hand.mainEquipment.armourPiercing, EnergyCoef::zero(), EnergyCoef::percent(100)); // *
		hand.mainEquipment.antiDeflection = max(-1.0f, hand.mainEquipment.antiDeflection); // *
		hand.mainEquipment.magazineSizeCoef = max(hand.mainEquipment.magazineSizeCoef, EnergyCoef::percent(-100 + 10)); // *
		hand.mainEquipment.magazineMinSize = max(hand.mainEquipment.magazineMinSize, Energy::zero());
		hand.mainEquipment.magazineMinShotCount = max(hand.mainEquipment.magazineMinShotCount, 0);
		hand.mainEquipment.magazineCooldownCoef = max(hand.mainEquipment.magazineCooldownCoef, -1.0f);
		hand.mainEquipment.storageOutputSpeedCoef = max(hand.mainEquipment.storageOutputSpeedCoef, EnergyCoef::percent(-100 + 10)); // *
		hand.mainEquipment.magazineOutputSpeedCoef = max(hand.mainEquipment.magazineOutputSpeedCoef, EnergyCoef::percent(-100 + 10)); // *

		// this is forced for time being
		hand.pullEnergyDistanceRecentlyRenderedRoom = max(hand.pullEnergyDistanceRecentlyRenderedRoom, 50.0f);
		hand.pullEnergyDistanceRecentlyNotRenderedRoom = max(hand.pullEnergyDistanceRecentlyNotRenderedRoom, 5.0f);
	}
}

// various utilities
Energy PilgrimBlackboard::get_health_modifier_for(Framework::IModulesOwner const* _imo)
{
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.health;
		}
	}
	return Energy::zero();
}

Energy PilgrimBlackboard::get_health_backup_modifier_for(Framework::IModulesOwner const* _imo)
{
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.healthBackup;
		}
	}
	return Energy::zero();
}

Energy PilgrimBlackboard::get_health_regen_rate_modifier_for(Framework::IModulesOwner const* _imo)
{
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.healthRegenRate;
		}
	}
	return Energy::zero();
}

EnergyCoef PilgrimBlackboard::get_health_regen_cooldown_coef_modifier_for(Framework::IModulesOwner const* _imo)
{
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.healthRegenCooldownCoef;
		}
	}
	return EnergyCoef::zero();
}

EnergyCoef PilgrimBlackboard::get_health_damage_received_coef_modifier_for(Framework::IModulesOwner const* _imo)
{
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.healthDamageReceivedCoef;
		}
	}
	return EnergyCoef::zero();
}

EnergyCoef PilgrimBlackboard::get_explosion_resistance(Framework::IModulesOwner const* _imo)
{
	if (auto* topOwner = _imo->get_top_instigator())
	{
		if (auto* mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.explosionResistance;
		}
	}
	return EnergyCoef::zero();
}

int PilgrimBlackboard::get_passive_exm_slot_count(Framework::IModulesOwner const* _imo)
{
	if (auto* topOwner = _imo->get_top_instigator())
	{
		if (auto* mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.passiveEXMSlots;
		}
	}
	return 1;
}

int PilgrimBlackboard::get_pocket_level(Framework::IModulesOwner const* _imo)
{
	if (auto* topOwner = _imo->get_top_instigator())
	{
		if (auto* mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.pocketLevel;
		}
	}
	return 1;
}

Energy PilgrimBlackboard::get_health_for_melee_kill(Framework::IModulesOwner const* _imo)
{
	if (auto* topOwner = _imo->get_top_instigator())
	{
		if (auto* mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.healthForMeleeKill;
		}
	}
	return Energy::zero();
}

Energy PilgrimBlackboard::get_ammo_for_melee_kill(Framework::IModulesOwner const* _imo)
{
	if (auto* topOwner = _imo->get_top_instigator())
	{
		if (auto* mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.ammoForMeleeKill;
		}
	}
	return Energy::zero();
}

Energy PilgrimBlackboard::get_hand_energy_storage(Framework::IModulesOwner const* _imo, Hand::Type _hand)
{
	if (! Hand::is_valid(_hand))
	{
		return Energy::zero();
	}
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.hands[_hand].handEnergyStorage;
		}
	}
	return Energy::zero();
}

EnergyCoef PilgrimBlackboard::get_main_equipment_chamber_size_coef_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand)
{
	if (! Hand::is_valid(_hand))
	{
		return EnergyCoef::zero();
	}
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.hands[_hand].mainEquipment.chamberSizeCoef;
		}
	}
	return EnergyCoef::zero();
}

EnergyCoef PilgrimBlackboard::get_main_equipment_damage_coef_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand)
{
	if (! Hand::is_valid(_hand))
	{
		return EnergyCoef::zero();
	}
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.hands[_hand].mainEquipment.damageCoef;
		}
	}
	return EnergyCoef::zero();
}

EnergyCoef PilgrimBlackboard::get_main_equipment_armour_piercing_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand)
{
	if (! Hand::is_valid(_hand))
	{
		return EnergyCoef::zero();
	}
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.hands[_hand].mainEquipment.armourPiercing;
		}
	}
	return EnergyCoef::zero();
}

float PilgrimBlackboard::get_main_equipment_anti_deflection_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand)
{
	if (! Hand::is_valid(_hand))
	{
		return 0.0f;
	}
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.hands[_hand].mainEquipment.antiDeflection;
		}
	}
	return 0.0f;
}

float PilgrimBlackboard::get_main_equipment_max_dist_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand)
{
	if (! Hand::is_valid(_hand))
	{
		return 0.0f;
	}
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.hands[_hand].mainEquipment.maxDist;
		}
	}
	return 0.0f;
}

EnergyCoef PilgrimBlackboard::get_main_equipment_magazine_size_coef_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand)
{
	if (! Hand::is_valid(_hand))
	{
		return EnergyCoef::zero();
	}
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.hands[_hand].mainEquipment.magazineSizeCoef;
		}
	}
	return EnergyCoef::zero();
}

Energy PilgrimBlackboard::get_main_equipment_magazine_min_size_for(Framework::IModulesOwner const* _imo, Hand::Type _hand)
{
	if (! Hand::is_valid(_hand))
	{
		return Energy::zero();
	}
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.hands[_hand].mainEquipment.magazineMinSize;
		}
	}
	return Energy::zero();
}

int PilgrimBlackboard::get_main_equipment_magazine_min_shot_count_for(Framework::IModulesOwner const* _imo, Hand::Type _hand)
{
	if (! Hand::is_valid(_hand))
	{
		return 0;
	}
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.hands[_hand].mainEquipment.magazineMinShotCount;
		}
	}
	return 0;
}

float PilgrimBlackboard::get_main_equipment_cooldown_coef_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand)
{
	if (! Hand::is_valid(_hand))
	{
		return 0.0f;
	}
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.hands[_hand].mainEquipment.magazineCooldownCoef;
		}
	}
	return 0.0f;
}

bool PilgrimBlackboard::get_main_equipment_using_magazine(Framework::IModulesOwner const* _imo, Hand::Type _hand)
{
	if (! Hand::is_valid(_hand))
	{
		return false;
	}
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.hands[_hand].mainEquipment.magazineMinSize.is_positive() ||
				   blackboard.hands[_hand].mainEquipment.magazineMinShotCount > 0;
		}
	}
	return false;
}

EnergyCoef PilgrimBlackboard::get_main_equipment_storage_output_speed_coef_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand)
{
	if (! Hand::is_valid(_hand))
	{
		return EnergyCoef::zero();
	}
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.hands[_hand].mainEquipment.storageOutputSpeedCoef;
		}
	}
	return EnergyCoef::zero();
}

EnergyCoef PilgrimBlackboard::get_main_equipment_magazine_output_speed_coef_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand)
{
	if (! Hand::is_valid(_hand))
	{
		return EnergyCoef::zero();
	}
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.hands[_hand].mainEquipment.magazineOutputSpeedCoef;
		}
	}
	return EnergyCoef::zero();
}

Energy PilgrimBlackboard::get_main_equipment_storage_output_speed_add_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand)
{
	if (! Hand::is_valid(_hand))
	{
		return Energy::zero();
	}
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.hands[_hand].mainEquipment.storageOutputSpeedAdd;
		}
	}
	return Energy::zero();
}

Energy PilgrimBlackboard::get_main_equipment_magazine_output_speed_add_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand)
{
	if (! Hand::is_valid(_hand))
	{
		return Energy::zero();
	}
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.hands[_hand].mainEquipment.magazineOutputSpeedAdd;
		}
	}
	return Energy::zero();
}

float PilgrimBlackboard::get_pull_energy_distance(Framework::IModulesOwner const* _imo, Hand::Type _hand, bool _recentlyRenderedRoom)
{
	if (! Hand::is_valid(_hand))
	{
		return 0.0f;
	}
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return _recentlyRenderedRoom? blackboard.hands[_hand].pullEnergyDistanceRecentlyRenderedRoom : blackboard.hands[_hand].pullEnergyDistanceRecentlyNotRenderedRoom;
		}
	}
	return 0.0f;
}

EnergyCoef PilgrimBlackboard::get_energy_inhale_coef(Framework::IModulesOwner const* _imo, Hand::Type _hand)
{
	if (! Hand::is_valid(_hand))
	{
		return EnergyCoef::zero();
	}
	if (auto * topOwner = _imo->get_top_instigator())
	{
		if (auto * mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			return blackboard.hands[_hand].energyInhaleCoef;
		}
	}
	return EnergyCoef::zero();
}

void PilgrimBlackboard::update_physical_violence_damage(Framework::IModulesOwner const* _imo, Hand::Type _hand, bool _strong, REF_ Energy& _damage)
{
	if (!Hand::is_valid(_hand))
	{
		return;
	}
	if (auto* topOwner = _imo->get_top_instigator())
	{
		if (auto* mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			auto& pv = blackboard.hands[_hand].physicalViolence;
			auto& pvDamage = _strong ? pv.damageStrong : pv.damage;
			if (pvDamage.is_set())
			{
				_damage = pvDamage.get();
			}
		}
	}
	return;
}

void PilgrimBlackboard::update_physical_violence_speeds(Framework::IModulesOwner const* _imo, Hand::Type _hand, REF_ float& _minSpeed, REF_ float& _minSpeedStrong)
{
	if (!Hand::is_valid(_hand))
	{
		return;
	}
	if (auto* topOwner = _imo->get_top_instigator())
	{
		if (auto* mp = topOwner->get_gameplay_as<ModulePilgrim>())
		{
			auto& blackboard = mp->get_pilgrim_blackboard();
			auto& pv = blackboard.hands[_hand].physicalViolence;
			if (pv.minSpeed.is_set())
			{
				_minSpeed *= pv.minSpeed.get();
			}
			if (pv.minSpeedStrong.is_set())
			{
				_minSpeedStrong *= pv.minSpeedStrong.get();
			}
		}
	}
	return;
}
