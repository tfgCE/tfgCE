#include "moduleEXM.h"

#include "moduleEnergyQuantum.h"
#include "modulePilgrim.h"

#include "..\custom\mc_emissiveControl.h"

#include "..\..\game\gameLog.h"

#include "..\..\library\exmType.h"

#include "..\..\utils\buildStatsInfo.h"

#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\modules.h"

#ifdef AN_CLANG
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// module parameters
DEFINE_STATIC_NAME(canBeActiveNotSelected);

// emissive layers
DEFINE_STATIC_NAME(active);
DEFINE_STATIC_NAME(selected);
DEFINE_STATIC_NAME(physicalViolence);

// sounds
DEFINE_STATIC_NAME(activate);
DEFINE_STATIC_NAME(deactivate);

//

REGISTER_FOR_FAST_CAST(ModuleEXM);

static Framework::ModuleGameplay* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleEXM(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new ModuleEXMData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleGameplay> & ModuleEXM::register_itself()
{
	return Framework::Modules::gameplay.register_module(String(TXT("exm")), create_module, create_module_data);
}

ModuleEXM::ModuleEXM(Framework::IModulesOwner* _owner)
: base( _owner )
{
}

ModuleEXM::~ModuleEXM()
{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("destroying %S"), get_owner() ? get_owner()->ai_get_name().to_char() : TXT("??"));
#endif
}

void ModuleEXM::set_exm_type(EXMType const* _exmType)
{
	exmType = _exmType;
}

EXMType const* ModuleEXM::get_exm_type() const
{
	return exmType.get();
}

void ModuleEXM::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	exmData = fast_cast<ModuleEXMData>(_moduleData);

	exmCanBeActiveNotSelected = _moduleData->get_parameter<bool>(this, NAME(canBeActiveNotSelected), exmCanBeActiveNotSelected);
}

void ModuleEXM::reset()
{
	base::reset();
}

void ModuleEXM::initialise()
{
	base::initialise();
}

Optional<float> ModuleEXM::get_cooldown_left_pt() const
{
	if (cooldownTimeLeft > 0.0f)
	{
		float cooldownTime = get_cooldown_time();
		if (cooldownTime > 0.0f)
		{
			return clamp(cooldownTimeLeft / cooldownTime, 0.0f, 1.0f);
		}
	}
	return NP;
}

void ModuleEXM::mark_exm_selected(bool _selected)
{
	exmIsSelected = _selected;
	if (!exmIsSelected && !exmCanBeActiveNotSelected)
	{
		exmIsActive = false;
	}
	if (auto * em = get_owner()->get_custom<CustomModules::EmissiveControl>())
	{
		if (_selected)
		{
			em->emissive_activate(NAME(selected));
			if (! exmIsActive)
			{
				em->emissive_deactivate(NAME(active));
			}
		}
		else
		{
			if (!exmIsActive)
			{
				em->emissive_deactivate(NAME(active));
			}
			em->emissive_deactivate(NAME(selected));
		}
	}
}

bool ModuleEXM::mark_exm_active(bool _active)
{
	bool wasActive = exmIsActive;
	if (exmIsSelected || exmCanBeActiveNotSelected)
	{
		if (_active && cooldownTimeLeft > 0.0f)
		{
			_active = false;
		}
		exmIsActive = _active;
		if (exmIsActive)
		{
			if (!get_cost_single().is_zero() || !get_cost_per_second().is_zero())
			{
				exmIsActive = false;
				if (hand.is_set())
				{
					if (auto * topInstigator = get_owner()->get_top_instigator())
					{
						if (auto * mp = topInstigator->get_gameplay_as<ModulePilgrim>())
						{
							exmIsActive = true;
							if (exmIsActive && !get_cost_per_second().is_zero() &&
								!mp->can_consume_equipment_energy_for(hand.get(), get_cost_per_second()))
							{
								exmIsActive = false;
							}
							if (exmIsActive && !get_cost_single().is_zero() &&
								!mp->consume_equipment_energy_for(hand.get(), get_cost_single()))
							{
								exmIsActive = false;
							}

							mp->access_overlay_info().done_tip(PilgrimOverlayInfoTip::InputActivateEXM, hand);
						}
					}
				}
			}
		}
	}
	else
	{
		exmIsActive = false;
	}

	if (wasActive != exmIsActive)
	{
		if (auto * s = get_owner()->get_sound())
		{
			s->play_sound(exmIsActive ? NAME(activate) : NAME(deactivate));
		}
		if (hand.is_set())
		{
			if (Framework::GameUtils::is_local_player(get_owner()->get_top_instigator()))
			{
				PhysicalSensations::SingleSensation s(PhysicalSensations::SingleSensation::UsedSomething);
				s.for_hand(hand.get());
				PhysicalSensations::start_sensation(s);
			}
		}
		if (exmIsActive)
		{
			cooldownTimeLeft = get_cooldown_time();
			++exmActivatedCount;
			if (Framework::GameUtils::is_local_player(get_owner()->get_top_instigator()))
			{
				GameLog::get().increase_counter(GameLog::UsedActiveEXM);
			}
		}
		else
		{
			++exmDeactivatedCount;
		}
	}

	if (auto * em = get_owner()->get_custom<CustomModules::EmissiveControl>())
	{
		// selected should be done already
		if (exmIsActive)
		{
			em->emissive_activate(NAME(active));
		}
		else
		{
			em->emissive_deactivate(NAME(active));
		}
	}

	return exmIsActive;
}

bool ModuleEXM::mark_exm_active_blink()
{
	if (exmIsSelected)
	{
		if (cooldownTimeLeft > 0.0f)
		{
			return false;
		}
		bool isOk = true;
		if (!get_cost_single().is_zero())
		{
			isOk = false;
		}
		if (hand.is_set())
		{
			if (auto * topInstigator = get_owner()->get_top_instigator())
			{
				if (auto * mp = topInstigator->get_gameplay_as<ModulePilgrim>())
				{
					if (!get_cost_single().is_zero())
					{
						isOk = mp->consume_equipment_energy_for(hand.get(), get_cost_single());
					}

					mp->access_overlay_info().done_tip(PilgrimOverlayInfoTip::InputActivateEXM, hand);
				}
			}
		}
		if (isOk)
		{
			cooldownTimeLeft = get_cooldown_time();
			if (auto * em = get_owner()->get_custom<CustomModules::EmissiveControl>())
			{
				em->emissive_activate(NAME(active), 0.0f);
			}
			deactivateBlinkIn = 0.2f;
			if (auto * s = get_owner()->get_sound())
			{
				s->play_sound(NAME(activate));
			}
			if (hand.is_set())
			{
				if (Framework::GameUtils::is_local_player(get_owner()->get_top_instigator()))
				{
					PhysicalSensations::SingleSensation s(PhysicalSensations::SingleSensation::UsedSomething);
					s.for_hand(hand.get());
					PhysicalSensations::start_sensation(s);
				}
			}
			++exmActivatedCount;
			if (Framework::GameUtils::is_local_player(get_owner()->get_top_instigator()))
			{
				GameLog::get().increase_counter(GameLog::UsedActiveEXM);
			}
			return true;
		}
	}
	return false;
}

void ModuleEXM::blink_on_physical_violence()
{
	if (auto* em = get_owner()->get_custom<CustomModules::EmissiveControl>())
	{
		em->emissive_activate(NAME(physicalViolence), 0.0f);
		deactivatePhysicalViolenceBlinkIn = 1.0f;
	}
}

void ModuleEXM::advance_post_move(float _deltaTime)
{
	base::advance_post_move(_deltaTime);

	if (exmIsActive)
	{
		cooldownTimeLeft = get_cooldown_time();
	}
	else
	{
		cooldownTimeLeft = max(0.0f, cooldownTimeLeft - _deltaTime);
	}

	if (deactivateBlinkIn > 0.0f)
	{
		deactivateBlinkIn = max(0.0f, deactivateBlinkIn - _deltaTime);
		if (deactivateBlinkIn <= 0.0f)
		{
			if (auto * em = get_owner()->get_custom<CustomModules::EmissiveControl>())
			{
				em->emissive_deactivate(NAME(active));
			}
		}
	}

	if (deactivatePhysicalViolenceBlinkIn > 0.0f)
	{
		deactivatePhysicalViolenceBlinkIn = max(0.0f, deactivatePhysicalViolenceBlinkIn - _deltaTime);
		if (deactivatePhysicalViolenceBlinkIn <= 0.0f)
		{
			if (auto * em = get_owner()->get_custom<CustomModules::EmissiveControl>())
			{
				em->emissive_deactivate(NAME(physicalViolence));
			}
		}
	}

	if (exmIsActive)
	{
		if (!get_cost_per_second().is_zero() && hand.is_set())
		{
			Energy costDelta = get_cost_per_second().timed(_deltaTime, costPerSecondMissingBit);
			if (!costDelta.is_zero())
			{
				bool costOK = false;
				if (auto * topInstigator = get_owner()->get_top_instigator())
				{
					if (auto * mp = topInstigator->get_gameplay_as<ModulePilgrim>())
					{
						if (mp->consume_equipment_energy_for(hand.get(), costDelta))
						{
							costOK = true;
						}
					}
				}

				if (!costOK)
				{
					mark_exm_active(false);
				}
			}
		}
	}
}

#define GET_FROM_MODULE(type, get_function_name) type ModuleEXM::get_function_name() const { return exmData->get_function_name(); }
#define GET_FROM_MODULE_CONST_REF(type, get_function_name) type const & ModuleEXM::get_function_name() const { return exmData->get_function_name(); }

GET_FROM_MODULE(Energy const&, get_cost_single);
GET_FROM_MODULE(Energy const&, get_cost_per_second);
GET_FROM_MODULE(EnergyCoef const&, get_recall_percentage);

GET_FROM_MODULE(float, get_cooldown_time);

GET_FROM_MODULE(Energy, get_health);
GET_FROM_MODULE(Energy, get_health_backup);
GET_FROM_MODULE(Energy, get_health_regen_rate);
GET_FROM_MODULE(EnergyCoef, get_health_regen_cooldown_coef);
GET_FROM_MODULE(EnergyCoef, get_health_damage_received_coef);

GET_FROM_MODULE(Energy, get_hand_energy_storage);

GET_FROM_MODULE(EnergyCoef, get_main_equipment_chamber_size_coef);
GET_FROM_MODULE(EnergyCoef, get_main_equipment_damage_coef);
GET_FROM_MODULE(EnergyCoef, get_main_equipment_armour_piercing);
GET_FROM_MODULE(EnergyCoef, get_main_equipment_magazine_size_coef);
GET_FROM_MODULE(Energy, get_main_equipment_magazine_min_size);
GET_FROM_MODULE(int, get_main_equipment_magazine_min_shot_count);
GET_FROM_MODULE(EnergyCoef, get_main_equipment_storage_output_speed_coef);
GET_FROM_MODULE(EnergyCoef, get_main_equipment_magazine_output_speed_coef);
GET_FROM_MODULE(Energy, get_main_equipment_storage_output_speed_add);
GET_FROM_MODULE(Energy, get_main_equipment_magazine_output_speed_add);
GET_FROM_MODULE(float, get_main_equipment_anti_deflection);
GET_FROM_MODULE(float, get_main_equipment_max_dist);

GET_FROM_MODULE(float, get_pull_energy_distance_recently_rendered_room);
GET_FROM_MODULE(float, get_pull_energy_distance_recently_not_rendered_room);

GET_FROM_MODULE(EnergyCoef, get_energy_inhale_coef);

GET_FROM_MODULE_CONST_REF(PhysicalViolence, get_physical_violence);

#undef GET_FROM_MODULE

//

REGISTER_FOR_FAST_CAST(ModuleEXMData);

ModuleEXMData::ModuleEXMData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

ModuleEXMData::~ModuleEXMData()
{
}

bool ModuleEXMData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	costPerSecond.load_from_xml(_node, TXT("costPerSecond"));
	costSingle.load_from_xml(_node, TXT("costSingle"));
	recallPercentage.load_from_xml(_node, TXT("recallPercentage"));

	cooldownTime = _node->get_float_attribute_or_from_child(TXT("cooldownTime"), cooldownTime);

	health.load_from_xml(_node, TXT("health"));
	healthBackup.load_from_xml(_node, TXT("healthBackup"));
	healthRegenRate.load_from_xml(_node, TXT("healthRegenRate"));
	healthRegenCooldownCoef.load_from_xml(_node, TXT("healthRegenCooldownCoef"));
	healthDamageReceivedCoef.load_from_xml(_node, TXT("healthDamageReceivedCoef"));
	explosionResistance.load_from_xml(_node, TXT("explosionResistance"));

	additionalPassiveEXMSlotCount = _node->get_int_attribute_or_from_child(TXT("additionalPassiveEXMSlotCount"), additionalPassiveEXMSlotCount);
	additionalPocketLevel = _node->get_int_attribute_or_from_child(TXT("additionalPocketLevel"), additionalPocketLevel);

	healthForMeleeKill.load_from_xml(_node, TXT("healthForMeleeKill"));
	ammoForMeleeKill.load_from_xml(_node, TXT("ammoForMeleeKill"));

	handEnergyStorage.load_from_xml(_node, TXT("handEnergyStorage"));

	mainEquipmentChamberSizeCoef.load_from_xml(_node, TXT("mainEquipmentChamberSizeCoef"));
	mainEquipmentDamageCoef.load_from_xml(_node, TXT("mainEquipmentDamageCoef"));
	mainEquipmentArmourPiercing.load_from_xml(_node, TXT("mainEquipmentArmourPiercing"));
	mainEquipmentMagazineSizeCoef.load_from_xml(_node, TXT("mainEquipmentMagazineSizeCoef"));
	mainEquipmentMagazineMinSize.load_from_xml(_node, TXT("mainEquipmentMagazineMinSize"));
	mainEquipmentMagazineMinShotCount = _node->get_int_attribute_or_from_child(TXT("mainEquipmentMagazineMinShotCount"), mainEquipmentMagazineMinShotCount);
	mainEquipmentMagazineCooldownCoef = _node->get_float_attribute_or_from_child(TXT("mainEquipmentMagazineCooldownCoef"), mainEquipmentMagazineCooldownCoef);
	mainEquipmentStorageOutputSpeedCoef.load_from_xml(_node, TXT("mainEquipmentStorageOutputSpeedCoef"));
	mainEquipmentMagazineOutputSpeedCoef.load_from_xml(_node, TXT("mainEquipmentMagazineOutputSpeedCoef"));
	mainEquipmentStorageOutputSpeedAdd.load_from_xml(_node, TXT("mainEquipmentStorageOutputSpeedAdd"));
	mainEquipmentMagazineOutputSpeedAdd.load_from_xml(_node, TXT("mainEquipmentMagazineOutputSpeedAdd"));
	mainEquipmentAntiDeflection = _node->get_float_attribute_or_from_child(TXT("mainEquipmentAntiDeflection"), mainEquipmentAntiDeflection);
	mainEquipmentMaxDist = _node->get_float_attribute_or_from_child(TXT("mainEquipmentMaxDist"), mainEquipmentMaxDist);

	pullEnergyDistanceRecentlyRenderedRoom = _node->get_float_attribute_or_from_child(TXT("pullEnergyDistance"), pullEnergyDistanceRecentlyRenderedRoom);
	pullEnergyDistanceRecentlyRenderedRoom = _node->get_float_attribute_or_from_child(TXT("pullEnergyDistanceRecentlyRenderedRoom"), pullEnergyDistanceRecentlyRenderedRoom);
	pullEnergyDistanceRecentlyNotRenderedRoom = _node->get_float_attribute_or_from_child(TXT("pullEnergyDistanceRecentlyNotRenderedRoom"), pullEnergyDistanceRecentlyNotRenderedRoom);
	if (pullEnergyDistanceRecentlyNotRenderedRoom == 0.0f)
	{
		pullEnergyDistanceRecentlyNotRenderedRoom = pullEnergyDistanceRecentlyRenderedRoom * 0.1f;
	}

	energyInhaleCoef.load_from_xml(_node, TXT("energyInhaleCoef"));

	result &= physicalViolence.load_from_xml(_node->first_child_named(TXT("physicalViolence")));

	return result;
}

bool ModuleEXMData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

void ModuleEXMData::prepare_to_unload()
{
	base::prepare_to_unload();
}