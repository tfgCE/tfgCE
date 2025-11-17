#include "moduleRareAdvance.h"

#include "module.h"

#include "..\framework.h"

#ifdef AN_ALLOW_BULLSHOTS
#include "..\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

void ModuleRareAdvance::setup_with(Module * _module, ModuleData const* _data, Name const& _rareAdvanceParam)
{
	interval = _data->get_parameter<Range>(_module, _rareAdvanceParam, interval);
	currentInterval = 0.0f;
	accumulatedDeltaTime = 0.0f;
}

void ModuleRareAdvance::reset_to_no_rare_advancement()
{
	interval = Range::empty;
	overrideInterval.clear();
	currentInterval = 0.0f;
	longWait = 0.0f;
}

void ModuleRareAdvance::reset()
{
	Range useInterval = get_used_interval();
	if (useInterval.is_empty() || useInterval.max <= 0.0f)
	{
		currentInterval = 0.0f;
	}
	else
	{
		currentInterval = max(0.0f, Random::get(useInterval));
	}
	accumulatedDeltaTime = 0.0f;
}

bool ModuleRareAdvance::update(float _deltaTime)
{
#ifdef AN_NO_RARE_ADVANCE_AVAILABLE
	if (Framework::access_no_rare_advance()
		|| noRareForced
#ifdef AN_ALLOW_BULLSHOTS
		|| Framework::BullshotSystem::is_active()
#endif
		)
	{
		accumulatedLongWaitDeltaTime = 0.0f;
		accumulatedDeltaTime = 0.0f;
		useDeltaTime = _deltaTime;
		advanceNow = true;
		return true;
	}
#endif

	if (longWait > 0.0f)
	{
		longWait -= _deltaTime;
		if (longWait > 0.0f)
		{
			accumulatedLongWaitDeltaTime += _deltaTime;
			advanceNow = false;
			return false;
		}
		// do not modify delta time, we could do the whole wait in one step, so it's like there was no wait
		//_deltaTime += longWait; // make the delta time smaller by amount we went negative
	}
	if (accumulatedLongWaitDeltaTime > 0.0f)
	{
		// use long wait to advance, how long were we there etc, we want to see what happened in the mean time
		useDeltaTime = accumulatedLongWaitDeltaTime + _deltaTime;
		accumulatedLongWaitDeltaTime = 0.0f;
		advanceNow = true;
		return true;
	}
	Range useInterval = get_used_interval();
	if (useInterval.is_empty() || useInterval.max <= 0.0f)
	{
		useDeltaTime = _deltaTime;
		advanceNow = true;
		firstAdvance = false;
		return true;
	}
	else
	{
		advanceNow = false;
		bool newInterval = currentInterval == 0.0f;
		accumulatedDeltaTime += _deltaTime;
		if (accumulatedDeltaTime > currentInterval && !firstAdvance)
		{
			useDeltaTime = accumulatedDeltaTime;
			advanceNow = true;
			newInterval = true;
		}

		firstAdvance = false;
		if (newInterval)
		{
			currentInterval = max(0.0f, Random::get(useInterval));
			accumulatedDeltaTime = 0.0f;
		}

		return advanceNow;
	}
}
