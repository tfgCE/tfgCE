#include "aiManagerLogic_regionManager.h"

#include "..\..\..\game\gameDirector.h"
#include "..\..\..\game\gameSettings.h"
#include "..\..\..\game\missionState.h"

#include "..\..\..\library\missionDifficultyModifier.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\module\moduleAI.h"
#include "..\..\..\..\framework\world\region.h"
#include "..\..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_ALLOW_EXTENSIVE_LOGS
//#define AN_OUTPUT_INFO
#endif
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Managers;

//

// parameters
DEFINE_STATIC_NAME(inRegion);
DEFINE_STATIC_NAME(spawnManagerValue);

//

MissionDifficultyModifier::ForPool const* get_mission_difficulty_modifier_for_pool(Name const& _pool)
{
	if (auto* ms = MissionState::get_current())
	{
		if (auto* mdm = ms->get_difficulty_modifier())
		{
			return mdm->get_for_pool(_pool);
		}
	}

	return nullptr;
}

//--

REGISTER_FOR_FAST_CAST(RegionManager);

RegionManager::RegionManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
, SafeObject<RegionManager>(this)
, regionManagerData(fast_cast<RegionManagerData>(_logicData))
{
	random = _mind->get_owner_as_modules_owner()->get_individual_random_generator();
	random.advance_seed(8524, 2975792);
}

RegionManager::~RegionManager()
{
	make_safe_object_unavailable();
}

Energy RegionManager::get_value_for(Framework::IModulesOwner* imo)
{
	return imo->get_variables().get_value<Energy>(NAME(spawnManagerValue), Energy::one());
}

RegionManager* RegionManager::find_for(Framework::Room* r)
{
	return r ? find_for(r->get_in_region()) : nullptr;
}

RegionManager* RegionManager::find_for(Framework::Region* r)
{
	while (r)
	{
		RegionManager* result = nullptr;
		r->for_every_ai_manager([&result](Framework::IModulesOwner* imo)
			{
				if (!result)
				{
					if (auto* ai = imo->get_ai())
					{
						if (auto* mind = ai->get_mind())
						{
							if (auto* rm = fast_cast<RegionManager>(mind->get_logic()))
							{
								result = rm;
							}
						}
					}
				}
			});
		if (result)
		{
			return result;
		}

		r = r->get_in_region();
	}

	return nullptr;
}

void RegionManager::init()
{
	random = get_imo()->get_individual_random_generator();
	random.advance_seed(8524, 2975792);

	pools.clear();
	for_every(pd, regionManagerData->pools)
	{
		Pool p;
		p.name = pd->name;
		p.concurrentLimitCoef = pd->concurrentLimitCoef;
		p.active = true;
		p.hostile = pd->hostile;
		p.infinite = pd->infinite;
		p.limit = pd->amount.get_random(random).mul_with_min(GameSettings::get().difficulty.spawnCountModifier, pd->lowestAmountForCoefs.get(Energy::one()));
		if (auto* fp = get_mission_difficulty_modifier_for_pool(p.name))
		{
			p.limit = p.limit.mul(fp->amountCoef.get(1.0f));
			p.limit += fp->amountAdd.get(Energy::zero());
		}
		p.lowerLimitWhenDeactivated = pd->lowerAmountWhenDeactivated.get_random(random); // this is by what we lower the value
		p.regenMissingBit = 0.0f;
		p.regenPerMinute = pd->regenPerMinute.get_random(random);
		p.regenPerMinuteInactive = pd->regenPerMinuteInactive.get_random(random);
		p.stopAt = pd->stopAt.get_random(random);
		p.minToRestart = pd->minToRestart.get_random(random).mul(GameSettings::get().difficulty.spawnCountModifier);
		p.noRestart = pd->noRestart;
		//
		p.stopAt = max(Energy::zero(), p.stopAt);
		if (!p.noRestart)
		{
			p.minToRestart = max(p.stopAt, p.minToRestart);
			p.limit = max(p.minToRestart, p.limit);
		}
		p.limitFull = p.limit;
		//
		p.available = p.limit;
		//
		pools.push_back(p);
	}
}

void RegionManager::log_logic_info(LogInfoContext& _log) const
{
	base::log_logic_info(_log);

	if (!pools.is_empty())
	{
		_log.log(TXT("POOLS"));
		LOG_INDENT(_log);
		for_every(pool, pools)
		{
			_log.log(TXT("pool %S"), pool->name.to_char());
			LOG_INDENT(_log);

			Optional<Colour> defColour;
			if (!pool->active) defColour = Colour::grey;
			_log.set_colour(defColour);
			_log.log(pool->active? TXT("[ACTIVE]") : TXT("[inactive]"));
			if (pool->hostile) _log.set_colour(defColour.get(Colour::red));
			_log.log(pool->hostile? TXT("[HOSTILE]") : TXT("[not hostile]"));
			_log.set_colour(defColour);
			_log.log(TXT("conc. limit x %6.2f%%"), pool->concurrentLimitCoef.get(1.0f) * 100.0f);
			_log.log(TXT("available     %6.2f SV"), pool->available.as_float());
			if (pool->infinite) _log.set_colour(defColour.get(Colour::cyan));
			_log.log(TXT("  limit       %6.2f SV%S"), pool->limit.as_float(), pool->infinite? TXT(" [INFINITE]") : TXT(""));
			_log.set_colour(defColour);
			if (!pool->lowerLimitWhenDeactivated.is_zero())
			{
				_log.log(TXT("  lower (dac) %6.2f SV"), pool->lowerLimitWhenDeactivated.as_float());
			}
			_log.log(TXT("regen rate    %6.2f SV/minute"), pool->active? pool->regenPerMinute.as_float() : pool->regenPerMinuteInactive.as_float());
			_log.log(TXT("stop at       %6.2f SV"), pool->stopAt.as_float());
			_log.log(TXT("%S"), pool->noRestart? TXT("no restart") : TXT("restart available"));
			if (!pool->noRestart)
			{
				_log.log(TXT("restart req   %6.2f SV"), pool->minToRestart.as_float());
			}

			_log.log(TXT(""));
			_log.set_colour();
		}
	}
}

void RegionManager::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (started)
	{
		bool wereHostileSpawnsBlockedByGameDirector = hostileSpawnsBlockedByGameDirector;
		hostileSpawnsBlockedByGameDirector = false;
		bool forceHostileSpawns = false;
		if (auto* gd = GameDirector::get_active())
		{
			if (gd->are_hostile_spawns_forced())
			{
				forceHostileSpawns = true;
			}
			if (gd->should_block_hostile_spawns())
			{
				hostileSpawnsBlockedByGameDirector = true;
			}
		}

		Concurrency::ScopedMRSWLockWrite lock(poolLock);
		for_every(p, pools)
		{
			if (p->hostile && forceHostileSpawns)
			{
				if (!p->noRestart)
				{
					p->limit = max(p->limit, p->minToRestart);
				}
				if (p->infinite)
				{
					p->limit = p->limitFull;
				}
				p->available = p->limit; // force it until we say stop
			}
			if (p->hostile && !hostileSpawnsBlockedByGameDirector && wereHostileSpawnsBlockedByGameDirector)
			{
				int a = 1;
				int l = 3;
				p->available = (p->available * a + p->limit * l) / (a + l); // get closer
			}
			if (p->infinite)
			{
				p->limit = p->limitFull;
				p->available = p->limit;
			}
			else if (p->available < p->limit)
			{
				Energy regenerated = (p->active ? p->regenPerMinute : p->regenPerMinuteInactive).timed(_deltaTime / 60.0f, p->regenMissingBit);
				p->available += regenerated;
			}
			p->available = clamp(p->available, Energy::zero(), p->limit); // to limit when restored
			bool thisOneBlocked = p->hostile && hostileSpawnsBlockedByGameDirector;
			bool newActive = p->active && !thisOneBlocked;
			if (!thisOneBlocked)
			{
				if (p->infinite)
				{
					newActive = true;
				}
				else if (p->noRestart)
				{
					if (!p->active && p->available > p->stopAt)
					{
						newActive = true;
					}
					if (p->active && p->available <= p->stopAt)
					{
						newActive = false;
					}
				}
				else
				{
					if (!p->active && p->available >= p->minToRestart && p->available >= p->limit && p->available.is_positive()) // has to reach limit but may restart only if is above "min to restart"
					{
						newActive = true;
					}
					if (p->active && p->available < p->minToRestart && p->available <= p->stopAt)
					{
						newActive = false;
					}
				}

				if (p->active && !newActive && ! p->infinite && p->limit.is_positive())
				{
					p->limit -= p->lowerLimitWhenDeactivated;
					p->limit = max(p->limit, Energy::zero());
				}
			}
#ifdef AN_OUTPUT_INFO
			if (p->active != newActive)
			{
				if (newActive)
				{
					output(TXT("[region manager] activated %S"), p->name.to_char());
				}
				else
				{
					output(TXT("[region manager] deactivated %S"), p->name.to_char());
				}
			}
#endif
			p->active = newActive;
		}
	}
}

int RegionManager::get_altered_concurrent_limit(Name const& _pool, int _concurrentLimit) const
{
	float coef = get_concurrent_limit_coef(_pool);
	if (coef != 1.0f)
	{
		return max(1, TypeConversions::Normal::f_i_closest((float)_concurrentLimit * coef));
	}
	else
	{
		return _concurrentLimit;
	}
}

float RegionManager::get_concurrent_limit_coef(Name const& _pool) const
{
	Concurrency::ScopedMRSWLockRead lock(poolLock);
	float alterCoef = 1.0f;
	if (auto* fp = get_mission_difficulty_modifier_for_pool(_pool))
	{
		alterCoef *= fp->concurrentLimitCoef.get(1.0f);
	}
	for_every(p, pools)
	{
		if (p->name == _pool)
		{
			return alterCoef * p->concurrentLimitCoef.get(1.0f);
		}
	}
	return alterCoef * 1.0f;
}

bool RegionManager::is_pool_hostile(Name const& _pool) const
{
	Concurrency::ScopedMRSWLockRead lock(poolLock);
	for_every(p, pools)
	{
		if (p->name == _pool)
		{
			return p->hostile;
		}
	}
	return false;
}

bool RegionManager::is_pool_active(Name const& _pool) const
{
	Concurrency::ScopedMRSWLockRead lock(poolLock);
	for_every(p, pools)
	{
		if (p->name == _pool)
		{
			return p->active;
		}
	}
	return false;
}

Energy RegionManager::get_available_for_pool(Name const& _pool) const
{
	Concurrency::ScopedMRSWLockRead lock(poolLock);
	for_every(p, pools)
	{
		if (p->name == _pool)
		{
			return p->active? p->available.mul(GameSettings::get().difficulty.spawnCountModifier) : Energy::zero();
		}
	}
	return Energy::zero();
}

Energy RegionManager::get_pool_value(Name const& _pool) const
{
	Concurrency::ScopedMRSWLockRead lock(poolLock);
	for_every(p, pools)
	{
		if (p->name == _pool)
		{
			return p->available.mul(GameSettings::get().difficulty.spawnCountModifier);
		}
	}
	return Energy::zero();
}

Energy RegionManager::get_lost_pool_value(Name const& _pool) const
{
	Concurrency::ScopedMRSWLockRead lock(poolLock);
	for_every(p, pools)
	{
		if (p->name == _pool)
		{
			return p->lost.mul(GameSettings::get().difficulty.spawnCountModifier);
		}
	}
	return Energy::zero();
}

void RegionManager::consume_for_pool(Name const& _pool, Energy const& _combatValue)
{
	Concurrency::ScopedMRSWLockWrite lock(poolLock);
	for_every(p, pools)
	{
		if (p->name == _pool)
		{
			p->available -= _combatValue.div_safe(GameSettings::get().difficulty.spawnCountModifier);
			// update active only in advance
#ifdef AN_OUTPUT_INFO
			output(TXT("[region manager] consumed %.2f->%.2f for %S"), _combatValue.as_float(), p->available.as_float(), p->name.to_char());
#endif
		}
	}
}

void RegionManager::restore_for_pool(Name const& _pool, Energy const& _combatValue)
{
	Concurrency::ScopedMRSWLockWrite lock(poolLock);
	for_every(p, pools)
	{
		if (p->name == _pool)
		{
			p->available += _combatValue.div_safe(GameSettings::get().difficulty.spawnCountModifier);
			// update active only in advance
#ifdef AN_OUTPUT_INFO
			output(TXT("[region manager] restored %.2f->%.2f for %S"), _combatValue.as_float(), p->available.as_float(), p->name.to_char());
#endif
		}
	}
}

void RegionManager::alter_limit_for_pool(Name const& _pool, int& _limit) const
{
	if (_limit > 0)
	{
		Concurrency::ScopedMRSWLockRead lock(poolLock);
		for_every(p, pools)
		{
			if (p->name == _pool &&
				! p->infinite)
			{
				_limit = p->available.is_positive() && p->limitFull.is_positive()? TypeConversions::Normal::f_i_closest((float)_limit * (0.4f + 0.6f * clamp(p->available.div_to_float(max(Energy::one(), p->limitFull)), 0.0f, 1.0f))) : _limit;
				_limit = max(1, _limit);
			}
		}
	}
}

LATENT_FUNCTION(RegionManager::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto

	LATENT_END_PARAMS();

	auto * self = fast_cast<RegionManager>(logic);
	auto * imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	// get information where it does work
	self->inRegion = imo->get_variables().get_value<::SafePtr<Framework::Region>>(NAME(inRegion), ::SafePtr<Framework::Region>());

	{	// register ai manager
		if (auto* r = self->inRegion.get())
		{
			self->register_ai_manager_in(r);
		}
	}

	self->init();

	self->started = true;

	while (true)
	{
		LATENT_WAIT(Random::get_float(0.1f, 0.5f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	self->unregister_ai_manager();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(RegionManagerData);

RegionManagerData::RegionManagerData()
{
}

RegionManagerData::~RegionManagerData()
{
}

bool RegionManagerData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("pool")))
	{
		Pool p;
		p.name = node->get_name_attribute_or_from_child(TXT("name"), p.name);
		error_loading_xml_on_assert(p.name.is_valid(), result, node, TXT("no \"name\" provided"));
		
		p.hostile = node->get_bool_attribute_or_from_child_presence(TXT("hostile"), p.hostile);
		p.infinite = node->get_bool_attribute_or_from_child_presence(TXT("infinite"), p.infinite);
		p.concurrentLimitCoef.load_from_xml(node, TXT("concurrentLimitCoef"));
		p.amount.load_from_xml(node, TXT("amount"));
		p.lowestAmountForCoefs.load_from_xml(node, TXT("lowestAmountForCoefs"));
		p.lowerAmountWhenDeactivated.load_from_xml(node, TXT("lowerAmountWhenDeactivated"));
		p.regenPerMinute.load_from_xml(node, TXT("regenPerMinute"));
		p.regenPerMinuteInactive.load_from_xml(node, TXT("regenPerMinuteInactive"));
		p.stopAt.load_from_xml(node, TXT("stopAt"));
		p.minToRestart.load_from_xml(node, TXT("minToRestart"));
		p.noRestart = node->get_bool_attribute_or_from_child_presence(TXT("noRestart"), p.noRestart);
		p.noRestart = ! node->get_bool_attribute_or_from_child_presence(TXT("restart"), ! p.noRestart);

		pools.push_back(p);
	}

	return result;
}

bool RegionManagerData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
