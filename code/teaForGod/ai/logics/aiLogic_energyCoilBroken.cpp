#include "aiLogic_energyCoilBroken.h"

#include "exms\aiLogic_exmEnergyDispatcher.h"

#include "..\..\game\game.h"
#include "..\..\game\gameDirector.h"

#include "..\..\modules\gameplay\moduleEnergyQuantum.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// variables
DEFINE_STATIC_NAME(energy);
DEFINE_STATIC_NAME(energyType);
DEFINE_STATIC_NAME(useStoryFact);
DEFINE_STATIC_NAME(energyQuantumType);

// state variables
DEFINE_STATIC_NAME(energyInStorage);

// sockets
DEFINE_STATIC_NAME(bay);

// temporary objects
DEFINE_STATIC_NAME(damage1);
DEFINE_STATIC_NAME(damage2);

//

REGISTER_FOR_FAST_CAST(EnergyCoilBroken);

EnergyCoilBroken::EnergyCoilBroken(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

EnergyCoilBroken::~EnergyCoilBroken()
{
}

void EnergyCoilBroken::advance(float _deltaTime)
{
	base::advance(_deltaTime);
}

void EnergyCoilBroken::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);

	{
		String et = _parameters.get_value<String>(NAME(energyType), String::empty());
		if (!et.is_empty())
		{
			energyType = EnergyType::parse(et, energyType);
		}
	}
	energy = Energy::get_from_storage(_parameters, NAME(energy), energy);
	useStoryFact = _parameters.get_value<Name>(NAME(useStoryFact), useStoryFact);
	{
		Framework::LibraryName sit = _parameters.get_value<Framework::LibraryName>(NAME(energyQuantumType), Framework::LibraryName::invalid());
		if (sit.is_valid())
		{
			energyQuantumType.set_name(Framework::LibraryName(sit));
			energyQuantumType.find(Framework::Library::get_current());
		}
	}

	if (auto* mind = get_mind())
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
			// store
			imo->access_variables().access<Energy>(NAME(energyInStorage)) = energy;
			imo->access_variables().access<String>(NAME(energyType)) = EnergyType::to_char(energyType);
		}
	}

	ai_log(this, TXT("starting energy %S"), energy.as_string_auto_decimals().to_char());
}

LATENT_FUNCTION(EnergyCoilBroken::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai energy coil broken] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(Array<float>, timeLeftDamage);
	LATENT_VAR(Random::Generator, rg);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<EnergyCoilBroken>(logic);

	{
		for_every(t, timeLeftDamage)
		{
			*t -= LATENT_DELTA_TIME;
		}
	}

	LATENT_BEGIN_CODE();

	self->baySocket.set_name(NAME(bay));
	self->baySocket.look_up(imo->get_appearance()->get_mesh());

	ai_log(self, TXT("energy coil broken, hello!"));

	while (timeLeftDamage.get_size() < 2)
	{
		timeLeftDamage.push_back(rg.get_float(0.1f, 3.0f));
	}

	LATENT_WAIT(1.0f); // give it a bit of time
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("energy coil broken %S, energy in storage: %S [%S]"), imo->ai_get_name().to_char(), self->energy.as_string().to_char(), imo->get_variables().get_value<Energy>(NAME(energyInStorage), self->energy).as_string().to_char());
#endif

	LATENT_WAIT(0.1f);

	{
		self->energy = imo->get_variables().get_value<Energy>(NAME(energyInStorage), self->energy);

		bool spawn = ! self->energy.is_zero();
		if (spawn && self->useStoryFact.is_valid())
		{
			if (auto* gd = TeaForGodEmperor::GameDirector::get_active())
			{
				Concurrency::ScopedMRSWLockRead lock(gd->access_story_execution().access_facts_lock());
				spawn = !gd->access_story_execution().get_facts().get_tag(self->useStoryFact);
			}
		}

		if (spawn)
		{
			if (auto* sit = self->energyQuantumType.get())
			{
				auto* inRoom = imo->get_presence()->get_in_room();
				Framework::DelayedObjectCreation* doc = new Framework::DelayedObjectCreation();
				doc->activateImmediatelyEvenIfRoomVisible = true;
				doc->wmpOwnerObject = imo;
				doc->inRoom = inRoom;
				doc->name = TXT("energy item");
				doc->objectType = sit;
				Transform placement = imo->get_appearance()->calculate_socket_os(self->baySocket.get_index());
				placement = imo->get_appearance()->get_os_to_ws_transform().to_world(placement);
				doc->placement = placement;
				doc->randomGenerator = imo->get_individual_random_generator();
				doc->priority = 1000;
				doc->checkSpaceBlockers = false;

				inRoom->collect_variables(doc->variables);

				self->energyItemDOC = doc;

				Energy energy = self->energy;
				EnergyType::Type energyType = self->energyType;
				if (energyType == EnergyType::Health)
				{
					energy = energy.mul(GameSettings::get().difficulty.lootHealth);
				}
				if (energyType == EnergyType::Ammo)
				{
					energy = energy.mul(GameSettings::get().difficulty.lootAmmo);
				}
				doc->post_initialise_modules_function = [energy, energyType](Framework::Object* spawnedObject)
					{
						if (auto* meq = spawnedObject->get_gameplay_as<ModuleEnergyQuantum>())
						{
							meq->start_energy_quantum_setup();
							meq->set_energy(energy);
							meq->set_energy_type(energyType);
							meq->end_energy_quantum_setup();
							meq->no_time_limit();
						}
					};

				Game::get()->queue_delayed_object_creation(doc);
			}
		}
	}

	while (true)
	{
		if (auto* doc = self->energyItemDOC.get())
		{
			if (doc->is_done())
			{
				self->energyItem = doc->createdObject.get();
				self->energyItem->set_instigator(imo);
				self->energyItemDOC.clear();
				self->energyItemExists = true;
			}
		}
		if (self->energyItemExists &&
			! self->energyItem.get())
		{
			self->energyItemExists = false;

			if (self->useStoryFact.is_valid())
			{
				if (auto* gd = TeaForGodEmperor::GameDirector::get_active())
				{
					Concurrency::ScopedMRSWLockWrite lock(gd->access_story_execution().access_facts_lock());
					gd->access_story_execution().access_facts().set_tag(self->useStoryFact);
				}
			}

			// store as state variable
			{
				imo->access_variables().access<Energy>(NAME(energyInStorage)) = Energy::zero(); // taken
			}
		}

		{
			for_every(t, timeLeftDamage)
			{
				if (*t < 0.0f)
				{
					*t = rg.get_float(0.5f, 4.0f);
					if (imo->get_presence()->get_in_room()->get_distance_to_recently_seen_by_player() < 3)
					{
						if (auto* to = imo->get_temporary_objects())
						{
							to->spawn_all(for_everys_index(t) == 0 ? NAME(damage1) : NAME(damage2));
						}
					}
				}
			}
		}

		LATENT_WAIT(Random::get_float(0.1f, 0.2f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}
