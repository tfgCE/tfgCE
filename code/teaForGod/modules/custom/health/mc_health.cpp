#include "mc_health.h"

#include "..\mc_emissiveControl.h"
#include "..\mc_lootProvider.h"
#include "..\mc_hitIndicator.h"

#include "..\..\moduleSound.h"

#include "..\..\gameplay\moduleEnergyQuantum.h"
#include "..\..\gameplay\moduleEquipment.h"
#include "..\..\gameplay\moduleExplosion.h"
#include "..\..\gameplay\modulePilgrim.h"
#include "..\..\gameplay\moduleProjectile.h"

#include "..\..\..\ai\aiCommon.h"
#include "..\..\..\game\damage.h"
#include "..\..\..\game\game.h"
#include "..\..\..\game\gameConfig.h"
#include "..\..\..\game\gameDirector.h"
#include "..\..\..\game\gameLog.h"
#include "..\..\..\game\gameSettings.h"
#include "..\..\..\music\musicPlayer.h"
#include "..\..\..\pilgrim\pilgrimBlackboard.h"

#include "..\..\..\..\core\mainSettings.h"
#include "..\..\..\..\core\profilePerformanceSettings.h"
#include "..\..\..\..\core\physicalSensations\physicalSensations.h"
#include "..\..\..\..\core\random\randomUtils.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\ai\aiSocial.h"
#include "..\..\..\..\framework\appearance\controllers\ac_particlesUtils.h"
#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\..\framework\game\gameUtils.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\module\modules.h"
#include "..\..\..\..\framework\world\subWorld.h"
#include "..\..\..\..\framework\world\world.h"
#include "..\..\..\..\framework\world\worldAddress.h"

#ifdef AN_DEVELOPMENT
#include "..\..\..\..\framework\ai\aiLogic.h"
#endif

// !@#
#include "..\..\..\library\library.h"
#include "..\..\..\..\framework\object\temporaryObject.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT
#define AN_HEALTH_VERBOSE
#define INSPECT_DAMAGE_RULE_EXPLOSION_DAMAGE
#endif

COMMENT_IF_NO_LONGER_REQUIRED_
#define AN_HEALTH_IMPORTANT_INFOS

#ifdef DEBUG_WORLD_LIFETIME
	#ifndef AN_HEALTH_IMPORTANT_INFOS
		#define AN_HEALTH_IMPORTANT_INFOS
	#endif
#endif

//

#define TIME_BASED_INTENSITY

//

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// params
DEFINE_STATIC_NAME(invincible);
DEFINE_STATIC_NAME(immortal);
DEFINE_STATIC_NAME(performDeathAIMessage);
DEFINE_STATIC_NAME(triggerGameScriptTrapOnDeath);
DEFINE_STATIC_NAME(triggerGameScriptTrapOnHealthReachesBottom);
DEFINE_STATIC_NAME(particlesOnDeath);
DEFINE_STATIC_NAME(affectVelocityOnDeath);
DEFINE_STATIC_NAME(allowHitFromSameInstigator);
DEFINE_STATIC_NAME(noRewardForKill);
DEFINE_STATIC_NAME(noEnergyTransfer);
DEFINE_STATIC_NAME(canBeVampirisedWithMelee);
DEFINE_STATIC_NAME(health);
DEFINE_STATIC_NAME(maxHealth);
DEFINE_STATIC_NAME(startingHealth);
DEFINE_STATIC_NAME(affectVelocityLinearLimit);
DEFINE_STATIC_NAME(affectVelocityLinearAffectLimit);
DEFINE_STATIC_NAME(affectVelocityLinearPerDamagePoint);
DEFINE_STATIC_NAME(forcedAffectVelocityLinearLimit);
DEFINE_STATIC_NAME(affectVelocityRotationLimit);
DEFINE_STATIC_NAME(affectVelocityRotationLimitYaw);
DEFINE_STATIC_NAME(affectVelocityRotationLimitPitch);
DEFINE_STATIC_NAME(affectVelocityRotationLimitRoll);
DEFINE_STATIC_NAME(affectVelocityRotationPerDamagePoint);
DEFINE_STATIC_NAME(affectVelocityRotationPerDamagePointYaw);
DEFINE_STATIC_NAME(affectVelocityRotationPerDamagePointPitch);
DEFINE_STATIC_NAME(affectVelocityRotationPerDamagePointRoll);

DEFINE_STATIC_NAME(delayDeath);

DEFINE_STATIC_NAME(restoreEmissive);
DEFINE_STATIC_NAME(healthDealDamage);

DEFINE_STATIC_NAME(storeExistenceInPilgrimage);
DEFINE_STATIC_NAME(storeExistenceInPilgrimageWorldAddress);
DEFINE_STATIC_NAME(storeExistenceInPilgrimagePOIIdx);

// ai message name
DEFINE_STATIC_NAME(dealtDamage);
DEFINE_STATIC_NAME(death);
DEFINE_STATIC_NAME(someoneElseDied);
DEFINE_STATIC_NAME_STR(aimConfussion, TXT("confussion"));

// ai message params
DEFINE_STATIC_NAME(damage);
DEFINE_STATIC_NAME(damageUnadjusted);
DEFINE_STATIC_NAME(damageDamager);
DEFINE_STATIC_NAME(damageDamagerProjectile);
DEFINE_STATIC_NAME(damageSource);
DEFINE_STATIC_NAME(damageInstigator);
DEFINE_STATIC_NAME(hitIndicatorRelativeLocation);
DEFINE_STATIC_NAME(hitIndicatorRelativeDir);
DEFINE_STATIC_NAME(confussionDuration);

DEFINE_STATIC_NAME(killer);
DEFINE_STATIC_NAME(killed);

// sounds
DEFINE_STATIC_NAME(revival);

// sound tags
DEFINE_STATIC_NAME(alive);

// timers
DEFINE_STATIC_NAME(invincibleOff);
DEFINE_STATIC_NAME(autoDeath);

// variables
DEFINE_STATIC_NAME(dead);
DEFINE_STATIC_NAME(aliveAutoControllersOff);
DEFINE_STATIC_NAME(intensity);

// game log
DEFINE_STATIC_NAME(died);

// colours
DEFINE_STATIC_NAME(corrosion);

// shader param
DEFINE_STATIC_NAME(decompose);
DEFINE_STATIC_NAME(decomposeLit);

// bones
DEFINE_STATIC_NAME(head);

// variables
DEFINE_STATIC_NAME(voiceless);

// exm params
DEFINE_STATIC_NAME(costEnergyForOneHealth);
DEFINE_STATIC_NAME(fillHealth);
DEFINE_STATIC_NAME(invulnerabilityTime);

//

REGISTER_FOR_FAST_CAST(Health);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new Health(_owner);
}

Framework::ModuleData* HealthData::create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new HealthData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleCustom> & Health::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("health")), create_module, HealthData::create_module_data);
}

Health::Health(Framework::IModulesOwner* _owner)
: base( _owner )
{
}

Health::~Health()
{
}

bool Health::does_exists_and_is_alive(Framework::IModulesOwner* imo)
{
	if (imo)
	{
		if (auto* h = imo->get_custom<Health>())
		{
			return h->is_alive();
		}
		else
		{
			return true; // always alive if no health module
		}
	}
	return false;
}

Energy Health::read_max_health_from(Framework::ModuleData const* _moduleData)
{
	Energy maxHealth = Energy::zero();
	maxHealth = Energy::get_from_module_data(nullptr, _moduleData, NAME(health), maxHealth);
	maxHealth = Energy::get_from_module_data(nullptr, _moduleData, NAME(maxHealth), maxHealth);
	return GameConfig::apply_health_coef(maxHealth, _moduleData);
}

void Health::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	healthData = fast_cast<HealthData>(_moduleData);
	an_assert(healthData);
	if (_moduleData)
	{
		invincible = _moduleData->get_parameter<bool>(this, NAME(invincible), invincible);
		{
			if (_moduleData->get_parameter<bool>(this, NAME(immortal), false))
			{
				immortal = true;
			}
			if (! _moduleData->get_parameter<bool>(this, NAME(immortal), true))
			{
				immortal = false;
			}
		}
		decompose = _moduleData->get_parameter<bool>(this, NAME(decompose), decompose);
		delayDeath = _moduleData->get_parameter<float>(this, NAME(delayDeath), delayDeath);
		performDeathAIMessage = _moduleData->get_parameter<Name>(this, NAME(performDeathAIMessage), performDeathAIMessage);
		triggerGameScriptTrapOnDeath = _moduleData->get_parameter<Name>(this, NAME(triggerGameScriptTrapOnDeath), triggerGameScriptTrapOnDeath);
		triggerGameScriptTrapOnHealthReachesBottom = _moduleData->get_parameter<Name>(this, NAME(triggerGameScriptTrapOnHealthReachesBottom), triggerGameScriptTrapOnHealthReachesBottom);
		particlesOnDeath = _moduleData->get_parameter<bool>(this, NAME(particlesOnDeath), particlesOnDeath);
		affectVelocityOnDeath = _moduleData->get_parameter<bool>(this, NAME(affectVelocityOnDeath), affectVelocityOnDeath);
		allowHitFromSameInstigator = _moduleData->get_parameter<bool>(this, NAME(allowHitFromSameInstigator), allowHitFromSameInstigator);
		noRewardForKill = _moduleData->get_parameter<bool>(this, NAME(noRewardForKill), noRewardForKill);
		noEnergyTransfer = _moduleData->get_parameter<bool>(this, NAME(noEnergyTransfer), noEnergyTransfer);
		canBeVampirisedWithMelee = _moduleData->get_parameter<bool>(this, NAME(canBeVampirisedWithMelee), canBeVampirisedWithMelee);
		{
			maxHealth = GameConfig::apply_health_coef_inv(maxHealth, _moduleData);
			health = GameConfig::apply_health_coef_inv(health, _moduleData);
			startingHealth = GameConfig::apply_health_coef_inv(startingHealth, _moduleData);
			//
			health = Energy::get_from_module_data(this, _moduleData, NAME(health), health);
			maxHealth = Energy::get_from_module_data(this, _moduleData, NAME(health), maxHealth);
			maxHealth = Energy::get_from_module_data(this, _moduleData, NAME(maxHealth), maxHealth);
			if (health == maxHealth)
			{
				health = Energy::zero();
			}
			else if (!health.is_negative())
			{
				startingHealth = health;
			}
			startingHealth = Energy::get_from_module_data(this, _moduleData, NAME(startingHealth), startingHealth);
			//
			maxHealth = GameConfig::apply_health_coef(maxHealth, _moduleData);
			health = GameConfig::apply_health_coef(health, _moduleData);
			startingHealth = GameConfig::apply_health_coef(startingHealth, _moduleData);
		}
		affectVelocityLinearLimit = _moduleData->get_parameter<float>(this, NAME(affectVelocityLinearLimit), affectVelocityLinearLimit);
		affectVelocityLinearAffectLimit = _moduleData->get_parameter<float>(this, NAME(affectVelocityLinearAffectLimit), affectVelocityLinearAffectLimit);
		affectVelocityLinearPerDamagePoint = _moduleData->get_parameter<float>(this, NAME(affectVelocityLinearPerDamagePoint), affectVelocityLinearPerDamagePoint);
		forcedAffectVelocityLinearLimit = _moduleData->get_parameter<float>(this, NAME(forcedAffectVelocityLinearLimit), forcedAffectVelocityLinearLimit);
		{
			affectVelocityRotationLimit.yaw = _moduleData->get_parameter<float>(this, NAME(affectVelocityRotationLimit), affectVelocityRotationLimit.yaw);
			affectVelocityRotationLimit.pitch = _moduleData->get_parameter<float>(this, NAME(affectVelocityRotationLimit), affectVelocityRotationLimit.pitch);
			affectVelocityRotationLimit.roll = _moduleData->get_parameter<float>(this, NAME(affectVelocityRotationLimit), affectVelocityRotationLimit.roll);
			affectVelocityRotationLimit.yaw = _moduleData->get_parameter<float>(this, NAME(affectVelocityRotationLimitYaw), affectVelocityRotationLimit.yaw);
			affectVelocityRotationLimit.pitch = _moduleData->get_parameter<float>(this, NAME(affectVelocityRotationLimitPitch), affectVelocityRotationLimit.pitch);
			affectVelocityRotationLimit.roll = _moduleData->get_parameter<float>(this, NAME(affectVelocityRotationLimitRoll), affectVelocityRotationLimit.roll);
		}
		{
			affectVelocityRotationPerDamagePoint.yaw = _moduleData->get_parameter<float>(this, NAME(affectVelocityRotationPerDamagePoint), affectVelocityRotationPerDamagePoint.yaw);
			affectVelocityRotationPerDamagePoint.pitch = _moduleData->get_parameter<float>(this, NAME(affectVelocityRotationPerDamagePoint), affectVelocityRotationPerDamagePoint.pitch);
			affectVelocityRotationPerDamagePoint.roll = _moduleData->get_parameter<float>(this, NAME(affectVelocityRotationPerDamagePoint), affectVelocityRotationPerDamagePoint.roll);
			affectVelocityRotationPerDamagePoint.yaw = _moduleData->get_parameter<float>(this, NAME(affectVelocityRotationPerDamagePointYaw), affectVelocityRotationPerDamagePoint.yaw);
			affectVelocityRotationPerDamagePoint.pitch = _moduleData->get_parameter<float>(this, NAME(affectVelocityRotationPerDamagePointPitch), affectVelocityRotationPerDamagePoint.pitch);
			affectVelocityRotationPerDamagePoint.roll = _moduleData->get_parameter<float>(this, NAME(affectVelocityRotationPerDamagePointRoll), affectVelocityRotationPerDamagePoint.roll);
		}
	}
	if (healthData)
	{
		ruleInstances.set_size(0);
		ruleInstances.make_space_for(healthData->rules.get_size());
		for_every(r, healthData->rules)
		{
			RuleInstance ri;
			ri.health = GameConfig::apply_health_coef(r->health, _moduleData);
			ruleInstances.push_back(ri);
		}
	}
}

void Health::update_super_health_storage()
{
	superHealthStorage = false;
	if (auto* mp = get_owner()->get_gameplay_as<ModulePilgrim>())
	{
		superHealthStorage = mp->has_exm_equipped(EXMID::Passive::super_health_storage());
	}

	Energy excessEnergy = Energy::zero();
	redistribute_energy(REF_ excessEnergy);
}

Energy Health::get_max_health_internal() const
{
	return maxHealth + PilgrimBlackboard::get_health_modifier_for(get_owner());
}

void Health::on_brought_to_life()
{
	isDying = false;
	isPerformingDeath = false;
	rewardForKillGiven = false;
	peacefulDeath = false;
	lootableDeath = false; // has to be explicitly set
	wasDamagedByPlayer = false;

	clear_continuous_damage();
	clear_extra_effects();

	rewardForKillGiven = false;
	lastDamageInstigator.clear();
	deathInstigator.clear();
	deathDamageType.clear();
}

void Health::reset()
{
	base::reset();

	on_brought_to_life();

	reset_health();
}

void Health::initialise()
{
	base::initialise();

	reset_health();
}

void Health::reset_health(Optional<Energy> const& _to, Optional<bool> const& _clearEffects)
{
	if (_to.is_set() && !_to.get().is_positive())
	{
		// no reset
		return;
	}
	if (!health.is_positive())
	{
		on_brought_to_life();
	}
	health = _to.get(max(startingHealth, Energy::zero()));
	if (health.is_zero())
	{
		health = get_max_health();
	}
	health = min(health, get_max_health());
	lastPreDamageHealth = health;
	if (_clearEffects.get(true))
	{
		clear_continuous_damage();
		clear_extra_effects();
	}
}

void Health::add_energy(REF_ Energy& _addEnergy)
{
	if (!health.is_positive() && _addEnergy.is_positive())
	{
		on_brought_to_life();
	}
	Energy currentMaxHealth = get_max_health();
	Energy addNow = min(_addEnergy, currentMaxHealth - health);
	health += addNow;
	_addEnergy -= addNow;
}

void Health::redistribute_energy(REF_ Energy& _excessEnergy)
{
	Energy currentMaxHealth = get_max_health();
	if (health > currentMaxHealth)
	{
		_excessEnergy += health - currentMaxHealth;
		health = currentMaxHealth;
	}
	if (health < currentMaxHealth &&
		_excessEnergy.is_positive())
	{
		Energy use = min(_excessEnergy, currentMaxHealth - health);
		health += use;
		_excessEnergy -= use;
	}
}

void Health::be_invincible(Optional<float> _forTime)
{
	get_owner()->clear_timer(NAME(invincibleOff));
	invincible = true;
	if (invincible && _forTime.is_set())
	{
		get_owner()->set_timer(max(0.001f, _forTime.get()), NAME(invincibleOff), [](Framework::IModulesOwner* _imo)
		{
			if (auto* h = _imo->get_custom<Health>())
			{
				h->be_no_longer_invincible();
			}
		});
	}
}

void Health::be_no_longer_invincible()
{
	get_owner()->clear_timer(NAME(invincibleOff));
	invincible = false;
}

void Health::process_energy_quantum_health(ModuleEnergyQuantum* _eq)
{
	an_assert(_eq->is_being_processed_by_us());

	if (_eq->has_energy())
	{
		MODULE_OWNER_LOCK(TXT("Health::process_energy_quantum_health"));

		Energy addHealth = min(_eq->get_energy(), get_max_health() - health);
		health += addHealth;

		_eq->use_energy(addHealth);
	}
}

void Health::set_health(Energy const & _health)
{
	MODULE_OWNER_LOCK(TXT("Health::set_health"));
	health = _health;
}

void Health::spawn_particles_on_decompose()
{
#ifndef NO_TEMPORARY_OBJECTS_ON_DEATH
	auto* imo = get_owner();

	auto* deathParticlesInstigator = deathInstigator.get();
	for_every(sod, healthData->onDecompose)
	{
		if (sod->temporaryObject.is_valid())
		{
			MEASURE_AND_OUTPUT_PERFORMANCE(TXT("spawn on decompose of \"%S\" to=\"%S\""), imo->ai_get_name().to_char(), sod->temporaryObject.to_char());
			if (auto* to = imo->get_temporary_objects())
			{
				// may do damage
				Array<SafePtr<Framework::IModulesOwner>> spawned;
				to->spawn_all(sod->temporaryObject, NP, &spawned);
				for_every_ref(s, spawned)
				{
					s->set_creator(imo);
					s->set_instigator(deathParticlesInstigator);
				}
			}
		}
		else if (sod->temporaryObjectType.is_set())
		{
			if (auto* particleType = sod->temporaryObjectType.get())
			{
				MEASURE_AND_OUTPUT_PERFORMANCE(TXT("spawn on decompose of \"%S\" pt=\"%S\""), imo->ai_get_name().to_char(), particleType->get_name().to_string().to_char());
				if (auto* subWorld = imo->get_as_object()->get_in_sub_world())
				{
					if (auto* particles = subWorld->get_one_for(particleType, imo->get_presence()->get_in_room()))
					{
						particles->set_creator(imo);
						particles->set_instigator(deathParticlesInstigator);
						particles->on_activate_attach_to(imo);
						particles->on_activate_place_in_fail_safe(imo->get_presence()->get_in_room(), imo->get_presence()->get_centre_of_presence_transform_WS());
						particles->mark_to_activate();
					}
				}
			}
		}
	}
#endif
}

void Health::on_decompose()
{
	auto* imo = get_owner();

	// past light up time it should no longer be collidable, it is a cloud of bits, right?
	if (auto* c = imo->get_collision())
	{
		c->push_collision_flags(NAME(died), Collision::Flags::none());
		c->update_collidable_object();
	}

	// hide it altogether when we're fully light up
	for_every_ptr(a, imo->get_appearances())
	{
		a->be_visible(false);
	}

	// it's dead, stop talking
	if (auto* s = fast_cast<ModuleSound>(imo->get_sound()))
	{
		s->stop_talk_sounds(0.05f);
	}

	// prevent from talking
	{
		imo->access_variables().access<bool>(NAME(voiceless)) = true;
	}

	spawn_particles_on_decompose();

	// just in any case
	clear_extra_effects();

	mark_no_longer_requires(all_customs__advance_post, HealthPostMoveReason::Decomposing);

	// allow particles to be activated nicely and then get destroyed
	imo->set_timer(1 /* one frame */, [](Framework::IModulesOwner* _imo) { _imo->cease_to_exist(true); });
}

void Health::advance_post(float _deltaTime)
{
	base::advance_post(_deltaTime);

	if (!spawnedTemporaryObjects.is_empty())
	{
		for (int i = 0; i < spawnedTemporaryObjects.get_size(); ++i)
		{
			if (!spawnedTemporaryObjects[i].get())
			{
				spawnedTemporaryObjects.remove_fast_at(i);
				--i;
			}
		}
	}

	if (decomposingFor.is_set() &&
		decomposeTime.is_set())
	{
		float prevTime = decomposingFor.get();
		decomposingFor = prevTime + _deltaTime;
		float lightUpTime = decomposeTime.get();
		float appearDecompose = 0.0f;
		{
			float l = clamp(1.4f * decomposingFor.get() / lightUpTime, 0.0f, 1.0f);
			l = clamp(1.0f - l, 0.0f, 1.0f);
			l = sqr(l);
			l = 1.0f - l;
			appearDecompose = clamp(l, 0.0f, 1.0f);
		}

		if (decomposingFor.get() >= lightUpTime && prevTime < lightUpTime)
		{
			on_decompose();
		}

		float decomposeLit = PlayerPreferences::are_currently_flickering_lights_allowed() ? 1.0f : 0.0f;
		if (auto* imo = get_owner())
		{
			ArrayStatic<Framework::IModulesOwner*, 16> imos;
			imos.push_back(imo);
			for (int i = 0; i < imos.get_size(); ++i)
			{
				auto* imo = imos[i];
				for_every_ptr(a, imo->get_appearances())
				{
					a->set_shader_param(NAME(decompose), appearDecompose);
					a->set_shader_param(NAME(decomposeLit), decomposeLit);
				}
				//
				if (imo->get_presence()->has_anything_attached())
				{
					// just one layer of depth, for time being it's enough for us
					Concurrency::ScopedSpinLock lock(imo->get_presence()->access_attached_lock());
					for_every_ptr(aimo, imo->get_presence()->get_attached())
					{
						if (!aimo->get_as_temporary_object())
						{
							if (! imos.has_place_left())
							{
								if (i >= 0)
								{
									imos.remove_at(0, i + 1);
									i = -1;
								}
								else
								{
									error(TXT("no space left to update decompose!"));
									continue;
								}
							}
							imos.push_back(aimo);
						}
					}
				}
			}
		}
	}

	dampedHealth = max(health, dampedHealth - get_max_health().mul(_deltaTime));

	float globalTintColourActiveTarget = 0.0f;
	Colour globalTintColourTarget = globalTintColour;

	// continuous damage
	{
		if (continuousDamage.is_empty())
		{
			mark_no_longer_requires(all_customs__advance_post, HealthPostMoveReason::ContinuousDamage);
		}
		else
		{
			DamageType::Type strongestDamageType = DamageType::Unknown;
			Energy strongestDamage = Energy::zero();

			bool removeOne = false;
			for_every(cd, continuousDamage)
			{
				if (cd->timeLeft <= 0.0f)
				{
					removeOne = true;
					continue;
				}
				float time = min(cd->timeLeft, _deltaTime);

				Damage dealDamage = cd->damagePerSecond;
				an_assert(dealDamage.continousDamage);
				dealDamage.damage = dealDamage.damage.timed(time, cd->damageMissingBit);

				// don't adjust_damage_on_hit_with_extra_effects
				deal_damage(dealDamage, dealDamage, cd->damageInfo);

				if ((strongestDamageType == DamageType::Unknown) ||
					(dealDamage.damageType != DamageType::Unknown &&
					 cd->damagePerSecond.damage >= strongestDamage)) // get raw value, not timed, as when timing we may end up with something really small
				{
					strongestDamage = dealDamage.damage;
					strongestDamageType = dealDamage.damageType;
				}

				cd->timeLeft -= _deltaTime;

				{
					// do not clamp intensity, let the controller do the thing if it wants to
#ifdef HEALTH__CONTINUOUS_DAMAGE__TIME_BASED_INTENSITY
					cd->intensity = cd->intensityFull != 0.0f? cd->timeLeft / cd->intensityFull : 1.0f;
#else
					Energy damageLeft = cd->damagePerSecond.damage.mul(cd->timeLeft);
					cd->intensity = damageLeft.div_to_float(cd->intensityFull);
#endif
				}
			}

			if (removeOne)
			{
				for_every(cd, continuousDamage)
				{
					if (cd->timeLeft <= 0.0f)
					{
						continuousDamage.remove_at(for_everys_index(cd));
						break;
					}
				}
			}

			if (strongestDamageType != DamageType::Unknown)
			{
				// look through extra health effects to get the right tint
				for_every(ee, extraEffects)
				{
					if (ee->forContinuousDamageType.is_set() &&
						ee->forContinuousDamageType.get() == strongestDamageType &&
						ee->params.globalTintColour.is_set())
					{
						globalTintColourTarget = ee->params.globalTintColour.get();
						globalTintColourActiveTarget = 1.0f;
						break;
					}
				}
			}
		}
	}

	// extra effects
	{
		if (extraEffects.is_empty())
		{
			mark_no_longer_requires(all_customs__advance_post, HealthPostMoveReason::ExtraEffect);
		}
		else
		{
			Optional<int> removeEE;
			for_every(ee, extraEffects)
			{
				if (ee->timeLeft.is_set())
				{
					if (ee->timeLeft.get() <= 0.0f)
					{
						removeEE.set_if_not_set(for_everys_index(ee));
						continue;
					}
					float time = min(ee->timeLeft.get(), _deltaTime);

					ee->timeLeft = ee->timeLeft.get() - time;
				}
				if (ee->forContinuousDamageType.is_set())
				{
					bool removeIt = true;
					for_every(cd, continuousDamage)
					{
						if (ee->forContinuousDamageType.get() == cd->damagePerSecond.damageType)
						{
							if (auto* to = ee->particles.get())
							{
								to->access_variables().access<float>(NAME(intensity)) = cd->intensity;
							}
							removeIt = false;
							break;
						}
					}
					if (removeIt)
					{
						removeEE.set_if_not_set(for_everys_index(ee));
					}
				}
			}

			if (removeEE.is_set())
			{
				auto& ee = extraEffects[removeEE.get()];
				if (auto* to = ee.particles.get())
				{
					Framework::ParticlesUtils::desire_to_deactivate(to);
					ee.particles.clear();
				}
				extraEffects.remove_at(removeEE.get());
			}

			if (globalTintColourActiveTarget == 0.0f)
			{
				// get the first one that has colour
				for_every(ee, extraEffects)
				{
					if (ee->params.globalTintColour.is_set())
					{
						globalTintColourTarget = ee->params.globalTintColour.get();
						globalTintColourActiveTarget = 1.0f;
						break;
					}
				}
			}
		}
	}

	if (globalTintColourActive == 0.0f)
	{
		globalTintColour = globalTintColourTarget;
	}
	else
	{
		globalTintColour = blend_to_using_time(globalTintColour, globalTintColourTarget, 0.1f, _deltaTime);
	}

	globalTintColourActive = blend_to_using_speed_based_on_time(globalTintColourActive, globalTintColourActiveTarget, globalTintColourActive > globalTintColourActiveTarget? 0.7f: 0.2f, _deltaTime);
	globalTintColour.a = globalTintColourActive * (0.15f + 0.07f * sin_deg(::System::Core::get_timer_mod(0.7f) * 360.0f));
}

void Health::clear_continuous_damage()
{
	continuousDamage.clear();
}

void Health::add_continuous_damage(TeaForGodEmperor::ContinuousDamage const& _continuousDamage, DamageInfo& _info, float _percentage, bool _accumulate)
{
	MODULE_OWNER_LOCK(TXT("Health::add_continuous_damage"));

	auto* imo = get_owner();

	if (! imo || !health.is_positive())
	{
		// already dead
		return;
	}

	TeaForGodEmperor::ContinuousDamage continuousDamage = _continuousDamage;
	if (! check_and_act(continuousDamage, _info))
	{
		return;
	}
	if (! continuousDamage.is_valid())
	{
		return;
	}
	Damage damagePerSecond;
	damagePerSecond.damageType = continuousDamage.damageType;
	damagePerSecond.damage = continuousDamage.damage;
	if (!_continuousDamage.selfDamageCoef.is_one() && _info.source.get() && _info.source->get_top_instigator() == imo->get_top_instigator())
	{
		damagePerSecond.damage = damagePerSecond.damage.adjusted(_continuousDamage.selfDamageCoef);
	}
	damagePerSecond.damage = damagePerSecond.damage.mul(1.0f / continuousDamage.time);
	damagePerSecond.meleeDamage = continuousDamage.meleeDamage;
	damagePerSecond.explosionDamage = continuousDamage.explosionDamage;
	internal_add_continuous_damage(damagePerSecond, _info, continuousDamage.time, continuousDamage.minTime, _percentage, _accumulate);
}

void Health::apply_continuous_damage_instantly(DamageType::Type _ofType, Optional<Energy> const & _damage, Optional<EnergyCoef> const & _damageCoef, Framework::IModulesOwner* _instigator)
{
	MODULE_OWNER_LOCK(TXT("Health::apply_continuous_damage_instantly"));

	auto* imo = get_owner();

	if (!imo || !health.is_positive())
	{
		// already dead
		return;
	}

	for_every(cd, continuousDamage)
	{
		if (cd->damagePerSecond.damageType == _ofType)
		{
			if (cd->timeLeft > 0.0f)
			{
				Damage dealDamage = cd->damagePerSecond;
				an_assert(dealDamage.continousDamage);
				dealDamage.damage = _damage.get(Energy::zero()) + dealDamage.damage.timed(cd->timeLeft, cd->damageMissingBit).adjusted(_damageCoef.get(EnergyCoef::one()));

				DamageInfo damageInfo = cd->damageInfo;
				if (_instigator)
				{
					damageInfo.instigator = _instigator;
				}

				// don't adjust_damage_on_hit_with_extra_effects
				deal_damage(dealDamage, dealDamage, damageInfo);

				cd->timeLeft = 0.0f;
			}
		}
	}
}

void Health::internal_add_continuous_damage(Damage const& _damagePerSecond, DamageInfo& _info, float _time, float _minTime, float _percentage, bool _accumulate)
{
	// make time smaller when past a certain threshold, it will increase the DPS
	float const timeThreshold = 10.0f;
	if (_accumulate)
	{
		for_every(cd, continuousDamage)
		{
			if (cd->damagePerSecond.damageType == _damagePerSecond.damageType &&
				// do not mix per type - we only care about instigator
				cd->damageInfo.instigator == _info.instigator &&
				cd->timeLeft > 0.0f) // doesn't make sense to add to a damage that is going away
			{
				_time *= _percentage;
				// sum damage and time and calculate new DPS
				Energy damageLeft = cd->damagePerSecond.damage.mul(cd->timeLeft);
				Energy damageToAdd = _damagePerSecond.damage.mul(_time);
				Energy damageSum = damageLeft + damageToAdd;
				float timeSum = cd->timeLeft + _time;
				if (timeSum > timeThreshold)
				{
					timeSum = timeThreshold + log((timeSum - timeThreshold) + 1.0f);
				}
				timeSum = max(_minTime, timeSum);
				cd->damagePerSecond.damage = damageSum.mul(1.0f / timeSum);
				cd->timeLeft = timeSum;
				return;
			}
		}
	}

	ContinuousDamage cd;
	cd.damagePerSecond = _damagePerSecond;

	{
		_time *= _percentage;
		Energy damage = cd.damagePerSecond.damage.mul(_time);
		if (_time > timeThreshold)
		{
			_time = timeThreshold + log((_time - timeThreshold) + 1.0f);
		}
		_time = max(_minTime, _time);
		cd.damagePerSecond.damage = damage.mul(1.0f / _time);
	}

	cd.damagePerSecond.continousDamage = true;
	cd.damagePerSecond.affectVelocity = 0.0f;
	cd.damagePerSecond.affectVelocityDead = 0.0f;
	cd.damageInfo = _info;
	cd.damageInfo.hitBone.clear();
	cd.damageInfo.hitRelativeDir.clear();
	cd.damageInfo.hitRelativeLocation.clear();
	cd.damageInfo.hitRelativeNormal.clear();
	cd.timeLeft = _time;

	continuousDamage.push_back(cd);

	mark_requires(all_customs__advance_post, HealthPostMoveReason::ContinuousDamage);
}

void Health::clear_extra_effects()
{
	for_every(ee, extraEffects)
	{
		if (auto* to = ee->particles.get())
		{
			Framework::ParticlesUtils::desire_to_deactivate(to);
			ee->particles.clear();
		}
	}

	extraEffects.clear();
}

bool Health::has_extra_effect(Name const& _name) const
{
	MODULE_OWNER_LOCK(TXT("Health::has_extra_effect"));

	for_every(ee, extraEffects)
	{
		if (ee->name == _name)
		{
			return true;
		}
	}

	return false;
}

void Health::add_extra_effect(HealthExtraEffect const & _ee)
{
	MODULE_OWNER_LOCK(TXT("Health::add_extra_effect"));

	auto* imo = get_owner();

	if (!imo || !health.is_positive())
	{
		// already dead
		return;
	}

	for_every(ee, extraEffects)
	{
		if (ee->name == _ee.name)
		{
			if (_ee.time.is_set())
			{
				ee->timeLeft = max(ee->timeLeft.get(_ee.time.get()), _ee.time.get());
			}
			return;
		}
	}

	ExtraEffect ee;
	ee.name = _ee.name;
	ee.timeLeft = _ee.time;
	ee.forContinuousDamageType = _ee.forContinuousDamageType;
	ee.params = _ee; // copy just required params

	{
		Framework::TemporaryObject* particles = nullptr;
		if (!particles)
		{
			if (_ee.temporaryObject.is_valid())
			{
				if (auto* imo = get_owner())
				{
					if (auto* tom = imo->get_temporary_objects())
					{
						particles = tom->spawn(_ee.temporaryObject);
					}
				}
			}
		}
		if (!particles)
		{
			if (auto* tot = _ee.temporaryObjectType.get())
			{
				if (auto* imo = get_owner())
				{
					if (auto* subWorld = imo->get_in_sub_world())
					{
						if ((particles = subWorld->get_one_for(tot, imo->get_presence()->get_in_room())))
						{
							particles->mark_to_activate();
							particles->on_activate_attach_to(imo);
						}
					}
				}
			}
		}
		if (particles)
		{
			particles->set_creator(imo);
			//particles->set_instigator();
			ee.particles = particles;
		}
	}
	extraEffects.push_back(ee);

	mark_requires(all_customs__advance_post, HealthPostMoveReason::ExtraEffect);
}

void Health::adjust_damage_on_hit_with_extra_effects(REF_ Damage& _damage) const
{
	MODULE_OWNER_LOCK(TXT("Health::adjust_damage_on_hit_with_extra_effects"));
	for_every(ee, extraEffects)
	{
		if (ee->params.ignoreForDamageType.is_set() &&
			ee->params.ignoreForDamageType.get() == _damage.damageType)
		{
			continue;
		}
		if (ee->params.damageCoef.is_set())
		{
			Energy newDamage = _damage.damage.adjusted(ee->params.damageCoef.get());
			Energy extraDamage = newDamage - _damage.damage;
			if (ee->params.extraDamageCap.is_set())
			{
				extraDamage = min(extraDamage, ee->params.extraDamageCap.get());
			}
			_damage.damage += extraDamage;
		}
		if (ee->params.armourPiercing.is_set())
		{
			// get closer to 100%
			// 50% takes:
			//    0% to 50%
			//   50% to 75%
			//   80% to 90%
			//  100% to 100%  -- all above stay the same
			//  150% to 150%
			_damage.armourPiercing = max(_damage.armourPiercing, EnergyCoef::one() - (EnergyCoef::one() - _damage.armourPiercing).adjusted(EnergyCoef::one() - clamp(ee->params.armourPiercing.get(), EnergyCoef::zero(), EnergyCoef::one())));
		}
		if (ee->params.ricocheting.is_set())
		{
			_damage.ricocheting = ee->params.ricocheting.get();
		}
	}
}

bool Health::do_extra_effects_allow_ricochets() const
{
	MODULE_OWNER_LOCK(TXT("Health::adjust_damage_on_hit_with_extra_effects"));
	for_every(ee, extraEffects)
	{
		if (! ee->params.ricocheting.get(true))
		{
			return false;
		}
	}
	return true;
}

void Health::deal_damage(Damage const & _damage, Damage const & _unadjustedDamage, DamageInfo & _info)
{
	MODULE_OWNER_LOCK(TXT("Health::deal_damage"));

	Damage damage = _damage;
	if (check_and_act(damage, _info))
	{
		on_deal_damage(damage, _unadjustedDamage, _info);
	}
}

bool Health::check_and_act_filter(DamageRule const * damageRule, Damage const * _damage, TeaForGodEmperor::ContinuousDamage const * _continuousDamage, DamageInfo& _info) const
{
	an_assert(_damage || _continuousDamage);
	if (damageRule->forDamageType.is_set() && damageRule->forDamageType.get() != (_damage? _damage->damageType : _continuousDamage->damageType))
	{
		return false;
	}
	if (damageRule->forDamageExtraInfo.is_set() && damageRule->forDamageExtraInfo != (_damage? _damage->damageExtraInfo : _continuousDamage->damageExtraInfo))
	{
		return false;
	}

	if (_damage)
	{
		if (damageRule->forMeleeDamage.is_set() && damageRule->forMeleeDamage.get() != _damage->meleeDamage)
		{
			return false;
		}

		if (damageRule->forExplosionDamage.is_set() && damageRule->forExplosionDamage.get() != _damage->explosionDamage)
		{
			return false;
		}
	}

	if (damageRule->ifCurrentContinuousDamageType.is_set())
	{
		bool isActive = false;
		for_every(cd, continuousDamage)
		{
			if (damageRule->ifCurrentContinuousDamageType.get() == cd->damagePerSecond.damageType)
			{
				isActive = true;
				break;
			}
		}
		if (!isActive)
		{
			return false;
		}
	}

	if (damageRule->ifCurrentExtraEffect.is_set())
	{
		bool isActive = false;
		for_every(ee, extraEffects)
		{
			if (damageRule->ifCurrentExtraEffect.get() == ee->name)
			{
				isActive = true;
				break;
			}
		}
		if (!isActive)
		{
			return false;
		}
	}

	if (damageRule->forDamagerIsExplosion.is_set())
	{
		bool damagerIsExplosion = _info.damager.get() && _info.damager->get_gameplay_as<ModuleExplosion>();
		if (damageRule->forDamagerIsExplosion.get() != damagerIsExplosion)
		{
			return false;
		}
	}

	if (damageRule->forDamagerIsAlly.is_set())
	{
		bool damagerIsAlly = false;
		if (auto* damagerIMO = _info.instigator.get())
		{
			if (auto* ai = damagerIMO->get_ai())
			{
				auto const& aiSocial = ai->get_mind()->get_social();
				damagerIsAlly = aiSocial.is_ally(get_owner());
			}
			else if (auto* ai = get_owner()->get_ai())
			{
				auto const& aiSocial = ai->get_mind()->get_social();
				damagerIsAlly = aiSocial.is_ally(damagerIMO);
			}
		}
		if (damageRule->forDamagerIsAlly.get() != damagerIsAlly)
		{
			return false;
		}
	}

	if (damageRule->forDamagerIsSelf.is_set())
	{
		bool damagerIsSelf = false;
		if (auto* damagerIMO = _info.instigator.get())
		{
			if (damagerIMO == get_owner())
			{
				damagerIsSelf = true;
			}
		}
		if (damageRule->forDamagerIsSelf.get() != damagerIsSelf)
		{
			return false;
		}
	}

	return true;
}

bool Health::check_and_act(Damage& _damage, DamageInfo& _info)
{
	bool result = true;
	if (healthData)
	{
		if (_damage.explosionDamage)
		{
			EnergyCoef useExplosionResistance = explosionResistance;
			EnergyCoef pbExplosionResistance = PilgrimBlackboard::get_explosion_resistance(get_owner());
			useExplosionResistance = useExplosionResistance.adjusted_to_one(pbExplosionResistance);
			if (!useExplosionResistance.is_zero())
			{
				_damage.damage = _damage.damage.adjusted_rev_one(useExplosionResistance);
			}
		}
		for_every(damageRule, healthData->damageRules)
		{
			if (! check_and_act_filter(damageRule, &_damage, nullptr, _info))
			{
				continue;
			}
			
			// process
			if (damageRule->applyCoef.is_set())
			{
				_damage.damage = _damage.damage.adjusted(damageRule->applyCoef.get());
			}
			//
			if (damageRule->noExplosion && _damage.explosionDamage)
			{
				result = false;
			}
			//
			if (_damage.continousDamage)
			{
				if (damageRule->noContinuousDamage)
				{
					result = false;
				}
				// addToProjectileDamage should be handled earlier
			}
			else
			{
				if (damageRule->spawnTemporaryObject.is_valid() ||
					damageRule->spawnTemporaryObjectType.get())
				{
					bool okToSpawn = true;
					if (damageRule->spawnTemporaryObjectCooldown.is_set() &&
						damageRule->spawnTemporaryObjectCooldownId.is_valid())
					{
						if (spawnTemporaryObjectOnDamageRuleCooldowns.is_available(damageRule->spawnTemporaryObjectCooldownId))
						{
							spawnTemporaryObjectOnDamageRuleCooldowns.set_if_longer(damageRule->spawnTemporaryObjectCooldownId, damageRule->spawnTemporaryObjectCooldown.get());
						}
						else
						{
							okToSpawn = false;
						}
					}
					if (okToSpawn)
					{
						auto* instigator = _info.source->get_top_instigator();
						Array<SafePtr<Framework::IModulesOwner>> spawned;
						if (damageRule->spawnTemporaryObject.is_valid())
						{
							if (auto* tos = get_owner()->get_temporary_objects())
							{
								tos->spawn_all(damageRule->spawnTemporaryObject, NP, &spawned);
							}
						}
						if (auto* tot = damageRule->spawnTemporaryObjectType.get())
						{
							if (auto* room = get_owner()->get_presence()->get_in_room())
							{
								if (auto* subWorld = room->get_in_sub_world())
								{
									if (auto* to = subWorld->get_one_for(tot, room))
									{
										Framework::TemporaryObjectSpawnContext spawnContext;
										spawnContext.inRoom = room;
										spawnContext.placement = get_owner()->get_presence()->get_centre_of_presence_transform_WS();
										if (damageRule->spawnTemporaryObjectAtDamager)
										{
											if (auto* imo = _info.damager.get())
											{
												spawnContext.inRoom = imo->get_presence()->get_in_room();
												spawnContext.placement = imo->get_presence()->get_centre_of_presence_transform_WS();
											}
										}

										to->on_activate_place_in(spawnContext.inRoom.get(), spawnContext.placement.get());
										to->mark_to_activate();
										spawned.push_back(SafePtr<Framework::IModulesOwner>(to));

										Framework::ModuleTemporaryObjects::spawn_companions_of(get_owner(), to, &spawned, spawnContext);
									}
								}
							}
						}
						if (spawned.is_empty())
						{
							warn(TXT("\"%S\" doesn't have a way to explode"), get_owner()->ai_get_name().to_char());
						}
						for_every_ref(s, spawned)
						{
							spawnedTemporaryObjects.push_back(SafePtr<Framework::IModulesOwner>(s));
							s->set_creator(get_owner());
							s->set_instigator(instigator);

							if (auto* to = fast_cast<Framework::TemporaryObject>(s))
							{
								if (auto* e = to->get_gameplay_as<ModuleExplosion>())
								{
									Energy providedDamage = _damage.damage.adjusted(damageRule->temporaryObjectDamageCoef.get(EnergyCoef::one()));
									Energy providedContinuousDamage = Energy::zero();

									to->perform_on_activate([providedDamage, providedContinuousDamage](Framework::TemporaryObject* to)
										{
											if (auto* e = to->get_gameplay_as<ModuleExplosion>())
											{
												Energy useDamage = providedDamage;
												Energy useContinuousDamage = providedContinuousDamage;

												// now let's redistribute our total damage according to how it is distributed in explosion we use - maintain proportion
												Energy orgDamage = e->access_damage().damage;
												Energy orgContinuousDamage = e->access_continuous_damage().is_valid() ? e->access_continuous_damage().damage : Energy::zero();

												// keep continuous damage as is, we have weapon modifiers working hard on keeping those
#ifdef INSPECT_DAMAGE_RULE_EXPLOSION_DAMAGE
												output(TXT("[health] explode redistribute damage: %S"), useDamage.as_string().to_char(), useContinuousDamage.as_string().to_char());
												output(TXT("[health] to fit: %S + %S"), orgDamage.as_string().to_char(), orgContinuousDamage.as_string().to_char());
#endif

												if (orgContinuousDamage.is_zero())
												{
													useDamage = useDamage + useContinuousDamage;
													useContinuousDamage = Energy::zero();
												}
												else if (orgDamage.is_zero())
												{
													useContinuousDamage = useDamage + useContinuousDamage;
													useDamage = Energy::zero();
												}
												else
												{
													float proportion = orgDamage.div_to_float(orgDamage + orgContinuousDamage);
													Energy newDamageSum = useDamage + useContinuousDamage;
													useDamage = newDamageSum.mul(proportion);
													useContinuousDamage = newDamageSum - useDamage;
#ifdef INSPECT_DAMAGE_RULE_EXPLOSION_DAMAGE
													output(TXT("[health] using proportion %.2f%%"), proportion * 100.0f);
#endif
												}
#ifdef INSPECT_DAMAGE_RULE_EXPLOSION_DAMAGE
												output(TXT("[health] explode damage (redistributed): %S + %S"), useDamage.as_string().to_char(), useContinuousDamage.as_string().to_char());
#endif

												// and store new values
												e->access_damage().damage = useDamage;
												e->access_continuous_damage().damage = useContinuousDamage;
											}

										});
								}
							}
						}
					}
				}
				if (damageRule->endContinuousDamageType.is_set())
				{
					for_every(cd, continuousDamage)
					{
						if (damageRule->endContinuousDamageType.get() == cd->damagePerSecond.damageType)
						{
							cd->timeLeft = 0.0f; // will end it
						}
					}
				}
				for_every(ee, damageRule->extraEffects)
				{
					add_extra_effect(*ee);
				}
				if (damageRule->endExtraEffect.is_set())
				{
					for_every(ee, extraEffects)
					{
						if (damageRule->endExtraEffect.get() == ee->name)
						{
							// will end it
							ee->timeLeft = 0.0f;
							ee->forContinuousDamageType.clear();
						}
					}
				}
				if (damageRule->kill)
				{
					// make sure it will get killed
					_damage.instantDeath = true;
				}
				if (damageRule->heal)
				{
					// make sure it is healing
					_damage.damage = -_damage.cost.get(_damage.damage);
				}
				if (damageRule->addToProjectileDamage)
				{
					if (auto* p = get_owner()->get_gameplay_as<ModuleProjectile>())
					{
						auto& pd = p->access_damage();
						if (pd.damageType == _damage.damageType)
						{
							pd.damage += _damage.damage;
							// and consume
							_damage.damage = Energy::zero();

							// update its look
							p->update_appearance_variables(ModuleProjectile::UAV_Damage);
						}
						else
						{
							warn(TXT("different damage type (for damage), can't add to projectile from health's receiving damage"));
						}
					}
				}
				if (damageRule->noDamage)
				{
					result = false;
				}
			}

			if (damageRule->stopProcessing)
			{
				break;
			}
		}
	}

	return result;
}

bool Health::check_and_act(TeaForGodEmperor::ContinuousDamage& _continuousDamage, DamageInfo& _info)
{
	bool result = true;
	if (healthData)
	{
		for_every(damageRule, healthData->damageRules)
		{
			if (!check_and_act_filter(damageRule, nullptr, &_continuousDamage, _info))
			{
				continue;
			}

			// process
			if (damageRule->noExplosion && _continuousDamage.explosionDamage)
			{
				result = false;
			}
			//
			if (damageRule->noContinuousDamage)
			{
				result = false;
			}
			//
			if (damageRule->addToProjectileDamage)
			{
				if (auto* p = get_owner()->get_gameplay_as<ModuleProjectile>())
				{
					auto& pcd = p->access_continuous_damage();
					if (pcd.damageType == _continuousDamage.damageType)
					{
						pcd.damage += _continuousDamage.damage;
						// and consume
						_continuousDamage.damage = Energy::zero();

						// update its look
						p->update_appearance_variables(ModuleProjectile::UAV_Damage);
					}
					else
					{
						warn(TXT("different damage type (for continuous damage), can't add to projectile from health's receiving damage"));
					}
				}
			}

			if (damageRule->stopProcessing)
			{
				break;
			}
		}
	}

	return result;
}

void Health::heal(REF_ Energy& _amountLeft)
{
	IS_MODULE_OWNER_LOCKED_HERE;

	Energy healUp = min(_amountLeft, get_max_health() - health);
	health += healUp;
	_amountLeft -= healUp;
}

void Health::on_deal_damage(Damage const & _damage, Damage const & _unadjustedDamage, DamageInfo & _info)
{
	IS_MODULE_OWNER_LOCKED_HERE;

	auto * imo = get_owner();

	update_last_pre_damage_health();

	if (_damage.damage.is_negative())
	{
		// heal!
		Energy healAmount = abs(_damage.damage);
		heal(healAmount);
	}

	if (!imo || (invincible && ! _info.forceDamage))
	{
		return;
	}

	if (!_damage.continousDamage && (!healthData->soundsOnDamage.is_empty() || ! healthData->soundsOnDamageControlledByPlayer.is_empty()))
	{
		{
			bool controlledByPlayer = false;
			if (auto* ai = imo->get_ai())
			{
				controlledByPlayer = ai->is_controlled_by_player();
			}
			auto& useSoundsOnDamage = controlledByPlayer && !healthData->soundsOnDamageControlledByPlayer.is_empty()? healthData->soundsOnDamageControlledByPlayer : healthData->soundsOnDamage;
			if (!useSoundsOnDamage.is_empty())
			{
				float damageAsFloat = _damage.damage.as_float();
				int playIdx = RandomUtils::ChooseFromContainer<Array<HealthData::SoundOnDamage>, HealthData::SoundOnDamage>::choose(rg, useSoundsOnDamage,
					[](HealthData::SoundOnDamage const & _sod) { return 1.0f; },
					[damageAsFloat](HealthData::SoundOnDamage const & _sod) { return _sod.damage.does_contain(damageAsFloat); },
					true
				);
				if (playIdx == NONE)
				{
					playIdx = RandomUtils::ChooseFromContainer<Array<HealthData::SoundOnDamage>, HealthData::SoundOnDamage>::choose(rg, useSoundsOnDamage,
						[](HealthData::SoundOnDamage const & _sod) { return 1.0f; },
						[damageAsFloat](HealthData::SoundOnDamage const & _sod) { return _sod.damage.is_empty() || _sod.damage.does_contain(damageAsFloat); },
						true
					);
				}
				if (playIdx != NONE)
				{
					if (auto* s = get_owner()->get_sound())
					{
						s->play_sound(useSoundsOnDamage[playIdx].sound);
					}
				}
			}
		}
	}

	if (health >= Energy::zero() && ! isDying)
	{
		float coef = 1.0f;
		bool ownerPlayer = Framework::GameUtils::is_player(imo);
		bool sourcePlayer = _info.source.get() && Framework::GameUtils::is_controlled_by_player(_info.source.get());
		if (ownerPlayer)
		{
			// if allows hitting by a player, have it full
			if (!sourcePlayer || ! does_allow_hit_from_same_instigator())
			{
				coef *= GameSettings::get().difficulty.playerDamageTaken;
			}
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (auto* g = Game::get_as<Game>())
			{
				if (g->is_autopilot_on())
				{
					coef = 0.0f;
				}
			}
#endif
		}
		if (sourcePlayer)
		{
			coef *= GameSettings::get().difficulty.playerDamageGiven;
		}
		Energy damageApplied = _damage.damage.mul(coef);
		if (! _damage.selfDamageCoef.is_one() && _info.source.get() && _info.source->get_top_instigator() == imo->get_top_instigator())
		{
			damageApplied = damageApplied.adjusted(_damage.selfDamageCoef);
		}
		EnergyCoef damageCoef = PilgrimBlackboard::get_health_damage_received_coef_modifier_for(get_owner());
		damageApplied = damageApplied.adjusted_plus_one(damageCoef);

		Energy damageAccumulatedNow = Energy::zero();

		if (damageApplied > Energy::zero() || _damage.instantDeath)
		{
#ifdef AN_HEALTH_VERBOSE
			output(TXT("[health] [%S] does [%S] damage of %S [org:%S]"),
				_info.source.get() ? _info.source.get()->ai_get_name().to_char() : TXT("--"),
				get_owner()->ai_get_name().to_char(),
				damageApplied.as_string_auto_decimals().to_char(),
				_damage.damage.as_string_auto_decimals().to_char());

#endif
			if (sourcePlayer)
			{
				wasDamagedByPlayer = true;
			}
			Energy extraDamage = Energy::zero();
			bool extraDamageCanKill = false;

			bool processAsUsual = true;
			if (!_damage.instantDeath && !_damage.continousDamage && healthData && ! healthData->rules.is_empty())
			{
				bool noHitBoneDamage = false;
				if (auto* gd = GameDefinition::get_chosen())
				{
					if (auto* wcm = gd->get_weapon_core_modifiers())
					{
						WeaponCoreKind::Type wck = WeaponCoreKind::from_damage_type(_damage.damageType);
						if (auto* fc = wcm->get_for_core(wck))
						{
							noHitBoneDamage = fc->noHitBoneDamage;
						}
					}
				}
				for_every(rule, healthData->rules)
				{
					if (!damageApplied.is_positive())
					{
						break;
					}
					auto& ruleInstance = ruleInstances[for_everys_index(rule)];

					// conditions
					{
						if (rule->hitBone.is_valid())
						{
							if (noHitBoneDamage)
							{
								continue;
							}
							if (!_info.hitBone.is_set())
							{
								continue;
							}
							if (rule->hitBone != _info.hitBone.get())
							{
								bool found = false;
								if (rule->includeChildrenBones)
								{
									if (auto* a = get_owner()->get_appearance())
									{
										if (auto* fs = a->get_skeleton())
										{
											if (auto* s = fs->get_skeleton())
											{
												int ruleBoneIdx = s->find_bone_index(rule->hitBone);
												int boneIdx = s->find_bone_index(_info.hitBone.get());
												if (s->is_descendant(ruleBoneIdx, boneIdx))
												{
													found = true;
												}
											}
										}
									}
								}
								if (!found)
								{
									continue;
								}
							}
						}
						if (rule->health.is_positive() && !ruleInstance.health.is_positive() && !rule->activeIfDepleted)
						{
							// already depleted
							continue;
						}
						{
							bool allRequiredVariablesOk = true;
							for_every(rv, rule->requiredVariables.get_all())
							{
								if (! RegisteredType::is_plain_data(rv->type_id()))
								{
									error_stop(TXT("only plain data!"));
									allRequiredVariablesOk = false;
									break;
								}
								if (auto* ov = get_owner()->get_variables().find_existing(rv->get_name(), rv->type_id()))
								{
									if (!memory_compare(ov->access_raw(), rv->access_raw(), RegisteredType::get_size_of(rv->type_id())))
									{
										allRequiredVariablesOk = false;
										break;
									}
								}
							}
							if (!allRequiredVariablesOk)
							{
								break;
							}
						}
					}

					// process
					if (rule->health.is_positive())
					{
						Energy damageHere = min(damageApplied, ruleInstance.health);

						if (rule->allowDamageToGoThroughOnDeplete)
						{
							// this covers both cases if we're already depleted or not
							damageApplied -= damageHere;
							damageApplied += damageHere.adjusted(rule->applyDamageToMainHealthCoef);
						}
						else if (ruleInstance.health.is_positive())
						{
							// this is only if we're not depleted.
							damageApplied = Energy::zero();
						}
						else
						{
							// we're depleted but we want to use coef (to pretend everything is as usual, ie. head is off but we may hit some leftovers)
							damageApplied = damageApplied.adjusted(rule->applyDamageToMainHealthCoef);
						}

						if (ruleInstance.health.is_positive())
						{
							damageAccumulatedNow += damageHere;
							ruleInstance.health -= damageHere;
							if (!ruleInstance.health.is_positive())
							{
								// deplete!
								if (!rule->setVariablesOnDeplete.is_empty())
								{
									get_owner()->access_variables().set_from(rule->setVariablesOnDeplete);
								}
								if (!rule->setAppearanceControllerVariablesOnDeplete.is_empty())
								{
									for_every_ptr(a, get_owner()->get_appearances())
									{
										a->access_controllers_variable_storage().set_from(rule->setAppearanceControllerVariablesOnDeplete);
									}
								}
								if (!rule->temporaryObjectsOnDeplete.is_empty())
								{
									if (auto* tom = get_owner()->get_temporary_objects())
									{
										for_every(to, rule->temporaryObjectsOnDeplete)
										{
											tom->spawn_all(*to);
										}
									}
								}
								if (!rule->soundsOnDeplete.is_empty())
								{
									if (auto* sm = get_owner()->get_sound())
									{
										for_every(s, rule->soundsOnDeplete)
										{
											sm->play_sound(*s);
										}
									}
								}
								if (rule->extraDamageOnDeplete.is_positive())
								{
									extraDamage += GameConfig::apply_health_coef(rule->extraDamageOnDeplete, healthData);
									extraDamageCanKill |= rule->extraDamageCanKill;
								}
								if (rule->dropHealthToOnDeplete.is_set())
								{
									health = min(health, GameConfig::apply_health_coef(rule->dropHealthToOnDeplete.get(), healthData));
								}
								if (!rule->applySpeedOnDeplete.is_zero())
								{
									Vector3 inDir = Vector3(rg.get(rule->applyVelocityOnDeplete.x), rg.get(rule->applyVelocityOnDeplete.y), rg.get(rule->applyVelocityOnDeplete.z)).normal();
									Vector3 velocityOS = inDir * rg.get(rule->applySpeedOnDeplete);
									get_owner()->get_presence()->add_velocity_impulse(get_owner()->get_presence()->get_placement().vector_to_world(velocityOS));
								}
								if (rule->autoDeathTime.is_set())
								{
									if (!imo->has_timer(NAME(autoDeath)))
									{
										imo->set_timer(rg.get(rule->autoDeathTime.get()), NAME(autoDeath),
											[](Framework::IModulesOwner* _imo)
											{
												if (auto* h = _imo->get_custom<Health>())
												{
													h->perform_death_without_reward();
												}
											});
									}
								}
							}
						}
					}
					else
					{
						damageApplied = damageApplied.adjusted(rule->applyDamageToMainHealthCoef);
					}

				}
			}

			if (processAsUsual)
			{
				if (damageApplied.is_positive())
				{
					if (ownerPlayer && !_damage.instantDeath)
					{
						// protect against killing in a frame, drop to 1
						if (dampedHealth > get_max_health().halved())
						{
							damageApplied = min(damageApplied, health - Energy::one());
						}
					}

					if (_damage.continousDamage && damageApplied > health - Energy::one())
					{
						Energy excessDamage = damageApplied - (health - Energy::one());
						health = Energy::one();
						handle_excess_continuous_damage(REF_ excessDamage);
						health -= excessDamage;
						damageAccumulatedNow += excessDamage;
					}
					else
					{
						health -= damageApplied;
						damageAccumulatedNow += damageApplied;
					}
				}

				if (extraDamage.is_positive())
				{
					if (!extraDamageCanKill)
					{
						extraDamage = max(Energy::zero(), min(extraDamage, health - Energy::one()));
					}
					health -= extraDamage;
					damageAccumulatedNow += extraDamage;
				}

				if (!_damage.continousDamage)
				{
					if (auto* ec = imo->get_custom<EmissiveControl>())
					{
						todo_note(TXT("hardcoded values"));
						ec->emissive_access(NAME(healthDealDamage), 9999).keep_colour().set_power(rg.get_float(0.3f, 0.7f)).activate(0.1f);
						imo->set_timer(0.1f, NAME(restoreEmissive), [ec](Framework::IModulesOwner* _imo) { ec->emissive_access(NAME(healthDealDamage)).deactivate(0.5f); });
					}

					if (!fast_cast<Framework::TemporaryObject>(imo))
					{
						if (auto* w = imo->get_presence()->get_in_world())
						{
							if (Framework::AI::Message* message = w->create_ai_message(NAME(dealtDamage)))
							{
								message->to_ai_object(imo);
								message->access_param(NAME(damage)).access_as<Damage>() = _damage.with_damage_adjusted_plus_one(damageCoef);
								message->access_param(NAME(damageUnadjusted)).access_as<Damage>() = _unadjustedDamage.with_damage_adjusted_plus_one(damageCoef);
								message->access_param(NAME(damageDamager)).access_as<SafePtr<Framework::IModulesOwner>>() = _info.damager;
								// soon damageDamager may be gone as it will cease to exist
								message->access_param(NAME(damageDamagerProjectile)).access_as<bool>() = _info.damager.get() ? _info.damager->get_gameplay_as<ModuleProjectile>() != nullptr : false;
								message->access_param(NAME(damageSource)).access_as<SafePtr<Framework::IModulesOwner>>() = _info.source;
								message->access_param(NAME(damageInstigator)).access_as<SafePtr<Framework::IModulesOwner>>() = _info.instigator;
								if (_info.hitRelativeDir.is_set())
								{
									message->access_param(NAME(hitIndicatorRelativeDir)).access_as<Vector3>() = _info.hitRelativeDir.get();
								}
								else if (_info.hitRelativeLocation.is_set())
								{
									message->access_param(NAME(hitIndicatorRelativeLocation)).access_as<Vector3>() = _info.hitRelativeLocation.get();
								}
								else if (_info.hitRelativeNormal.is_set())
								{
									// better than nothing
									message->access_param(NAME(hitIndicatorRelativeDir)).access_as<Vector3>() = -_info.hitRelativeNormal.get();
								}
							}
						}
					}

					if (Framework::GameUtils::is_local_player(get_owner())
#ifndef AN_DEVELOPMENT
						&& PhysicalSensations::is_active()
#endif
						)
					{
						PhysicalSensations::SingleSensation::Type sType = PhysicalSensations::SingleSensation::Unknown;
						if (_damage.explosionDamage)
						{
							sType = PhysicalSensations::SingleSensation::Explosion;
							if (_damage.damage <= Energy(8))
							{
								sType = PhysicalSensations::SingleSensation::ExplosionLight;
							}
						}
						else if (_damage.meleeDamage)
						{
							sType = PhysicalSensations::SingleSensation::Punched;
						}
						else if (_damage.damageType == DamageType::Plasma)
						{
							sType = PhysicalSensations::SingleSensation::Shot;
							if (_damage.damage <= Energy(3))
							{
								sType = PhysicalSensations::SingleSensation::ShotLight;
							}
							else if (_damage.damage >= Energy(8))
							{
								sType = PhysicalSensations::SingleSensation::ShotHeavy;
							}
						}
						else if (_damage.damageType == DamageType::Lightning)
						{
							sType = PhysicalSensations::SingleSensation::Electrocution;
							if (_damage.damage <= Energy(3))
							{
								sType = PhysicalSensations::SingleSensation::ElectrocutionLight;
							}
						}
						else if (_damage.damageType == DamageType::Corrosion)
						{
							sType = PhysicalSensations::SingleSensation::Shot;
							if (_damage.damage <= Energy(3)) // ??
							{
								sType = PhysicalSensations::SingleSensation::ShotLight;
							}
							else if (_damage.damage >= Energy(8)) // ??
							{
								sType = PhysicalSensations::SingleSensation::ShotHeavy;
							}
						}
						if (sType != PhysicalSensations::SingleSensation::Unknown)
						{
							PhysicalSensations::SingleSensation s(sType);
							if (_info.hitBone == NAME(head)) // hardcoded bone for player
							{
								s.for_head();
							}
							if (_info.hitRelativeDir.is_set())
							{
								s.with_dir_os(_info.hitRelativeDir.get());
							}
							if (_info.hitRelativeLocation.is_set())
							{
								s.with_hit_loc_os(_info.hitRelativeLocation.get());
							}
							if (auto* p = get_owner()->get_presence())
							{
								Transform eyesOS = p->get_eyes_relative_look();
								if (!eyesOS.get_translation().is_zero())
								{
									float eyesHeightOS = eyesOS.get_translation().z;
									s.with_eye_height_os(eyesHeightOS);

									if (_info.hitRelativeLocation.is_set())
									{
										float hitRelativeLocationZOS = _info.hitRelativeLocation.get().z;
										if (hitRelativeLocationZOS > eyesHeightOS - 0.15f)
										{
											s.for_head();
										}
									}
								}
							}
							PhysicalSensations::start_sensation(s);

						}
					}
				}

				// store last damage instigator
				lastDamageInstigator = _info.instigator.get();

				{
					bool actImmortal = false;
					if (immortal.is_set())
					{
						actImmortal = immortal.get();
					}
					else
					{
						if (_info.cantKillPilgrim &&
							imo->get_gameplay_as<ModulePilgrim>())
						{
							actImmortal = true;
						}
						if (GameSettings::get().difficulty.playerImmortal && !_info.forceDamage)
						{
							if (Framework::GameUtils::is_player(imo)) // this is only for player, objects held by player shall die
							{
								actImmortal = true;
							}
						}
					}
					if (actImmortal)
					{
						Energy minHealth = Energy::one();
						if (health < minHealth)
						{
							immortal_health_reached_below_minimum_by(health - minHealth);
							trigger_game_script_trap_on_health_reaches_bottom();
						}
						health = max(Energy::one(), health);
					}
				}

				if (health <= Energy::zero())
				{
					if (auto* mp = get_owner()->get_gameplay_as<ModulePilgrim>())
					{
						if (mp->has_exm_equipped(EXMID::Passive::another_chance()))
						{
							if (auto* exm = EXMType::find(EXMID::Passive::another_chance()))
							{
								Energy fillHealthUpTo = Energy::get_from_storage(exm->get_custom_parameters(), NAME(fillHealth), Energy::one());
								Energy costEnergyForOneHealth = Energy::get_from_storage(exm->get_custom_parameters(), NAME(costEnergyForOneHealth), Energy::one());
								Energy energyCost = fillHealthUpTo * costEnergyForOneHealth;
								if (GameSettings::get().difficulty.commonHandEnergyStorage)
								{
									Energy currHand = mp->get_hand_energy_storage(GameSettings::actual_hand(Hand::Left));

									if (!currHand.is_positive())
									{
										fillHealthUpTo = Energy::zero();
									}
									else if (currHand >= energyCost)
									{
										// use full fillHealthUpTo
										mp->set_hand_energy_storage(GameSettings::actual_hand(Hand::Left), currHand - energyCost);
									}
									else
									{
										fillHealthUpTo = min(fillHealthUpTo, currHand / costEnergyForOneHealth);
										mp->set_hand_energy_storage(GameSettings::actual_hand(Hand::Left), Energy::zero());
									}
								}
								else
								{
									Energy currHandL = mp->get_hand_energy_storage(GameSettings::actual_hand(Hand::Left));
									Energy currHandR = mp->get_hand_energy_storage(GameSettings::actual_hand(Hand::Right));

									if (!currHandL.is_positive() &&
										!currHandR.is_positive())
									{
										fillHealthUpTo = Energy::zero();
									}
									else if (currHandL + currHandR >= energyCost)
									{
										// use full fillHealthUpTo

										float useLeftPt = currHandL.div_to_float(currHandL + currHandR);

										Energy useLeft = energyCost.mul(useLeftPt);
										Energy useRight = energyCost - useLeft;

										if (currHandR < useRight)
										{
											Energy diff = useRight - currHandR;
											useLeft += diff;
											useRight -= diff;
										}
										if (currHandL < useLeft)
										{
											Energy diff = useLeft - currHandL;
											useRight += diff;
											useLeft -= diff;
										}

										mp->set_hand_energy_storage(GameSettings::actual_hand(Hand::Left), currHandL - useLeft);
										mp->set_hand_energy_storage(GameSettings::actual_hand(Hand::Right), currHandR - useRight);
									}
									else
									{
										fillHealthUpTo = min(fillHealthUpTo, (currHandL + currHandR) / costEnergyForOneHealth);
										mp->set_hand_energy_storage(GameSettings::actual_hand(Hand::Left), Energy::zero());
										mp->set_hand_energy_storage(GameSettings::actual_hand(Hand::Right), Energy::zero());
									}
								}

								if (fillHealthUpTo.is_positive())
								{
									set_health(fillHealthUpTo);
									float invulnerabilityTime = exm->get_custom_parameters().get_value<float>(NAME(invulnerabilityTime), 1.0f);
									be_invincible(invulnerabilityTime);

									if (auto* s = get_owner()->get_sound())
									{
										s->play_sound(NAME(revival));
									}
									if (auto* hi = get_owner()->get_custom<CustomModules::HitIndicator>())
									{
										Colour colour;
										colour.load_from_string(String(TXT("pilgrim_overlay_system")));
										for_count(int, i, 6)
										{
											CustomModules::HitIndicator::IndicateParams ip;
											ip.with_colour_override(colour);
											ip.with_delay((float)i * 0.15f);
											ip.is_damage(false);
											ip.is_reversed(false);
											hi->indicate_relative_dir(0.25f, Vector3(0.0f, 0.0f, 1.0f), ip);
										}
									}
								}
							}
						}
					}
				}

				if (health <= Energy::zero())
				{
					health_reached_below_zero(_damage, _unadjustedDamage, _info);
					trigger_game_script_trap_on_health_reaches_bottom();
				}
				else if (_info.requestCombatAuto)
				{
					// music
					{
						if (ownerPlayer)
						{
							MusicPlayer::request_combat_auto();
						}
						else if (sourcePlayer)
						{
							if (auto* imo = get_owner())
							{
								if (auto* ai = imo->get_ai())
								{
									if (auto* mind = ai->get_mind())
									{
										if (mind->get_social().is_enemy_or_owned_by_enemy(_info.source.get()))
										{
											MusicPlayer::request_combat_auto();
										}
									}
								}
							}
						}
					}
				}
			}
		}

		// store for local damage
		if (PilgrimOverlayInfo::should_collect_health_damage_for_local_player())
		{
			auto* iimo = _info.instigator.get();
			if (!iimo)
			{
				iimo = lastDamageInstigator.get();
			}
			if (iimo)
			{
				if (auto* timo = iimo->get_top_instigator())
				{
					if (Framework::GameUtils::is_local_player(timo))
					{
						if (auto* mp = timo->get_gameplay_as<ModulePilgrim>())
						{
							mp->access_overlay_info().collect_health_damage(imo, damageAccumulatedNow);
						}
					}
				}
			}
		}
	}

	if (health.is_positive() &&
		_damage.induceConfussion)
	{
		if (auto* message = imo->get_in_world()->create_ai_message(NAME(aimConfussion)))
		{
			message->access_param(NAME(confussionDuration)).access_as<float>() = _damage.induceConfussion;
			message->to_ai_object(imo);
		}
	}

	if (!_damage.continousDamage && should_affect_velocity())
	{
		if (_info.hitRelativeNormal.is_set() || _info.hitRelativeDir.is_set())
		{
			if (auto * presence = imo->get_presence())
			{
				Vector3 dir = Vector3::zero;
				if (_info.hitRelativeDir.is_set())
				{
					dir = _info.hitRelativeDir.get();
				}
				else
				{
					dir = -_info.hitRelativeNormal.get();
				}
				float massAdjustedUnadjustedDamageValue = _unadjustedDamage.damage.as_float();
				if (auto* c = imo->get_collision())
				{
					float m = c->get_mass();
					m = clamp(m, 10.0f, 500.0f);
					float baseMass = 100.0f;
					massAdjustedUnadjustedDamageValue = massAdjustedUnadjustedDamageValue * baseMass / m;
				}
				Vector3 oldVelocity = presence->get_velocity_linear();
				float affectVelocityImpulse = affectVelocityLinearPerDamagePoint * massAdjustedUnadjustedDamageValue * _damage.affectVelocity;
				if (affectVelocityLinearAffectLimit > 0.0f)
				{
					affectVelocityImpulse = min(affectVelocityLinearAffectLimit, affectVelocityImpulse);
				}
				float useVelocityLinearLimit = affectVelocityLinearLimit;
				if (_damage.forceAffectVelocityLinearPerDamagePoint != 0.0f)
				{
					useVelocityLinearLimit = max(useVelocityLinearLimit, forcedAffectVelocityLinearLimit);
					float fromForced = _damage.forceAffectVelocityLinearPerDamagePoint * massAdjustedUnadjustedDamageValue;
					affectVelocityImpulse += fromForced;
				}
				Vector3 newVelocity = oldVelocity + presence->get_placement().vector_to_world(dir) * affectVelocityImpulse;
				if (useVelocityLinearLimit > 0.0f)
				{
					newVelocity = newVelocity.normal() * clamp(newVelocity.length(), 0.0f, max(oldVelocity.length(), useVelocityLinearLimit));
				}
				presence->set_velocity_linear(newVelocity);
			}
		}

		if (!affectVelocityRotationLimit.is_zero())
		{
			if (auto * presence = imo->get_presence())
			{
				Rotator3 current = presence->get_velocity_rotation();
				Rotator3 affectLimit = affectVelocityRotationLimit;
				Rotator3 affect = Rotator3::zero;
				Rotator3 throughDamagePoints = affectVelocityRotationPerDamagePoint * _unadjustedDamage.damage.as_float() * _damage.affectVelocity;
				affect.yaw = clamp(rg.get_float(0.0f, 1.5f), 0.0f, 1.0f) * throughDamagePoints.yaw;
				affect.pitch = clamp(rg.get_float(0.0f, 1.5f), 0.0f, 1.0f) * throughDamagePoints.pitch;
				affect.roll = clamp(rg.get_float(0.0f, 1.5f), 0.0f, 1.0f) * throughDamagePoints.roll;
				affect.yaw = clamp(affect.yaw, 0.0f, affectLimit.yaw);
				affect.pitch = clamp(affect.yaw, 0.0f, affectLimit.pitch);
				affect.roll = clamp(affect.yaw, 0.0f, affectLimit.roll);
				affect.yaw *= rg.get_chance(0.5f) ? 1.0f : -1.0f;
				affect.pitch *= rg.get_chance(0.5f) ? 1.0f : -1.0f;
				affect.roll *= rg.get_chance(0.5f) ? 1.0f : -1.0f;
				presence->set_velocity_rotation(current + affect);
			}
		}
	}

	if (health <= Energy::zero() && !particlesOnDeath)
	{
		_info.spawnParticles = false;
	}
}

void Health::appear_dead(bool _appearDead)
{
	auto * imo = get_owner();

	imo->access_variables().access<float>(NAME(aliveAutoControllersOff)) = _appearDead? 1.0f : 0.0f;
	if (auto* ec = imo->get_custom<EmissiveControl>())
	{
		if (_appearDead)
		{
			ec->set_power(0.0f, 0.3f);
		}
		else
		{
			ec->set_default_power(0.3f);
		}
	}
}

void Health::update_last_pre_damage_health()
{
	int systemFrame = ::System::Core::get_frame();
	if (systemFrame != lastPreDamageHealthFrame)
	{
		lastPreDamageHealthFrame = systemFrame;
		lastPreDamageHealth = health;
	}
}

void Health::give_reward_for_kill(Optional<Framework::IModulesOwner*> const& _deathInstigator, Optional<Framework::IModulesOwner*> const& _source)
{
	MODULE_OWNER_LOCK(TXT("Health::give_reward_for_kill"));

	if (rewardForKillGiven || noRewardForKill)
	{
		return;
	}

	Framework::IModulesOwner* di = _deathInstigator.get(nullptr);
	if (!di)
	{
		di = deathInstigator.get();
	}
	if (!di)
	{
		di = lastDamageInstigator.get();
	}

	if (di)
	{
		rewardForKillGiven = true;

		// to make sure we reach to the player
		if (auto* topInstigator = di->get_top_instigator())
		{
			auto* imo = get_owner();

			if (Framework::GameUtils::is_local_player(topInstigator))
			{
				// we shouldn't be killing now
				GameDirector::narrative_mode_trust_lost();

				{
					if (auto* ai = imo->get_ai())
					{
						if (auto* mind = ai->get_mind())
						{
							if (mind->get_social().is_enemy_or_owned_by_enemy(topInstigator))
							{
								MusicPlayer::request_combat_auto_calm_down();
							}
						}
					}
				}

				{
					Energy experience = Energy::zero();
					imo->for_all_custom<LootProvider>([&experience](LootProvider* odr)
						{
							experience += odr->get_experience();
						});
					if (experience.is_positive())
					{
						experience = experience.adjusted_plus_one(GameSettings::get().experienceModifier);
						PlayerSetup::access_current().stats__experience(experience);
						GameStats::get().add_experience(experience);
						Persistence::access_current().provide_experience(experience);
					}
				}
				// this should be after experience calculation as it depends on the kill count

				Persistence::access_current().mark_killed(imo);
				PlayerSetup::access_current().stats__kill();
				GameStats::get().killed();
				{
					auto& e = GameLog::get().add_entry(GameLog::Entry(NAME(died)));
					if (di)
					{
						e.set_as_name(DIED__INSTIGATOR, di->get_library_name().get_name());
						if (Framework::GameUtils::is_local_player(di))
						{
							e.set_as_bool(DIED__BY_LOCAL_PLAYER, true);
						}
					}
					if (auto* s = _source.get(nullptr))
					{
						e.set_as_name(DIED__SOURCE, s->get_library_name().get_name());
					}
				}
				GameLog::get().check_for_energy_balance(topInstigator);
			}
		}
	}

	if (auto* imo = get_owner())
	{
		if (imo->get_variables().get_value<bool>(NAME(storeExistenceInPilgrimage), false))
		{
			if (auto* piow = PilgrimageInstance::get())
			{
				auto* wa = imo->get_variables().get_existing<Framework::WorldAddress>(NAME(storeExistenceInPilgrimageWorldAddress));
				auto* waPOIIdx = imo->get_variables().get_existing<int>(NAME(storeExistenceInPilgrimagePOIIdx));
				if (wa && waPOIIdx)
				{
					piow->mark_killed(*wa, *waPOIIdx);
				}
			}
		}
	}
}

void Health::health_reached_below_zero(Damage const & _damage, Damage const & _unadjustedDamage, DamageInfo & _info)
{
	auto * imo = get_owner();

#ifdef AN_HEALTH_IMPORTANT_INFOS
	output(TXT("[health] [%S] died due to low health"), get_owner()->ai_get_name().to_char());
#endif

	setup_death_params(_info.peacefulDamage, _damage.meleeDamage, _info.instigator.get());

	imo->clear_timer(NAME(restoreEmissive));

	imo->access_variables().access<bool>(NAME(dead)) = true;
	imo->access_variables().access<bool>(NAME(voiceless)) = true; // dead can't talk

	if (_info.source.get())
	{
		if (auto* eq = _info.source->get_gameplay_as<ModuleEquipment>())
		{
			eq->mark_kill();
		}
	}

	give_reward_for_kill(_info.instigator.get(), _info.source.get());

	if (auto * w = imo->get_presence()->get_in_world())
	{
		if (Framework::AI::Message* message = w->create_ai_message(NAME(death)))
		{
			message->to_ai_object(imo);
			message->access_param(NAME(damage)).access_as<Damage>() = _damage;
			message->access_param(NAME(killer)).access_as<SafePtr<Framework::IModulesOwner>>() = _info.source;
		}
		if (Framework::AI::Message* message = w->create_ai_message(NAME(someoneElseDied)))
		{
			message->to_all_in_range(imo->get_presence()->get_in_room(), imo->get_presence()->get_centre_of_presence_WS(), 10.0f);
			message->access_param(NAME(damage)).access_as<Damage>() = _damage;
			message->access_param(NAME(killed)).access_as<SafePtr<Framework::IModulesOwner>>() = imo;
			message->access_param(NAME(killer)).access_as<SafePtr<Framework::IModulesOwner>>() = _info.source;
		}
	}

	if (auto* ai = imo->get_ai())
	{
#ifdef AN_DEVELOPMENT
		if (auto* logic = ai->get_mind()->get_logic())
		{
			ai_log(logic, TXT("TERMINATED"));
			if (_info.source.get())
			{
				if (auto* topInstigator = _info.source->get_valid_top_instigator())
				{
					ai_log(logic, TXT("by %S"), topInstigator->ai_get_name().to_char());
				}
			}
		}
#endif
		// disable ai - stop, etc
		if (healthData && healthData->disableMindOnDeath)
		{
			ai->get_mind()->set_enabled(false);
		}
	}

	if (auto* s = imo->get_sound())
	{
		for_every_ref(sound, s->access_sounds().access_sounds())
		{
			if (sound->get_tags().get_tag(NAME(alive)))
			{
				sound->stop();
			}
		}
	}

	bool continueWithDeath = true;
	if (auto * g = imo->get_gameplay_as<ModuleGameplay>())
	{
		continueWithDeath = g->on_death(_damage, _info);
	}
	else if (auto * g = imo->get_gameplay_as<ModuleProjectile>())
	{
		continueWithDeath = g->on_death(_damage, _info);
	}

	on_death(_damage, _info);

	if (continueWithDeath)
	{
		deathInstigator = _info.instigator.get() ? _info.instigator.get() : imo->get_valid_top_instigator();
		deathRelativeDir = _info.hitRelativeDir.get(Vector3::zero);
		deathDamageType = _damage.damageType;

		bool performDeath = true;
		if (auto * s = imo->get_sound())
		{
			s->stop_sounds(AI::Common::sounds_talk());
		}

		if (performDeathAIMessage.is_valid())
		{
			if (auto * w = imo->get_presence()->get_in_world())
			{
				if (Framework::AI::Message* message = w->create_ai_message(performDeathAIMessage))
				{
					message->to_ai_object(imo);
					message->access_param(NAME(damage)).access_as<Damage>() = _damage;
					message->access_param(NAME(damageUnadjusted)).access_as<Damage>() = _unadjustedDamage;
					message->access_param(NAME(damageDamager)).access_as<SafePtr<Framework::IModulesOwner>>() = _info.damager;
					// soon damageDamager may be gone as it will cease to exist
					message->access_param(NAME(damageDamagerProjectile)).access_as<bool>() = _info.damager.get()? _info.damager->get_gameplay_as<ModuleProjectile>() != nullptr : false;
					message->access_param(NAME(damageSource)).access_as<SafePtr<Framework::IModulesOwner>>() = _info.source;
					message->access_param(NAME(damageInstigator)).access_as<SafePtr<Framework::IModulesOwner>>() = _info.instigator;
					if (_info.hitRelativeDir.is_set())
					{
						message->access_param(NAME(hitIndicatorRelativeDir)).access_as<Vector3>() = _info.hitRelativeDir.get();
					}
					else if (_info.hitRelativeLocation.is_set())
					{
						message->access_param(NAME(hitIndicatorRelativeLocation)).access_as<Vector3>() = _info.hitRelativeLocation.get();
					}
					else if (_info.hitRelativeNormal.is_set())
					{
						// better than nothing
						message->access_param(NAME(hitIndicatorRelativeDir)).access_as<Vector3>() = -_info.hitRelativeNormal.get();
					}
					performDeath = false;
				}
			}
		}
		if (performDeath)
		{
			float useDelayDeath = delayDeath;
			useDelayDeath = get_owner()->get_variables().get_value<float>(NAME(delayDeath), useDelayDeath);
			if (useDelayDeath == 0.0f)
			{
				perform_death();
			}
			else
			{
				imo->set_timer(useDelayDeath,
					[](Framework::IModulesOwner* _imo)
					{
						if (auto* h = _imo->get_custom<Health>())
						{
							h->perform_death();
						}
					});
			}
		}
	}
	else
	{
		// if we continue with death, perform_death handles this
		clear_continuous_damage();
	}

	isDying = true;
}

void Health::trigger_game_script_trap_on_death()
{
	if (triggerGameScriptTrapOnDeath.is_valid())
	{
		Framework::GameScript::ScriptExecution::trigger_execution_trap(triggerGameScriptTrapOnDeath);
	}
}

void Health::trigger_game_script_trap_on_health_reaches_bottom()
{
	if (triggerGameScriptTrapOnHealthReachesBottom.is_valid())
	{
		Framework::GameScript::ScriptExecution::trigger_execution_trap(triggerGameScriptTrapOnHealthReachesBottom);
	}
}

void Health::change_appearance_on_death()
{
	auto* imo = get_owner();
	for_every(cmod, healthData->changeAppearanceOnDeath)
	{
		Name hideAppearanceName = cmod->hideAppearance;
		Name showAppearanceName = cmod->showAppearance;
		Name setMainAppearanceName = cmod->setMainAppearance;
		imo->set_timer(max(0.002f, cmod->delay),
			[hideAppearanceName, showAppearanceName, setMainAppearanceName](Framework::IModulesOwner* _imo)
			{
				if (hideAppearanceName.is_valid())
				{
					if (auto* a = _imo->get_appearance_named(hideAppearanceName))
					{
						a->be_visible(false);
					}
				}
				if (showAppearanceName.is_valid())
				{
					if (auto* a = _imo->get_appearance_named(showAppearanceName))
					{
						a->be_visible(true);
					}
				}
				if (setMainAppearanceName.is_valid())
				{
					if (auto* a = _imo->get_appearance_named(setMainAppearanceName))
					{
						a->be_main_appearance();
					}
				}
			});
	}
}

void Health::spawn_particles_on_death(OPTIONAL_ OUT_ bool * _allowDecompose, OPTIONAL_ OUT_ float * _useCeaseOnDeathDelay)
{
	auto* imo = get_owner();
#ifndef NO_TEMPORARY_OBJECTS_ON_DEATH
	auto* deathParticlesInstigator = deathInstigator.get();
#endif
	for_every(sod, healthData->onDeath)
	{
		if (sod->notDuringPeacefulDeath && peacefulDeath)
		{
			// skip peaceful death
			continue;
		}
		if (sod->ifExtraEffectActive.is_valid())
		{
			bool ok = false;
			for_every(ee, extraEffects)
			{
				if (ee->name == sod->ifExtraEffectActive)
				{
					ok = true;
					break;
				}
			}
			if (!ok)
			{
				continue;
			}
		}
		if (!sod->preDeathHealth.is_empty() &&
			!sod->preDeathHealth.does_contain_min_exc_max_inc(lastPreDamageHealth))
		{
			// skip it, this is for a different range
			continue;
		}
		if (sod->forDamageType.is_set() &&
			(!deathDamageType.is_set() || deathDamageType.get() != sod->forDamageType.get()))
		{
			continue;
		}
		if (sod->forDamageTypeNot.is_set() &&
			(deathDamageType.is_set() && deathDamageType.get() == sod->forDamageTypeNot.get()))
		{
			continue;
		}
		if (sod->ceaseOnDeathDelay.is_set())
		{
			assign_optional_out_param(_useCeaseOnDeathDelay, sod->ceaseOnDeathDelay.get());
		}
		if (sod->disallowDecompose)
		{
			assign_optional_out_param(_allowDecompose, false);
		}
#ifndef NO_TEMPORARY_OBJECTS_ON_DEATH
		if (sod->temporaryObject.is_valid())
		{
			MEASURE_AND_OUTPUT_PERFORMANCE(TXT("spawn on death of \"%S\" to=\"%S\""), get_owner()->ai_get_name().to_char(), sod->temporaryObject.to_char());
			if (auto* to = imo->get_temporary_objects())
			{
				// may do damage
				Array<SafePtr<Framework::IModulesOwner>> spawned;
				to->spawn_all(sod->temporaryObject, NP, &spawned);
				for_every_ref(s, spawned)
				{
					s->set_creator(imo);
					s->set_instigator(deathParticlesInstigator);
				}
			}
		}
		else if (sod->temporaryObjectType.is_set())
		{
			if (auto * particleType = sod->temporaryObjectType.get())
			{
				MEASURE_AND_OUTPUT_PERFORMANCE(TXT("spawn on death of \"%S\" pt=\"%S\""), get_owner()->ai_get_name().to_char(), particleType->get_name().to_string().to_char());
				if (auto * subWorld = imo->get_as_object()->get_in_sub_world())
				{
					if (auto* particles = subWorld->get_one_for(particleType, imo->get_presence()->get_in_room()))
					{
						particles->set_creator(imo);
						particles->set_instigator(deathParticlesInstigator);
						particles->on_activate_attach_to(imo);
						particles->on_activate_place_in_fail_safe(imo->get_presence()->get_in_room(), imo->get_presence()->get_centre_of_presence_transform_WS());
						particles->mark_to_activate();
					}
				}
			}
		}
#endif
	}
}

void Health::perform_quick_death(Optional<Framework::IModulesOwner*> const& _deathInstigator, Optional<Vector3> const & _deathRelativeDir)
{
	MODULE_OWNER_LOCK(TXT("Health::perform_quick_death"));

	auto * imo = get_owner();
	if (!imo || isPerformingDeath)
	{
		return;
	}
	isPerformingDeath = true;
	isDying = true;
	peacefulDeath = true;
	lootableDeath = false;

	give_reward_for_kill(_deathInstigator);

	clear_continuous_damage();
	clear_extra_effects();

	change_appearance_on_death();

	trigger_game_script_trap_on_death();

	bool useCeaseOnDeath = healthData->ceaseOnDeath;

	spawn_particles_on_death();

	if (!peacefulDeath && lootableDeath)
	{
		imo->for_all_custom<LootProvider>([this, imo](LootProvider* odr)
		{
			Vector3 hitRelativeDir = deathRelativeDir;
			imo->set_timer(0.001f, [odr, hitRelativeDir](Framework::IModulesOwner* _imo) { odr->release_loot(hitRelativeDir, nullptr); });
		});
	}
	if (useCeaseOnDeath)
	{
		imo->set_timer(0.002f, [](Framework::IModulesOwner* _imo) { _imo->cease_to_exist(true); });
	}
}

void Health::perform_death(SafePtr<Framework::IModulesOwner> const& _deathInstigator)
{
	perform_death(_deathInstigator.get(), NP);
}

void Health::override_death_effects(Optional<bool> const& _allowDecompose, Optional<float> const& _ceaseOnDeathDelay)
{
	overrideDeathEffects.allowDecompose = _allowDecompose;
	overrideDeathEffects.ceaseOnDeathDelay = _ceaseOnDeathDelay;
}

void Health::perform_death_without_reward()
{
	noRewardForKill = true;
	lootableDeath = false;
	perform_death(nullptr, NP);
}

void Health::setup_death_params(Optional<bool> const& _peacefulDeath, Optional<bool> const& _meleeDeath, Framework::IModulesOwner* _deathInstigator)
{
	peacefulDeath = _peacefulDeath.get(false);
	//lootableDeath = (!_meleeDeath.get(false) && Framework::GameUtils::is_controlled_by_player(_deathInstigator)); hardcoded /* loot released only when not using melee and last blow done by the player */
	lootableDeath = Framework::GameUtils::is_controlled_by_player(_deathInstigator); hardcoded /* loot released only when the last blow done by the player */
}

void Health::perform_death(Optional<Framework::IModulesOwner*> const & _deathInstigator, Optional<Vector3> const & _deathRelativeDir)
{
	MODULE_OWNER_LOCK(TXT("Health::perform_death"));

	auto * imo = get_owner();
	if (!imo || isPerformingDeath)
	{
		return;
	}
	isPerformingDeath = true;
	isDying = true;

	give_reward_for_kill(_deathInstigator);

	if (health.is_positive())
	{
		health = Energy::zero();
	}

	if (_deathInstigator.is_set())
	{
		deathInstigator = _deathInstigator.get();
	}
	if (_deathRelativeDir.is_set())
	{
		deathRelativeDir = _deathRelativeDir.get();
	}
	bool useCeaseOnDeath = healthData->ceaseOnDeath;
	float useCeaseOnDeathDelay = healthData->ceaseOnDeathDelay;

	change_appearance_on_death();

	trigger_game_script_trap_on_death();

	bool allowDecompose = true;

	allowDecompose = overrideDeathEffects.allowDecompose.get(allowDecompose);
	useCeaseOnDeathDelay = overrideDeathEffects.ceaseOnDeathDelay.get(useCeaseOnDeathDelay);

	spawn_particles_on_death(OUT_ &allowDecompose, OUT_ &useCeaseOnDeathDelay);

	if (!peacefulDeath && lootableDeath)
	{
		imo->for_all_custom<LootProvider>([this, imo, _deathInstigator, useCeaseOnDeathDelay](LootProvider* odr)
		{
			Vector3 hitRelativeDir = deathRelativeDir;
			SafePtr<Framework::IModulesOwner> deathInstigator(_deathInstigator.get(nullptr));
			imo->set_timer(max(0.001f, useCeaseOnDeathDelay * magic_number 0.667f), [odr, hitRelativeDir, deathInstigator](Framework::IModulesOwner* _imo) { odr->release_loot(hitRelativeDir, deathInstigator.get()); });
		});
	}
	if (useCeaseOnDeath)
	{
		if (allowDecompose && decompose)
		{
			decomposingFor = 0.0f;
			decomposeTime = max(0.01f, useCeaseOnDeathDelay);
			mark_requires(all_customs__advance_post, HealthPostMoveReason::Decomposing);

			clear_extra_effects();

			for_every_ref(sto, spawnedTemporaryObjects)
			{
				if (auto* to = fast_cast<Framework::TemporaryObject>(sto))
				{
					Framework::ParticlesUtils::desire_to_deactivate(to);
				}
			}
		}
		else
		{
			clear_continuous_damage();
			clear_extra_effects();
			imo->set_timer(max(0.002f, useCeaseOnDeathDelay), [](Framework::IModulesOwner* _imo) { _imo->cease_to_exist(true); });
		}
	}

	clear_continuous_damage();
}

bool Health::handle_health_energy_transfer_request(EnergyTransferRequest & _request)
{
	if (noEnergyTransfer)
	{
		return false;
	}
	switch (_request.type)
	{
	case EnergyTransferRequest::Query:
	case EnergyTransferRequest::QueryWithdraw:
	case EnergyTransferRequest::QueryDeposit:
		break;
	case EnergyTransferRequest::Deposit:
		{
			MODULE_OWNER_LOCK(TXT("Health::handle_health_energy_transfer_request  deposit"));
			Energy addHealth = min(_request.energyRequested, get_max_health() - health);
			if (! _request.fillOrKill || addHealth == _request.energyRequested)
			{
				health += addHealth;
				_request.energyRequested -= addHealth;
				_request.energyResult += addHealth;
			}
		}
		break;
	case EnergyTransferRequest::Withdraw:
		{
			MODULE_OWNER_LOCK(TXT("Health::handle_health_energy_transfer_request  withdraw"));
			Energy giveHealth = min(_request.energyRequested, health - _request.minLeft);
			if (Framework::GameUtils::is_player(get_owner()))
			{
				// disallow killing yourself
				giveHealth = max(Energy::zero(), min(_request.energyRequested, health - Energy::one()));
			}
			if (!_request.fillOrKill || giveHealth == _request.energyRequested)
			{
				health -= giveHealth;
				if (health <= Energy::zero()) // for withdrawal 0 kills
				{
					Damage damage;
					damage.damage = giveHealth;
					DamageInfo info;
					info.instigator = _request.instigator;
					info.peacefulDamage = true;
					health_reached_below_zero(damage, damage, info);
				}
				_request.energyRequested -= giveHealth;
				_request.energyResult += giveHealth;
			}
		}
		break;
	}
	return true;
}

void Health::ex__set_total_health(Energy const& _health)
{
	Energy left = _health;
	health = min(left, get_max_health()); left -= health;
}

void Health::ex__drop_health_by(Energy const& _dropBy, Optional<Energy> const & _min)
{
	Energy left = _dropBy;
	{
		Energy l = min(left, health - _min.get(Energy::zero()));
		health -= l;
		left -= l;
	}
}

void Health::store_for_game_state(SimpleVariableStorage& _variables) const
{
	_variables.access<Energy>(NAME(health)) = health;
}

void Health::restore_from_game_state(SimpleVariableStorage const& _variables)
{
	if (auto* e = _variables.get_existing<Energy>(NAME(health)))
	{
		health = clamp(*e, Energy::zero(), get_max_health());
	}
}

bool Health::is_rule_health_active(Name const& _id) const
{
	if (healthData)
	{
		for_every(hr, healthData->rules)
		{
			if (hr->id == _id)
			{
				return is_rule_health_active(for_everys_index(hr));
			}
		}
	}
	return false;
}

bool Health::is_rule_health_active(int _idx) const
{
	return ruleInstances.is_index_valid(_idx) && ruleInstances[_idx].health.is_positive();
}

Optional<Energy> Health::get_rule_health(int _idx) const
{
	if (ruleInstances.is_index_valid(_idx) &&
		ruleInstances[_idx].health.is_positive())
	{
		return ruleInstances[_idx].health;
	}
	return NP;
}

bool Health::get_rule_hit_info(int _idx, OUT_ Optional<Name>& _hitBone, OUT_ Optional<Name>& _hitSocketInvestigateInfo) const
{
	if (healthData &&
		healthData->rules.is_index_valid(_idx))
	{
		if (healthData->rules[_idx].hitBone.is_valid())
		{
			_hitBone = healthData->rules[_idx].hitBone;
		}
		if (healthData->rules[_idx].hitSocketInvestigateInfo.is_valid())
		{
			_hitSocketInvestigateInfo = healthData->rules[_idx].hitSocketInvestigateInfo;
		}
		return true;
	}
	return false;
}

Optional<EnergyCoef> Health::get_prone_to_explosions() const
{
	if (healthData)
	{
		for_every(damageRule, healthData->damageRules)
		{
			if (damageRule->forExplosionDamage.get(false))
			{
				return damageRule->applyCoef;
			}
		}
	}
	return NP;
}

//

REGISTER_FOR_FAST_CAST(HealthData);

HealthData::HealthData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

HealthData::~HealthData()
{
}

bool HealthData::read_parameter_from(IO::XML::Attribute const * _attr, Framework::LibraryLoadingContext & _lc)
{
	if (_attr->get_name() == TXT("dontCeaseOnDeath"))
	{
		ceaseOnDeath = ! _attr->get_as_bool();
		return true;
	}
	else if (_attr->get_name() == TXT("ceaseOnDeath"))
	{
		ceaseOnDeath = _attr->get_as_bool();
		return true;
	}
	else if (_attr->get_name() == TXT("ceaseOnDeathDelay"))
	{
		ceaseOnDeathDelay = _attr->get_as_float();
		return true;
	}
	else
	{
		return base::read_parameter_from(_attr, _lc);
	}
}

bool HealthData::read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("rules"))
	{
		bool result = true;
		for_every(node, _node->children_named(TXT("rule")))
		{
			Rule hr;
			if (hr.load_from_xml(node, this))
			{
				rules.push_back(hr);
			}
			else
			{
				result = false;
			}
		}
		return result;
	}
	else if (_node->get_name() == TXT("rule"))
	{
		Rule hr;
		if (hr.load_from_xml(_node, this))
		{
			rules.push_back(hr);
			return true;
		}
		else
		{
			return false;
		}
	}
	if (_node->get_name() == TXT("damageRules"))
	{
		bool result = true;
		for_every(node, _node->children_named(TXT("damageRule")))
		{
			DamageRule hr;
			if (hr.load_from_xml(node, _lc))
			{
				damageRules.push_back(hr);
			}
			else
			{
				result = false;
			}
		}
		for_every(node, _node->children_named(TXT("include")))
		{
			Framework::UsedLibraryStored<DamageRuleSet> drs;
			if (drs.load_from_xml(node, TXT("damageRuleSet"), _lc))
			{
				includeDamageRuleSets.push_back(drs);
			}
			else
			{
				result = false;
			}
		}
		return result;
	}
	else if (_node->get_name() == TXT("damageRule"))
	{
		error_loading_xml(_node, TXT("put into damageRules"));
		return false;
	}
	else if (_node->get_name() == TXT("onDeath"))
	{
		bool result = true;
		OnDeath sod;
		sod.disallowDecompose = _node->get_bool_attribute_or_from_child_presence(TXT("disallowDecompose"), sod.disallowDecompose);
		sod.notDuringPeacefulDeath = _node->get_bool_attribute_or_from_child_presence(TXT("notDuringPeacefulDeath"), sod.notDuringPeacefulDeath);
		sod.ifExtraEffectActive = _node->get_name_attribute_or_from_child(TXT("ifExtraEffectActive"), sod.ifExtraEffectActive);
		sod.preDeathHealth.load_from_xml(_node, TXT("preDeathHealth"));
		sod.ceaseOnDeathDelay.load_from_xml(_node, TXT("ceaseOnDeathDelay"));
		sod.forDamageType.load_from_xml(_node, TXT("forDamageType"));
		sod.forDamageTypeNot.load_from_xml(_node, TXT("forDamageTypeNot"));
		if (auto* attr = _node->get_attribute(TXT("temporaryObject")))
		{
			sod.temporaryObject = attr->get_as_name();
		}
		else if (auto* attr = _node->get_attribute(TXT("temporaryObjectType")))
		{
			result &= sod.temporaryObjectType.load_from_xml(attr, _lc);
		}
		else
		{
			// it's okay to not have anything to spawn
		}
		if (result)
		{
			onDeath.push_back(sod);
			return true;
		}
		else
		{
			error_loading_xml(_node, TXT("no valid thing defined for onDeath"));
			return false;
		}
	}
	else if (_node->get_name() == TXT("onDecompose"))
	{
		bool result = true;
		OnDecompose sod;
		if (auto* attr = _node->get_attribute(TXT("temporaryObject")))
		{
			sod.temporaryObject = attr->get_as_name();
		}
		else if (auto* attr = _node->get_attribute(TXT("temporaryObjectType")))
		{
			result &= sod.temporaryObjectType.load_from_xml(attr, _lc);
		}
		if (result)
		{
			onDecompose.push_back(sod);
			return true;
		}
		else
		{
			error_loading_xml(_node, TXT("no valid thing defined for onDecompose"));
			return false;
		}
	}
	else if (_node->get_name() == TXT("changeAppearanceOnDeath"))
	{
		bool result = true;
		ChangeAppearanceOnDeath cmod;
		cmod.hideAppearance = _node->get_name_attribute(TXT("hideAppearance"), cmod.hideAppearance);
		cmod.showAppearance = _node->get_name_attribute(TXT("showAppearance"), cmod.showAppearance);
		cmod.setMainAppearance = _node->get_name_attribute(TXT("setMainAppearance"), cmod.setMainAppearance);
		cmod.delay = _node->get_float_attribute(TXT("delay"), cmod.delay);
		changeAppearanceOnDeath.push_back(cmod);
		return result;
	}
	else if (_node->get_name() == TXT("dontCeaseOnDeath"))
	{
		ceaseOnDeath = false;
		return true;
	}
	else if (_node->get_name() == TXT("ceaseOnDeath"))
	{
		ceaseOnDeath = _node->get_bool(true);;
		return true;
	}
	else if (_node->get_name() == TXT("ceaseOnDeathDelay"))
	{
		ceaseOnDeathDelay = _node->get_float(ceaseOnDeathDelay);
		return true;
	}
	else if (_node->get_name() == TXT("disableMindOnDeath"))
	{
		disableMindOnDeath = _node->get_bool(true);
		return true;
	}
	else if (_node->get_name() == TXT("soundsOnDamage"))
	{
		bool result = true;
		for_every(node, _node->children_named(TXT("play")))
		{
			SoundOnDamage sod;
			sod.damage.load_from_xml(node, TXT("onDamage"));
			sod.sound = node->get_name_attribute(TXT("sound"));
			error_loading_xml_on_assert(sod.sound.is_valid(), result, _node, TXT("no sound for sounds on damage"));
			soundsOnDamage.push_back(sod);
		}
		return result;
	}
	else if (_node->get_name() == TXT("soundsOnDamageControlledByPlayer"))
	{
		bool result = true;
		for_every(node, _node->children_named(TXT("play")))
		{
			SoundOnDamage sod;
			sod.damage.load_from_xml(node, TXT("onDamage"));
			sod.sound = node->get_name_attribute(TXT("sound"));
			error_loading_xml_on_assert(sod.sound.is_valid(), result, _node, TXT("no sound for sounds on damage"));
			soundsOnDamageControlledByPlayer.push_back(sod);
		}
		return result;
	}
	else
	{
		return base::read_parameter_from(_node, _lc);
	}
}

bool HealthData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(sod, onDeath)
	{
		result &= sod->temporaryObjectType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	for_every(sod, onDecompose)
	{
		result &= sod->temporaryObjectType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	IF_PREPARE_LEVEL(_pfgContext, Framework::LibraryPrepareLevel::Resolve)
	{
		for_every(idrs, includeDamageRuleSets)
		{
			if (idrs->find(_library))
			{
				if (auto* drs = idrs->get())
				{
					for_every(dr, drs->get_rules())
					{
						bool damageRuleExists = false;
						for_every(edr, damageRules)
						{
							if (edr->id == dr->id)
							{
								*edr = *dr;
								warn_dev_ignore(TXT("overwriting damage rule"));
								damageRuleExists = true;
								break;
							}
						}
						if (!damageRuleExists)
						{
							damageRules.push_back(*dr);
						}
					}
				}
			}
		}
		DamageRule::set_loading_order(damageRules);
		sort(damageRules);
	}

	for_every(dr, damageRules)
	{
		result &= dr->spawnTemporaryObjectType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
		for_every(ee, dr->extraEffects)
		{
			result &= ee->temporaryObjectType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
		}
	}

	return result;
}

//

bool HealthData::Rule::load_from_xml(IO::XML::Node const* _node, HealthData const* _for)
{
	bool result = true;

	id = _node->get_name_attribute_or_from_child(TXT("id"), id);
	hitBone = _node->get_name_attribute_or_from_child(TXT("hitBone"), hitBone);
	hitSocketInvestigateInfo = _node->get_name_attribute_or_from_child(TXT("hitSocketInvestigateInfo"), hitSocketInvestigateInfo);
	includeChildrenBones = _node->get_bool_attribute_or_from_child_presence(TXT("includeChildrenBones"), includeChildrenBones);
	includeChildrenBones = ! _node->get_bool_attribute_or_from_child_presence(TXT("dontIncludeChildrenBones"), ! includeChildrenBones);
	health = Energy::load_from_attribute_or_from_child(_node, TXT("health"), health);
	activeIfDepleted = _node->get_bool_attribute_or_from_child_presence(TXT("activeIfDepleted"), activeIfDepleted);
	applyDamageToMainHealthCoef = EnergyCoef::load_from_attribute_or_from_child(_node, TXT("applyDamageToMainHealthCoef"), applyDamageToMainHealthCoef);
	requiredVariables.load_from_xml_child_node(_node, TXT("requiredVariables"));
	requiredVariables.load_from_xml_child_node(_node, TXT("requiresVariables"));

	for_every(node, _node->children_named(TXT("onDeplete")))
	{
		setVariablesOnDeplete.load_from_xml_child_node(node, TXT("setVariables"));
		setAppearanceControllerVariablesOnDeplete.load_from_xml_child_node(node, TXT("setAppearanceControllerVariables"));
		for_every(spawnNode, node->children_named(TXT("spawn")))
		{
			Name to = spawnNode->get_name_attribute(TXT("temporaryObject"));
			if (to.is_valid())
			{
				temporaryObjectsOnDeplete.push_back(to);
			}
			else
			{
				error_loading_xml(spawnNode, TXT("no info what to spawn"));
				result = false;
			}
		}
		for_every(playNode, node->children_named(TXT("play")))
		{
			Name sound = playNode->get_name_attribute(TXT("sound"));
			if (sound.is_valid())
			{
				soundsOnDeplete.push_back(sound);
			}
			else
			{
				error_loading_xml(playNode, TXT("no info what to play"));
				result = false;
			}
		}
		allowDamageToGoThroughOnDeplete = node->get_bool_attribute_or_from_child_presence(TXT("allowDamageToGoThrough"), allowDamageToGoThroughOnDeplete);
		extraDamageOnDeplete = Energy::load_from_attribute_or_from_child(node, TXT("extraDamage"), extraDamageOnDeplete);
		for_every(extraDamageNode, node->children_named(TXT("extraDamage")))
		{
			extraDamageCanKill = extraDamageNode->get_bool_attribute(TXT("canKill"), extraDamageCanKill);
		}
		extraDamageCanKill = node->get_bool_attribute_or_from_child_presence(TXT("extraDamageCanKill"), extraDamageCanKill);
		allowDamageToGoThroughOnDeplete = node->get_bool_attribute_or_from_child_presence(TXT("allowDamageToGoThrough"), allowDamageToGoThroughOnDeplete);
		dropHealthToOnDeplete.load_from_xml(node, TXT("dropHealthTo"));
		applyVelocityOnDeplete.load_from_xml_child_node(node, TXT("applyVelocity"));
		applySpeedOnDeplete.load_from_xml_child_node(node, TXT("applyVelocity"), TXT("speed"));
		autoDeathTime.load_from_xml(node, TXT("autoDeathTime"));
	}

	return result;
}
