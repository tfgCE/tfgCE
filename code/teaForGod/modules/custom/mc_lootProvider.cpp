#include "mc_lootProvider.h"

#include "health\mc_health.h"

#include "..\..\gameplayBalance.h"

#include "..\..\game\gameConfig.h"
#include "..\..\game\gameLog.h"
#include "..\..\game\persistence.h"
#include "..\..\library\gameDefinition.h"
#include "..\..\library\library.h"
#include "..\..\library\lootSelector.h"
#include "..\..\modules\custom\mc_weaponSetupContainer.h"
#include "..\..\modules\gameplay\moduleEnergyQuantum.h"
#include "..\..\modules\gameplay\moduleOrbItem.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\gameplay\equipment\me_gun.h"

#include "..\..\..\framework\game\game.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\item.h"

#include "..\..\..\core\random\randomUtils.h"

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// game log
DEFINE_STATIC_NAME(madeEnergyQuantumRelease);

// variables/module setup
DEFINE_STATIC_NAME(noLoot);

// library global referencer
DEFINE_STATIC_NAME_STR(grWeaponPartItem, TXT("weapon part item type"));

//

REGISTER_FOR_FAST_CAST(LootProvider);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new LootProvider(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new LootProviderData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleCustom>& LootProvider::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("lootProvider")), create_module, create_module_data);
}

LootProvider::LootProvider(Framework::IModulesOwner* _owner)
	: base(_owner)
{
}

LootProvider::~LootProvider()
{
	destroy_not_released_loot();
}

bool LootProvider::are_weapon_parts_from_weapon_setup_container() const
{
	return lootProviderData && lootProviderData->weaponPartsFromWeaponSetupContainer;
}

Name const& LootProvider::get_weapon_parts_from_weapon_setup_container_id() const
{
	return lootProviderData? lootProviderData->weaponPartsFromWeaponSetupContainerId : Name::invalid();
}

bool LootProvider::is_weapon_setup_from_weapon_setup_container() const
{
	return lootProviderData && lootProviderData->weaponSetupFromWeaponSetupContainer;
}

Name const& LootProvider::get_weapon_setup_from_weapon_setup_container_id() const
{
	return lootProviderData? lootProviderData->weaponSetupFromWeaponSetupContainerId : Name::invalid();
}

void LootProvider::on_owner_destroy()
{
	base::on_owner_destroy();

	destroy_not_released_loot();
}

void LootProvider::setup_with(Framework::ModuleData const* _moduleData)
{
	base::setup_with(_moduleData);
	lootProviderData = fast_cast<LootProviderData>(_moduleData);
	if (_moduleData)
	{
		noLoot = _moduleData->get_parameter<bool>(this, NAME(noLoot), noLoot);
	}
}

void LootProvider::ready_to_activate()
{
	base::ready_to_activate();

	if (!noLoot)
	{
		auto rg = get_owner()->get_individual_random_generator();
		rg.advance_seed(4598, 108235);
		float probability = lootProviderData->probability;
		{
			if (EXMType::does_player_have_equipped(EXMID::Passive::weapon_collector()))
			{
				if (auto* exm = EXMType::find(EXMID::Passive::weapon_collector()))
				{
					// for time being this should be enough, we just assume that we use weapon setup containers to create weapons
					// if so, probability of having something is much higher
					bool hasWeapons = false;
					if (are_weapon_parts_from_weapon_setup_container() ||
						is_weapon_setup_from_weapon_setup_container())
					{
						hasWeapons = true;
					}
					if (hasWeapons)
					{
						probability = 1.0f - (1.0f - probability) * 0.2f;
					}
				}
			}
		}
		if (probability >= 1.0f || rg.get_chance(probability))
		{
			LootSelectorContext lsc;
			lsc.lootProvider = this;
#ifdef WITH_CRYSTALITE
			lsc.crystalite = lootProviderData->crystalite;
#endif
			LootSelector::create_loot_for(REF_ lsc);
		}
	}
}

void LootProvider::add_loot(Framework::IModulesOwner* _item, bool _weaponSetupContainerLoot)
{
	if (_weaponSetupContainerLoot)
	{
		weaponSetupContainerLootItems.push_back(SafePtr<Framework::IModulesOwner>(_item));
	}
	else
	{
		lootItems.push_back(SafePtr<Framework::IModulesOwner>(_item));
	}
}

Tags const& LootProvider::get_loot_tags() const
{
	return lootProviderData->lootTags;
}

Energy LootProvider::get_experience() const
{
	return lootProviderData->provideExperience;
}

void LootProvider::release_loot(Vector3 const & _hitRelativeDir, Framework::IModulesOwner* _instigator)
{
	SafePtr<Framework::IModulesOwner> imo;
	imo = get_owner();
	Transform placement = imo->get_presence()->get_centre_of_presence_transform_WS();
	Vector3 velocity = imo->get_presence()->get_velocity_linear();
#ifdef AN_DEVELOPMENT
#ifndef AN_CLANG
	auto* inRoom = imo->get_presence()->get_in_room();
#endif
#endif
	Vector3 hitRelativeDir = _hitRelativeDir;

	if (! weaponSetupContainerLootItems.is_empty())
	{
		// move to loot items, but not all
		while (! weaponSetupContainerLootItems.is_empty())
		{
			int idx = Random::get_int(weaponSetupContainerLootItems.get_size());
			lootItems.push_back(weaponSetupContainerLootItems[idx]);
			weaponSetupContainerLootItems.remove_fast_at(idx);
		}
	}

	while (!lootItems.is_empty())
	{
		if (auto* lootItem = lootItems.get_last().get())
		{
			if (auto* oi = lootItem->get_gameplay_as<ModuleOrbItem>())
			{
				oi->set_source(OrbItemSource::Kill);
			}

			bool release = true;
#ifdef WITH_CRYSTALITE
			if (GameSettings::get().difficulty.instantCrystalite ||
				! GameSettings::get().difficulty.withCrystalite)
			{
				if (auto* moi = lootItem->get_gameplay_as<ModuleOrbItem>())
				{
					if (moi->is_crystalite())
					{
						if (_instigator)
						{
							auto* ti = _instigator->get_top_instigator();
							if (auto* p = ti->get_gameplay_as<ModulePilgrim>())
							{
								if (GameSettings::get().difficulty.instantCrystalite)
								{
									moi->process_taken(p);
								}
								release = false;
							}
						}
						else if (! GameSettings::get().difficulty.withCrystalite)
						{
							release = false;
						}
					}
				}
			}
#endif
			lootItem->be_autonomous();
			lootItem->resume_advancement();
			if (release)
			{
				Transform placeAt = get_owner()->get_presence()->get_centre_of_presence_os();
				placeAt.set_translation(placeAt.get_translation() + get_owner()->get_presence()->get_presence_centre_distance() * Random::get_float(-0.3f, 0.3f));
				lootItem->get_presence()->place_at(get_owner(), Name::invalid(), placeAt);

				Vector3 useVelocityLinear = velocity * 0.7f;
				float useRotationSpeed = 0.0f;

				if (auto* meq = lootItem->get_gameplay_as<ModuleEnergyQuantum>())
				{
					Vector3 addToVelocity = Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f)).normal();
					if (!hitRelativeDir.is_zero())
					{
						addToVelocity -= 5.0f * placement.vector_to_world(hitRelativeDir);
						addToVelocity.normalise();
					}
					useVelocityLinear += addToVelocity * magic_number 0.8f;
					useRotationSpeed = Random::get(Range(40.0f, 100.0f));
					if (Framework::GameUtils::is_local_player(_instigator))
					{
						GameLog::get().add_entry(GameLog::Entry(NAME(madeEnergyQuantumRelease)));
					}

					// alter energy during the run
					{
						meq->start_energy_quantum_setup();
						//
						Energy e = meq->get_energy();
						if (meq->get_energy_type() == EnergyType::Ammo)
						{
							e = GameConfig::apply_loot_energy_quantum_ammo(e);
						}
						meq->set_energy(e);
						//
						meq->end_energy_quantum_setup();
					}
				}
				else
				{
					Vector3 addToVelocity = Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f)).normal();
					useVelocityLinear = useVelocityLinear + addToVelocity * magic_number 0.4f;
					if (lootItem->get_gameplay_as<ModuleOrbItem>())
					{
						useRotationSpeed = Random::get(Range(40.0f, 100.0f));
					}
					else
					{
						useRotationSpeed = Random::get(Range(10.0f, 50.0f));
					}
				}
				Rotator3 useVelocityRotation = Rotator3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f)) * useRotationSpeed;

				lootItem->get_presence()->set_velocity_linear(useVelocityLinear);
				lootItem->get_presence()->set_velocity_rotation(useVelocityRotation);

				if (auto* h = lootItem->get_custom<CustomModules::Health>())
				{
					h->be_invincible(0.2f); // be invincible, now we are going to die and instigator will be nullified, we want to make sure this object won't be destroyed due to explosion
				}
			}
			else
			{
				lootItem->cease_to_exist(false);
			}
		}
		lootItems.pop_back();
	}

	destroy_not_released_loot();
}

void LootProvider::destroy_not_released_loot()
{
	lootItems.add_from(weaponSetupContainerLootItems);
	weaponSetupContainerLootItems.clear();

	while (!lootItems.is_empty())
	{
		if (auto* lootItem = lootItems.get_last().get())
		{
			lootItem->be_autonomous();
			lootItem->cease_to_exist(false);
		}
		lootItems.pop_back();
	}
}

//

REGISTER_FOR_FAST_CAST(LootProviderData);

LootProviderData::LootProviderData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

LootProviderData::~LootProviderData()
{
}

bool LootProviderData::read_parameter_from(IO::XML::Attribute const* _attr, Framework::LibraryLoadingContext& _lc)
{
	if (_attr->get_name() == TXT("probability"))
	{
		probability = _attr->get_as_float();
		return true;
	}
	if (_attr->get_name() == TXT("weaponPartsFromWeaponSetupContainer"))
	{
		weaponPartsFromWeaponSetupContainer = _attr->get_as_bool();
		return true;
	}
	if (_attr->get_name() == TXT("weaponPartsFromWeaponSetupContainerId"))
	{
		weaponPartsFromWeaponSetupContainerId = _attr->get_as_name();
		return true;
	}
	if (_attr->get_name() == TXT("weaponSetupFromWeaponSetupContainer"))
	{
		weaponSetupFromWeaponSetupContainer = _attr->get_as_bool();
		return true;
	}
	if (_attr->get_name() == TXT("weaponSetupFromWeaponSetupContainerId"))
	{
		weaponSetupFromWeaponSetupContainerId = _attr->get_as_name();
		return true;
	}
	if (_attr->get_name() == TXT("lootTags"))
	{
		bool result = true;
		result &= lootTags.load_from_string(_attr->get_as_string());
		return result;
	}
	else
	{
		return base::read_parameter_from(_attr, _lc);
	}
}

bool LootProviderData::read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("weaponPartsFromWeaponSetupContainer"))
	{
		weaponPartsFromWeaponSetupContainer = _node->get_bool(true);
		weaponPartsFromWeaponSetupContainerId = _node->get_name_attribute(TXT("id"), weaponPartsFromWeaponSetupContainerId);
		return true;
	}
	if (_node->get_name() == TXT("weaponSetupFromWeaponSetupContainer"))
	{
		weaponSetupFromWeaponSetupContainer = _node->get_bool(true);
		weaponSetupFromWeaponSetupContainerId = _node->get_name_attribute(TXT("id"), weaponSetupFromWeaponSetupContainerId);
		return true;
	}
	if (_node->get_name() == TXT("probability"))
	{
		probability = _node->get_float(probability);
		return true;
	}
	if (_node->get_name() == TXT("lootTags"))
	{
		bool result = true;
		result &= lootTags.load_from_string(_node->get_text());
		return result;
	}
#ifdef WITH_CRYSTALITE
	else if (_node->get_name() == TXT("crystalite"))
	{
		bool result = true;
		crystalite = _node->get_int_attribute(TXT("amount"), max(1, crystalite));
		return result;
	}
#endif
	else if (_node->get_name() == TXT("provideExperience"))
	{
		bool result = true;
		provideExperience.load_from_xml(_node, TXT("amount"));
		return result;
	}
	else
	{
		return base::read_parameter_from(_node, _lc);
	}
}

void LootProviderData::output_log(LogInfoContext& _log) const
{
	_log.log(TXT("probability: %.3f"), probability);
	LOG_INDENT(_log);
	if (!lootTags.is_empty())
	{
		_log.log(TXT("loot tags: %S"), lootTags.to_string().to_char());
	}
}
