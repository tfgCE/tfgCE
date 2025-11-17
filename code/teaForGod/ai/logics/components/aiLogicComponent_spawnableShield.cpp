#include "aiLogicComponent_spawnableShield.h"

#include "..\..\..\modules\custom\health\mc_health.h"

#include "..\..\..\..\framework\game\game.h"
#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\object\itemType.h"
#include "..\..\..\..\framework\object\object.h"
#include "..\..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define DEBUG_REFILL

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// parameters
DEFINE_STATIC_NAME(spawnShield);
DEFINE_STATIC_NAME(spawnShieldAtSocket);
DEFINE_STATIC_NAME(shieldRebuildSpeed);
DEFINE_STATIC_NAME(shieldRebuildTime);
DEFINE_STATIC_NAME(shieldHealth);

// variables
DEFINE_STATIC_NAME(spawnedShield); // safe ptr imo

//

SpawnableShield::SpawnableShield()
{
}

SpawnableShield::~SpawnableShield()
{
	recall_shield(true);
}

void SpawnableShield::learn_from(SimpleVariableStorage& _parameters)
{
	spawnShield.set_name(_parameters.get_value<Framework::LibraryName>(NAME(spawnShield), spawnShield.get_name()));
	spawnShield.find(Framework::Library::get_current());
	spawnShieldAtSocket = _parameters.get_value<Name>(NAME(spawnShieldAtSocket), spawnShieldAtSocket);
	shieldHealth = Energy::get_from_storage(_parameters, NAME(shieldHealth), shieldHealth);
	shieldMaxHealth = shieldHealth;

	shieldRebuildSpeed = Energy::get_from_storage(_parameters, NAME(shieldRebuildSpeed), shieldRebuildSpeed);

	float shieldRebuildTime = _parameters.get_value<float>(NAME(shieldRebuildTime), 0.0f);
	if (shieldRebuildTime != 0.0f)
	{
		shieldRebuildSpeed = shieldMaxHealth.div(shieldRebuildTime);
	}

	spawnedShieldVar.set_name(NAME(spawnedShield));
}

void SpawnableShield::advance(float _deltaTime)
{
	an_assert(owner, TXT("should be already set"));

	if (!spawnedShield.get() && !spawnShieldIssued)
	{
		shieldHealth += shieldRebuildSpeed.timed(_deltaTime, shieldRebuildTimeBit);
		if (shieldHealth >= shieldMaxHealth)
		{
			shieldHealth = shieldMaxHealth;
			shieldDestroyed = false;
		}
	}

	if (shieldRequested)
	{
		if (!spawnedShield.get())
		{
			if (spawnedShield.is_pointing_at_something())
			{
				// there was a shield but no longer exists
				spawnedShield.clear();
				if (spawnedShieldVar.is_valid())
				{
					spawnedShieldVar.access<SafePtr<Framework::IModulesOwner>>().clear();
				}

				shieldDestroyed = true;
				shieldHealth = Energy::zero();
			}
			else
			{
				// create shield if possible
				if (!shieldDestroyed)
				{
					create_shield();
				}
			}
		}
	}
	else if (spawnedShield.get())
	{
		recall_shield(false);
	}
}

void SpawnableShield::create_shield()
{
	recall_shield(false);

	if (!spawnShield.get())
	{
		return;
	}

	if (shieldDestroyed)
	{
		return; // can't
	}

	auto* imo = owner;

	SafePtr<Framework::IModulesOwner> imoSafe;
	imoSafe = imo;

	spawnShieldIssued = true;

	Framework::Game::get()->add_immediate_sync_world_job(TXT("spawn shield"),
		[imoSafe, this]()
		{
			Framework::Object* spawnedObject = nullptr;

			if (auto* imo = imoSafe.get())
			{
				auto* inRoom = imo->get_presence()->get_in_room();
				Framework::ScopedAutoActivationGroup saag(fast_cast<Framework::WorldObject>(imoSafe.get()), inRoom);

				Framework::Game::get()->perform_sync_world_job(TXT("spawn shield"), [this, &spawnedObject, inRoom]()
					{
						spawnedObject = spawnShield->create(spawnShield->get_name().to_string());
						spawnedObject->init(inRoom->get_in_sub_world());
					});
				spawnedObject->access_individual_random_generator() = Random::Generator();

				if (imoSafe.is_set())
				{
					spawnedObject->access_variables().set_from(imoSafe->get_variables());
					spawnedObject->learn_from(imoSafe->access_variables());
				}

				spawnedObject->initialise_modules();
				spawnedObject->set_instigator(imoSafe.get());
				spawnedObject->be_non_autonomous();

				// store 
				spawnedShield = spawnedObject;
				spawnShieldIssued = false;
				if (spawnShieldAtSocket.is_valid())
				{
					spawnedObject->get_presence()->attach_to_socket(imo, spawnShieldAtSocket, true);
				}
				if (auto* h = spawnedObject->get_custom<CustomModules::Health>())
				{
					h->reset_energy_state(shieldHealth);
				}

				if (!spawnedShieldVar.is_valid())
				{
					spawnedShieldVar.look_up<SafePtr<Framework::IModulesOwner>>(imo->access_variables());
				}

				if (spawnedShieldVar.is_valid())
				{
					spawnedShieldVar.access<SafePtr<Framework::IModulesOwner>>() = spawnedObject;
				}

				// auto activate
			}
		});
}

void SpawnableShield::recall_shield(bool _removeFromWorldImmediately)
{
	if (auto* ss = spawnedShield.get())
	{
		if (auto* h = ss->get_custom<CustomModules::Health>())
		{
			shieldHealth = max(Energy::zero(), h->calculate_total_energy_available(EnergyType::Health));
			if (h->is_alive())
			{
				h->perform_death();
			}
		}
		else
		{
			spawnedShield->cease_to_exist(_removeFromWorldImmediately);
		}
	}
	spawnedShield.clear();
	if (spawnedShieldVar.is_valid())
	{
		spawnedShieldVar.access<SafePtr<Framework::IModulesOwner>>().clear();
	}
}
