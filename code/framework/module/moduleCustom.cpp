#include "moduleCustom.h"

#include "..\advance\advanceContext.h"

#include "..\..\core\concurrency\scopedSpinLock.h"

#include "..\module\modulePresence.h"
#include "..\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(ModuleCustom);

ModuleCustom::ModuleCustom(IModulesOwner* _owner)
: Module(_owner)
{
}

void ModuleCustom::mark_requires(Concurrency::ImmediateJobFunc _jobFunc, int _flags)
{
	Concurrency::ScopedSpinLock lock(requiresLock);
	for_every(r, requires)
	{
		if (r->func == _jobFunc)
		{
			set_flag(r->flags, _flags);
			return;
		}
	}
	Requires r;
	r.func = _jobFunc;
	set_flag(r.flags, _flags);
	requires.push_back(r);
}

void ModuleCustom::mark_no_longer_requires(Concurrency::ImmediateJobFunc _jobFunc, int _flags)
{
	Concurrency::ScopedSpinLock lock(requiresLock);
	for_every(r, requires)
	{
		if (r->func == _jobFunc)
		{
			clear_flag(r->flags, _flags);
			if (!is_any_flag_set(r->flags))
			{
				requires.remove_at(for_everys_index(r));
				return;
			}
		}
	}
}

void ModuleCustom::all_customs__advance_post(IMMEDIATE_JOB_PARAMS)
{
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		for_every_ptr(module, modulesOwner->get_customs())
		{
			if (module->does_require(all_customs__advance_post) &&
				module->raAdvancePost.should_advance())
			{
				module->advance_post(module->raAdvancePost.get_advance_delta_time());
			}
		}
	}
}

void ModuleCustom::all_customs__update_rare_advance(IModulesOwner const * _modulesOwner, float _deltaTime)
{
	for_every_ptr(module, _modulesOwner->get_customs())
	{
		module->raAdvancePost.update(_deltaTime);
	}
}

bool ModuleCustom::all_customs__does_require(Concurrency::ImmediateJobFunc _jobFunc, IModulesOwner const * _modulesOwner)
{
	for_every_ptr(module, _modulesOwner->get_customs())
	{
		if (module->does_require(_jobFunc) &&
			module->raAdvancePost.should_advance())
		{
			return true;
		}
	}
	return false;
}

bool ModuleCustom::does_require(Concurrency::ImmediateJobFunc _jobFunc) const
{
	an_assert(!requiresLock.is_locked());
	for_every(r, requires)
	{
		if (r->func == _jobFunc)
		{
			return true;
		}
	}
	return false;
}

void ModuleCustom::rarer_post_advance_if_not_visible()
{
	{
		bool rareAdvance = true;
		if (get_owner()->get_room_distance_to_recently_seen_by_player() <= 1)
		{
			rareAdvance = false;
		}
		if (rareAdvance)
		{
			raAdvancePost.override_interval(Range(0.1f, 0.3f));
		}
		else
		{
			raAdvancePost.override_interval(NP);
		}
	}
}
