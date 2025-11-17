#include "physicalSensations.h"

#include "iPhysicalSensations.h"

//

using namespace PhysicalSensations;

//

void PhysicalSensations::initialise(bool _autoInit)
{
	IPhysicalSensations::initialise(_autoInit);
}

void PhysicalSensations::terminate()
{
	IPhysicalSensations::terminate();
}

bool PhysicalSensations::is_active()
{
	return IPhysicalSensations::get() != nullptr;
}

Sensation::ID PhysicalSensations::start_sensation(SingleSensation const& _sensation)
{
	if (auto* ps = IPhysicalSensations::get())
	{
		return ps->start_sensation(_sensation);
	}
	else
	{
		return NONE;
	}
}

Sensation::ID PhysicalSensations::start_sensation(OngoingSensation const& _sensation)
{
	if (auto* ps = IPhysicalSensations::get())
	{
		return ps->start_sensation(_sensation);
	}
	else
	{
		return NONE;
	}
}

void PhysicalSensations::stop_sensation(Sensation::ID _id)
{
	if (auto* ps = IPhysicalSensations::get())
	{
		ps->stop_sensation(_id);
	}
}

void PhysicalSensations::stop_all_sensations()
{
	if (auto* ps = IPhysicalSensations::get())
	{
		ps->stop_all_sensations();
	}
}

void PhysicalSensations::advance(float _deltaTime)
{
	if (auto* ps = IPhysicalSensations::get())
	{
		ps->advance(_deltaTime);
	}
}
