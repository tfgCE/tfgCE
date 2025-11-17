#include "poiManager.h"

#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\performance\performanceUtils.h"

//

using namespace Framework;

//

void POIManager::reset()
{
	entries.clear();
}

void POIManager::update(float _deltaTime)
{
	MEASURE_PERFORMANCE(poiManager);

	// shouldn't be called when scripts and other users are using it
	for_every(e, entries)
	{
		if (e->timeLeft.is_set())
		{
			e->timeLeft = e->timeLeft.get() - _deltaTime;
			if (e->timeLeft.get() <= 0.0f)
			{
				e->poi.clear();
			}
		}
	}

	if (!entries.is_empty())
	{
		if (!entries.get_last().poi.is_set())
		{
			entries.pop_back();
		}
	}
}

bool POIManager::check(Framework::PointOfInterestInstance const * _poi, Name const& _usage)
{
	int count = entries.get_size();

	for_count(int, i, count)
	{
		auto const& e = entries[i];
		if (e.usage == _usage &&
			e.poi == _poi)
		{
			return false;
		}
	}

	return true;
}

bool POIManager::check_and_occupy(Framework::PointOfInterestInstance const * _poi, Name const& _usage, Optional<float> const& _forTime)
{
	if (!check(_poi, _usage))
	{
		return false;
	}

	Concurrency::ScopedSpinLock lock(modifyLock);

	// make sure it wasn't occupied
	for_every(e, entries)
	{
		if (e->usage == _usage &&
			e->poi == _poi)
		{
			return false;
		}
	}

	// find a spot
	for_every(e, entries)
	{
		if (! e->poi.get())
		{
			e->usage = _usage;
			e->poi = _poi;
			e->timeLeft = _forTime;
			return true;
		}
	}

	if (entries.has_place_left())
	{
		entries.push_back(Entry());
		auto& e = entries.get_last();
		e.usage = _usage;
		e.poi = _poi;
		e.timeLeft = _forTime;
		return true;
	}
	else
	{
		// no free spot
		return false;
	}
}
