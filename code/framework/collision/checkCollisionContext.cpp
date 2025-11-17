#include "checkCollisionContext.h"

#include "..\module\modulePresence.h"

using namespace Framework;

CheckCollisionContext::CheckCollisionContext()
{
}

void CheckCollisionContext::avoid(IModulesOwner* imo, bool _andAttached)
{
	avoid(fast_cast<Collision::ICollidableObject>(imo));
	if (_andAttached)
	{
		Concurrency::ScopedSpinLock lock(imo->get_presence()->access_attached_lock());
		for_every_ptr(at, imo->get_presence()->get_attached())
		{
			avoid(at, _andAttached);
		}
	}
}

void CheckCollisionContext::avoid_up_to(IModulesOwner* imo, IModulesOwner* upTo)
{
	while (imo)
	{
		avoid(fast_cast<Collision::ICollidableObject>(imo));
		if (imo == upTo)
		{
			break;
		}
		imo = imo->get_presence()->get_attached_to();
	}
}

void CheckCollisionContext::avoid_up_to_top_instigator(IModulesOwner* imo)
{
	while (imo)
	{
		avoid(fast_cast<Collision::ICollidableObject>(imo));
		imo = imo->get_instigator(true);
	}
}
