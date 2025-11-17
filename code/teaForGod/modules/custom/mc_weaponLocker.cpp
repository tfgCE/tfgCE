#include "mc_weaponLocker.h"

#include "mc_itemHolder.h"

#include "..\gameplay\modulePilgrim.h"
#include "..\gameplay\equipment\me_gun.h"

#include "..\..\game\game.h"
#include "..\..\game\weaponSetup.h"

#include "..\..\library\library.h"

#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\object\itemType.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// module parameters
DEFINE_STATIC_NAME(weaponInsideVar);
DEFINE_STATIC_NAME(weaponInsideRGVar);

// variables
DEFINE_STATIC_NAME(weaponInside);
DEFINE_STATIC_NAME(weaponInsideRG);

// global references
DEFINE_STATIC_NAME_STR(grWeaponItemTypeToUseWithWeaponParts, TXT("weapon item type to use with weapon parts"));

// mesh generator parameters / variables
DEFINE_STATIC_NAME_STR(withLeftRestPoint, TXT("with left rest point"));
DEFINE_STATIC_NAME_STR(withRightRestPoint, TXT("with right rest point"));

// timers
DEFINE_STATIC_NAME(holdWeapon);

//

REGISTER_FOR_FAST_CAST(WeaponLocker);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new WeaponLocker(_owner);
}

Framework::RegisteredModule<Framework::ModuleCustom> & WeaponLocker::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("weaponLocker")), create_module);
}

WeaponLocker::WeaponLocker(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

WeaponLocker::~WeaponLocker()
{
}

void WeaponLocker::setup_with(Framework::ModuleData const* _moduleData)
{
	base::setup_with(_moduleData);

	weaponInsideVar = _moduleData->get_parameter<Name>(this, NAME(weaponInsideVar), NAME(weaponInside));
	weaponInsideRGVar = _moduleData->get_parameter<Name>(this, NAME(weaponInsideRGVar), NAME(weaponInsideRG));
}

void WeaponLocker::store_weapon_setup()
{
	if (auto* imo = get_owner())
	{
		auto& ws = imo->access_variables().access<WeaponSetup>(weaponInsideVar);
		bool clearWS = true;
		if (auto* ih = imo->get_custom<CustomModules::ItemHolder>())
		{
			if (auto* held = ih->get_held())
			{
				if (auto* gun = held->get_gameplay_as<ModuleEquipments::Gun>())
				{
					auto& rg = imo->access_variables().access<Random::Generator>(weaponInsideRGVar);
					rg = held->get_individual_random_generator();
					ws.copy_from(gun->get_weapon_setup());
					clearWS = false;
				}
			}
		}
		if (clearWS)
		{
			ws.clear();
		}
	}
}

void WeaponLocker::on_restore_pilgrimage_device_state()
{
	create_weapon_from_stored_weapon_setup();
}

void WeaponLocker::create_weapon_from_stored_weapon_setup()
{
	if (auto* imo = get_owner())
	{
		SafePtr<Framework::IModulesOwner> safeIMO(imo);
		Game::get()->add_async_world_job_top_priority(TXT("create weapon for weapon mixer container"), [this, safeIMO]()
		{
			if (auto* imo = safeIMO.get())
			{
				auto* game = Game::get_as<Game>();
				if (auto* ows = imo->get_variables().get_existing<WeaponSetup>(weaponInsideVar))
				{
					if (!ows->is_empty())
					{
						auto* useRG = imo->get_variables().get_existing<Random::Generator>(weaponInsideRGVar);

						WeaponSetup ws(nullptr);
						ws.copy_from(*ows);

						Framework::ItemType* useItemType = Library::get_current()->get_global_references().get<Framework::ItemType>(NAME(grWeaponItemTypeToUseWithWeaponParts));

						if (useItemType)
						{
							Framework::IModulesOwner* weaponObject = nullptr;

							Framework::ActivationGroupPtr activationGroup = game->push_new_activation_group();
							{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
								output(TXT("create weapon for weapon mixer from setup \"%S\""), useItemType->get_name().to_string().to_char());
#endif

								an_assert(Persistence::access_current_if_exists());
								auto* persistence = Persistence::access_current_if_exists();
								WeaponSetup useWeaponSetup(persistence);
								if (!ws.is_empty())
								{
									useWeaponSetup.copy_for_different_persistence(ws);
								}

								{
									for_every(part, useWeaponSetup.get_parts())
									{
										if (auto* p = part->part.get())
										{
											p->make_sure_schematic_mesh_exists();
										}
									}
								}

								useItemType->load_on_demand_if_required();

								Game::get_as<Game>()->perform_sync_world_job(TXT("create object"), [&weaponObject, useItemType, imo]()
									{
										weaponObject = useItemType->create(String(TXT("in weapon mixer")));
										weaponObject->init(imo->get_in_sub_world());
									});

								{
									Random::Generator rg = imo->get_individual_random_generator();
									if (useRG && !useRG->is_zero_seed())
									{
										rg = *useRG;
									}
									else
									{
										rg = Random::Generator(); // spawn random one
									}
									weaponObject->access_individual_random_generator() = rg;
								}

								{
									// rest points are also set via gun module
									//equipment->access_variables().access<bool>(_hand == Hand::Left ? NAME(withLeftRestPoint) : NAME(withRightRestPoint)) = true;
									weaponObject->access_variables().access<bool>(NAME(withLeftRestPoint)) = true;
									weaponObject->access_variables().access<bool>(NAME(withRightRestPoint)) = true;
									if (auto* pi = PilgrimageInstance::get())
									{
										if (auto* p = pi->get_pilgrimage())
										{
											weaponObject->access_variables().set_from(p->get_equipment_parameters());
										}
									}
									{
										todo_multiplayer_issue(TXT("we just get player here"));
										if (auto* g = Game::get_as<Game>())
										{
											if (auto* pa = g->access_player().get_actor())
											{
												if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
												{
													if (auto* pd = mp->get_pilgrim_data())
													{
														weaponObject->access_variables().set_from(pd->get_equipment_parameters());
													}
												}
											}
										}
									}
								}

								weaponObject->initialise_modules([&useWeaponSetup](Framework::Module* _module)
								{
									if (auto* g = fast_cast<ModuleEquipments::Gun>(_module))
									{
										// the stats differ then a bit as we have different storages
										//g->be_pilgrim_weapon();

										g->setup_with_weapon_setup(useWeaponSetup);
									}
								});

#ifdef AN_ALLOW_EXTENSIVE_LOGS
								output(TXT("weapon mixer object created!"));
#endif

								{
									// timer will start when the object gets activated
									SafePtr<Framework::IModulesOwner> safeIMO(imo);
									SafePtr<Framework::IModulesOwner> safeWeaponObject(weaponObject);
									weaponObject->set_timer(0.01f, NAME(holdWeapon), [safeIMO, safeWeaponObject](Framework::IModulesOwner* _imo)
										{
											Game::get()->add_immediate_sync_world_job(TXT("put a weapon into the container"), [safeIMO, safeWeaponObject]()
											{
												auto* imo = safeIMO.get();
												auto* weaponObject = safeWeaponObject.get();
												if (imo && weaponObject)
												{
													if (auto* ih = imo->get_custom<CustomModules::ItemHolder>())
													{
														ih->hold(weaponObject);
													}
												}
											});
										});
								}
							}
							game->request_ready_and_activate_all_inactive(game->get_current_activation_group());
							game->pop_activation_group(activationGroup.get());
						}
					}
				}
			}
		});
	}
}

