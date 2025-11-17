#include "gse_spawnEnergyQuantum.h"

#include "..\..\game\game.h"
#include "..\..\modules\gameplay\moduleEnergyQuantum.h"

#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

// script execution variable
DEFINE_STATIC_NAME(asyncJobDone);

// library global referencer
DEFINE_STATIC_NAME_STR(energyQuantumHealth, TXT("energy quantum health"));
DEFINE_STATIC_NAME_STR(energyQuantumAmmo, TXT("energy quantum ammo"));
DEFINE_STATIC_NAME_STR(energyQuantumPowerAmmo, TXT("energy quantum power ammo"));

// energy quantum types
DEFINE_STATIC_NAME(ammo);
DEFINE_STATIC_NAME(health);
DEFINE_STATIC_NAME_STR(powerAmmo, TXT("power ammo"));

//

bool SpawnEnergyQuantum::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	relativeLocation.load_from_xml_child_node(_node, TXT("relLocation"));
	energy.load_from_attribute_or_from_child(_node, TXT("energy"));
	sideEffectDamage.load_from_attribute_or_from_child(_node, TXT("sideEffectDamage"));
	type = _node->get_name_attribute(TXT("type"), type);
	noTimeLimit = _node->get_bool_attribute_or_from_child_presence(TXT("noTimeLimit"), noTimeLimit);

	storeVar = _node->get_name_attribute(TXT("storeVar"), storeVar);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type SpawnEnergyQuantum::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Entered))
	{
		SafePtr<Framework::GameScript::ScriptExecution> execution(&_execution);
		execution->access_variables().access<bool>(NAME(asyncJobDone)) = false;

		if (auto* game = Game::get_as<Game>())
		{
			auto relativeLocationCopy = relativeLocation;
			auto typeCopy = type;
			auto energyCopy = energy;
			auto sideEffectDamageCopy = sideEffectDamage;
			auto noTimeLimitCopy = noTimeLimit;
			auto storeVarCopy = storeVar;
			game->add_async_world_job(TXT("spawn energy quantum"),
				[execution, relativeLocationCopy, typeCopy, energyCopy, sideEffectDamageCopy, noTimeLimitCopy, storeVarCopy]()
			{
				if (auto* game = Game::get_as<Game>())
				{
					if (auto* playerActor = game->access_player().get_actor())
					{
						Framework::ScopedAutoActivationGroup saag(game->get_world());

						Transform placement = playerActor->get_presence()->get_placement().to_world(Transform(relativeLocationCopy, Quat::identity));

						Framework::LibraryName eqAmmoName = Framework::Library::get_current()->get_global_references().get_name(NAME(energyQuantumAmmo));
						Framework::LibraryName eqHealthName = Framework::Library::get_current()->get_global_references().get_name(NAME(energyQuantumHealth));
						Framework::LibraryName eqPowerAmmoName = Framework::Library::get_current()->get_global_references().get_name(NAME(energyQuantumPowerAmmo));
						{
							Framework::ItemType* itAmmo = eqAmmoName.is_valid() ? Framework::Library::get_current()->get_item_types().find(eqAmmoName) : nullptr;
							Framework::ItemType* itHealth = eqHealthName.is_valid() ? Framework::Library::get_current()->get_item_types().find(eqHealthName) : nullptr;
							Framework::ItemType* itPowerAmmo = eqPowerAmmoName.is_valid() ? Framework::Library::get_current()->get_item_types().find(eqPowerAmmoName) : nullptr;

							EnergyType::Type energyType = EnergyType::Ammo;
							Framework::ItemType* it = nullptr;
							if (typeCopy == NAME(ammo)) { it = itAmmo; energyType = EnergyType::Ammo; }
							if (typeCopy == NAME(health)) { it = itHealth; energyType = EnergyType::Health; }
							if (typeCopy == NAME(powerAmmo)) { it = itPowerAmmo; energyType = EnergyType::Ammo; }
							if (it)
							{
								Energy energy = energyCopy;
								Energy sideEffectDamage = sideEffectDamageCopy;

								Framework::Object* spawnedObject = nullptr;
								
								Framework::Room* inRoom = playerActor->get_presence()->get_in_room();

								{
									Framework::ScopedAutoActivationGroup saag(fast_cast<Framework::WorldObject>(playerActor), inRoom);

									if (it)
									{
										it->load_on_demand_if_required();

										Framework::Game::get()->perform_sync_world_job(TXT("spawn energy quantum"), [it, &spawnedObject, inRoom]()
										{
											spawnedObject = it->create(it->get_name().to_string());
											spawnedObject->init(inRoom->get_in_sub_world());
										});
									}
									else
									{
										an_assert(false);
									}
									spawnedObject->access_individual_random_generator() = Random::Generator();
									spawnedObject->initialise_modules();
									spawnedObject->be_autonomous();

									if (auto* meq = spawnedObject->get_gameplay_as<ModuleEnergyQuantum>())
									{
										meq->start_energy_quantum_setup();
										meq->set_energy(energy);
										meq->set_energy_type(energyType);
										meq->set_side_effect_damage(sideEffectDamage);
										meq->end_energy_quantum_setup();
										if (noTimeLimitCopy)
										{
											meq->no_time_limit();
										}
									}

									game->perform_sync_world_job(TXT("place energy quantum"), [spawnedObject, inRoom, placement]() { spawnedObject->get_presence()->place_in_room(inRoom, placement); });
								}

								if (storeVarCopy.is_valid())
								{
									execution->access_variables().access< SafePtr<Framework::IModulesOwner> >(storeVarCopy) = spawnedObject;
								}
							}
							else
							{
								error(TXT("no energy quantum found! maybe not defined in global referencer?"));
							}
						}
					}
				}
				if (auto* ex = execution.get())
				{
					ex->access_variables().access<bool>(NAME(asyncJobDone)) = true;
				}
			});
		}
	}

	if (_execution.get_variables().get_value<bool>(NAME(asyncJobDone), false))
	{
		return Framework::GameScript::ScriptExecutionResult::Continue;
	}
	else
	{
		return Framework::GameScript::ScriptExecutionResult::Repeat;
	}
}
