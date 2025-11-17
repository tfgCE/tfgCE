#include "moduleEnergyQuantum.h"

#include "moduleEquipment.h"
#include "modulePilgrim.h"
#include "modulePilgrimHand.h"
#include "..\custom\health\mc_health.h"

#include "..\..\game\damage.h"
#include "..\..\game\game.h"
#include "..\..\game\gameLog.h"
#include "..\..\game\gameSettings.h"

#include "..\..\..\framework\ai\aiMessage.h"
#include "..\..\..\framework\debug\previewGame.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// module params
DEFINE_STATIC_NAME(energy);
DEFINE_STATIC_NAME(energyType);
DEFINE_STATIC_NAME(timeAlive);
DEFINE_STATIC_NAME(noTimeLimit);
DEFINE_STATIC_NAME(quantumSize);
DEFINE_STATIC_NAME(explodeOnCollisionWithFlags);
DEFINE_STATIC_NAME(sideEffectDamage);
DEFINE_STATIC_NAME(sideEffectAIMessage);
DEFINE_STATIC_NAME(explodeOnDeath);

// temporary objects
DEFINE_STATIC_NAME(explode);

// game log
DEFINE_STATIC_NAME(grabbedEnergyQuantum);

//

REGISTER_FOR_FAST_CAST(ModuleEnergyQuantum);

static Framework::ModuleGameplay* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleEnergyQuantum(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new ModuleEnergyQuantumData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleGameplay> & ModuleEnergyQuantum::register_itself()
{
	return Framework::Modules::gameplay.register_module(String(TXT("energyQuantum")), create_module, create_module_data);
}

Concurrency::Counter g_moduleEnergyQuantumInExistence;

bool ModuleEnergyQuantum::does_any_exist()
{
	return g_moduleEnergyQuantumInExistence.get();
}

ModuleEnergyQuantum::ModuleEnergyQuantum(Framework::IModulesOwner* _owner)
: base(_owner)
{
	g_moduleEnergyQuantumInExistence.increase();
}

ModuleEnergyQuantum::~ModuleEnergyQuantum()
{
	g_moduleEnergyQuantumInExistence.decrease();
}

void ModuleEnergyQuantum::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	energyQuantumData = fast_cast<ModuleEnergyQuantumData>(_moduleData);
	if (_moduleData)
	{
		timeLeft = _moduleData->get_parameter<float>(this, NAME(timeAlive), timeLeft);
		noTimeLimit = _moduleData->get_parameter<bool>(this, NAME(noTimeLimit), noTimeLimit);
		type = EnergyType::parse(_moduleData->get_parameter<String>(this, NAME(energyType)), type);
		quantumSize = _moduleData->get_parameter<float>(this, NAME(quantumSize), quantumSize);
		if (quantumSize <= 0.0f)
		{
			error(TXT("invalid quantumSize"));
		}
#ifdef AN_DEVELOPMENT
		// allowed for development (to test features in test level)
		energy.set(Energy(_moduleData->get_parameter<float>(this, NAME(energy), energy.get(Energy::zero()).as_float())));
		if (energy.get().is_zero())
		{
			if (Game::get_as<::Framework::PreviewGame>())
			{
				energy = Energy(50.0f);
			}
			else
			{
				energy.clear();
			}
		}
#endif

		explodeOnCollisionWithFlags.apply(_moduleData->get_parameter<String>(this, NAME(explodeOnCollisionWithFlags)));
		sideEffectDamage.parse_from(_moduleData->get_parameter<String>(this, NAME(sideEffectDamage)), sideEffectDamage);
		sideEffectAIMessage = _moduleData->get_parameter<Name>(this, NAME(sideEffectAIMessage), sideEffectAIMessage);
		explodeOnDeath = _moduleData->get_parameter<bool>(this, NAME(explodeOnDeath), explodeOnDeath);
	}
}

void ModuleEnergyQuantum::reset()
{
	base::reset();
}

void ModuleEnergyQuantum::initialise()
{
	base::initialise();
}

void ModuleEnergyQuantum::set_side_effect_damage(Energy const & _sideEffectDamage)
{
	an_assert(processingLock.is_acquired_write_on_this_thread());
	sideEffectDamage = _sideEffectDamage;
}

void ModuleEnergyQuantum::set_energy(Energy const & _energy)
{
	an_assert(processingLock.is_acquired_write_on_this_thread());
	energy = _energy;
}

void ModuleEnergyQuantum::clear_energy()
{
	an_assert(processingLock.is_acquired_write_on_this_thread());
	energy.clear();
}

Energy ModuleEnergyQuantum::get_energy() const
{
	bool acquireRead = !processingLock.is_acquired_write_on_this_thread();
	if (acquireRead)
	{
		processingLock.acquire_read();
	}
	Energy result = energy.get(Energy(0));
	if (acquireRead) 
	{ 
		processingLock.release_read();
	}
	return result;
}

void ModuleEnergyQuantum::use_energy(Energy const & _usedEnergy)
{
	an_assert(processingLock.is_acquired_write_on_this_thread());
	if (_usedEnergy >= get_energy())
	{
		clear_energy();
	}
	else
	{
		set_energy(get_energy() - _usedEnergy);
	}
}

void ModuleEnergyQuantum::process_energy_quantum(EnergyQuantumContext const & _eqc, bool _clearEnergy)
{
	start_processing_energy_quantum();

	EnergyType::Type asType = _eqc.energyTypeOverride.get(type);

	if (!energy.is_set())
	{
		energy = Energy::zero();
	}
	EnergyCoef energyInhaleCoef = EnergyCoef::zero();
	if (_eqc.withHand.is_set())
	{
		energyInhaleCoef = PilgrimBlackboard::get_energy_inhale_coef(_eqc.adjustBy, _eqc.withHand.get());
	}
	if (!energyInhaleCoef.is_zero() && energy.is_set())
	{
		energy.access() = energy.get().adjusted_plus_one(energyInhaleCoef);
	}

	if (asType == EnergyType::Ammo && _eqc.ammoReceiver)
	{
		// shouldn't be equipment and hand at the same time
		if (auto* eq = _eqc.ammoReceiver->get_gameplay_as<ModuleEquipment>())
		{
			eq->process_energy_quantum_ammo(this);
		}
		if (auto* ph = _eqc.ammoReceiver->get_gameplay_as<ModulePilgrimHand>())
		{
			ph->process_energy_quantum_ammo(this);
		}
	}
	else if (asType == EnergyType::Health && _eqc.healthReceiver)
	{
		// shouldn't be equipment or health and hand at the same time
		// allow module equipment to use health energy first, if there will be anything left, health takes it
		if (auto* eq = _eqc.healthReceiver->get_gameplay_as<ModuleEquipment>())
		{
			eq->process_energy_quantum_health(this);
		}
		if (auto* h = _eqc.healthReceiver->get_custom<CustomModules::Health>())
		{
			h->process_energy_quantum_health(this);
		}
		if (auto* ph = _eqc.healthReceiver->get_gameplay_as<ModulePilgrimHand>())
		{
			ph->process_energy_quantum_health(this);
		}
	}

	if (!energyInhaleCoef.is_zero() && energy.is_set()) // if cleared, we no longer care
	{
		energy.access() = energy.get().adjusted_plus_one_inv(energyInhaleCoef);
	}

	apply_side_effects_to(_eqc.sideEffectsReceiver, _eqc.sideEffectsHealthReceiver? _eqc.sideEffectsHealthReceiver : _eqc.healthReceiver);

	if (_clearEnergy)
	{
		clear_energy();
	}

	end_processing_energy_quantum();
}

void ModuleEnergyQuantum::apply_side_effects_to(Framework::IModulesOwner* _toIMO, Framework::IModulesOwner* _healthReceiver)
{
	an_assert(processingLock.is_acquired_write_on_this_thread());

	if (sideEffectAIMessage.is_valid() && _toIMO)
	{
		if (auto* world = get_owner()->get_in_world())
		{
			if (Framework::AI::Message* message = world->create_ai_message(sideEffectAIMessage))
			{
				message->to_ai_object(_toIMO);
			}
		}
	}

	if (!sideEffectDamage.is_zero() && _healthReceiver)
	{
		if (auto* h = _healthReceiver->get_custom<CustomModules::Health>())
		{
			Damage damage;
			damage.damage = sideEffectDamage;
			damage.damageType = DamageType::EnergyQuantumSideEffects;
			DamageInfo damageInfo;
			damageInfo.damager = get_owner();
			damageInfo.source = get_owner();
			damageInfo.instigator = get_owner();

			ContinuousDamage dealContinuousDamage;
			dealContinuousDamage.setup_using_weapon_core_modifier_companion_for(damage);

			h->adjust_damage_on_hit_with_extra_effects(REF_ damage);

			h->deal_damage(damage, damage, damageInfo);
			if (dealContinuousDamage.is_valid())
			{
				h->add_continuous_damage(dealContinuousDamage, damageInfo);
			}
		}
	}
}

void ModuleEnergyQuantum::advance_post_move(float _deltaTime)
{
	base::advance_post_move(_deltaTime);

	bool disappear = false;
	if (!energy.is_set() || energy.get() == Energy(0))
	{
		disappear = true;
	}

	if (!disappear && !explodeOnCollisionWithFlags.is_empty())
	{
		if (auto* mc = get_owner()->get_collision())
		{
			bool collided = false;
			for_every(collidedWith, mc->get_collided_with())
			{
				if (auto* imo = fast_cast<Framework::IModulesOwner>(collidedWith->collidedWithObject.get()))
				{
					if (auto* imomc = imo->get_collision())
					{
						if (Collision::Flags::check(imomc->get_collision_flags(), explodeOnCollisionWithFlags))
						{
							collided = true;
							break;
						}
					}
				}
			}
			if (collided)
			{
				explode();
			}
		}
	}

	if (GameSettings::get().difficulty.energyQuantumsStayTime == 0.0f || ! get_owner()->get_presence()->get_in_room() || noTimeLimit)
	{
		timeLeft = max(0.0f, timeLeft);
	}
	else
	{
		timeLeft -= _deltaTime / GameSettings::get().difficulty.energyQuantumsStayTime;
	}
	if (timeLeft < 0.0f || disappear || markedToDisappear)
	{
		lifeScale -= (disappear ? 5.0f : 1.0f) * _deltaTime;
	}
	if (energy.is_set() &&
		energyQuantumData && ! energyQuantumData->scaling.is_empty() &&
		quantumSize != 0.0f)
	{
		float energyToScale = energy.get().as_float();
		float size = 0.0f;
		if (energyQuantumData->scaling.get_size() > 1)
		{
			ModuleEnergyQuantumData::ScalePair const * prev = energyQuantumData->scaling.begin();
			ModuleEnergyQuantumData::ScalePair const * next = prev + 1;
			if (energyToScale <= energyQuantumData->scaling.get_first().energy)
			{
				size = energyQuantumData->scaling.get_first().size;
			}
			else if (energyToScale >= energyQuantumData->scaling.get_last().energy)
			{
				size = energyQuantumData->scaling.get_last().size;
			}
			else
			{
				for_count(int, i, energyQuantumData->scaling.get_size() - 1)
				{
					if (energyToScale >= prev->energy &&
						energyToScale <= next->energy &&
						prev->energy != next->energy)
					{
						float t = (energyToScale - prev->energy) / (next->energy - prev->energy);
						size = prev->size + t * (next->size - prev->size);
						break;
					}
					++prev;
					++next;
				}
			}
		}
		else
		{
			size = energyQuantumData->scaling.get_first().size;
		}
		energyScale = size / quantumSize;
	}
	float targetFinalScale = lifeScale * energyScale;
	finalScale = finalScale.get(targetFinalScale); // if not set, snap
	finalScale = blend_to_using_time(finalScale.get(), targetFinalScale, 0.1f, _deltaTime);
	get_owner()->get_presence()->set_scale(finalScale.get());
	if (finalScale.get() <= 0.0f)
	{
		get_owner()->cease_to_exist(true);
	}
}

bool ModuleEnergyQuantum::on_death(Damage const& _damage, DamageInfo const & _damageInfo)
{
	if (!_damageInfo.peacefulDamage && explodeOnDeath)
	{
		explode(_damageInfo.instigator.get());
	}
	return false;
}
	
void ModuleEnergyQuantum::explode(Framework::IModulesOwner* _instigator)
{
	if (energy.is_set() && energy.get().is_positive())
	{
		if (auto * tos = get_owner()->get_temporary_objects())
		{
			if (!_instigator)
			{
				_instigator = get_owner()->get_valid_top_instigator();
			}
			Array<SafePtr<Framework::IModulesOwner>> spawned;
			tos->spawn_all(NAME(explode), NP, &spawned);
			if (spawned.is_empty())
			{
				warn(TXT("\"%S\" doesn't have a way to explode"), get_owner()->ai_get_name().to_char());
			}
			for_every_ref(s, spawned)
			{
				s->set_creator(get_owner());
				s->set_instigator(_instigator);
			}
		}
	}
	energy.clear();
}

void ModuleEnergyQuantum::mark_pulled_by_pilgrim(float _distance)
{
	++pulledByPilgrim;
	maxDistanceToPullByPilgrim = max(maxDistanceToPullByPilgrim, _distance);
}

//

REGISTER_FOR_FAST_CAST(ModuleEnergyQuantumData);

ModuleEnergyQuantumData::ModuleEnergyQuantumData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool ModuleEnergyQuantumData::read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("scale"))
	{
		bool result = true;
		for_every(node, _node->children_named(TXT("scale")))
		{
			ScalePair sp;
			sp.energy = node->get_float_attribute(TXT("energy"), sp.energy);
			sp.size = node->get_float_attribute(TXT("size"), sp.size);
			error_loading_xml_on_assert(sp.is_valid(), result, node, TXT("scale pair invalid"));
			scaling.push_back(sp);
		}
		sort(scaling);
		return result;
	}
	return base::read_parameter_from(_node, _lc);
}
