#include "pilgrimageInstanceObserver.h"

#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\concurrency\spinLock.h"

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(IPilgrimageInstanceObserver);

Concurrency::SpinLock pio_lock;

IPilgrimageInstanceObserver* IPilgrimageInstanceObserver::s_pio_first = nullptr;

void IPilgrimageInstanceObserver::pio_add()
{
	Concurrency::ScopedSpinLock lock(pio_lock);

	auto* pio = s_pio_first;
	while (pio)
	{
		if (pio == this)
		{
			return;
		}

		pio = pio->pio_next;
	}

	pio_next = s_pio_first;
	if (s_pio_first)
	{
		s_pio_first->pio_prev = this;
	}
	s_pio_first = this;
}

void IPilgrimageInstanceObserver::pio_remove()
{
	Concurrency::ScopedSpinLock lock(pio_lock);

	if (pio_prev)
	{
		pio_prev->pio_next = pio_next;
	}
	if (pio_next)
	{
		pio_next->pio_prev = pio_prev;
	}

	if (s_pio_first == this)
	{
		s_pio_first = pio_next;
	}

	pio_prev = nullptr;
	pio_next = nullptr;
}

void IPilgrimageInstanceObserver::pilgrimage_instance_switch(PilgrimageInstance const* _from, PilgrimageInstance const* _to)
{
	auto* pio = s_pio_first;
	while (pio)
	{
		auto* next = pio->pio_next;
		pio->on_pilgrimage_instance_switch(_from, _to); // may remove itself (and only self)
		pio = next;
	}
}

void IPilgrimageInstanceObserver::pilgrimage_instance_end(PilgrimageInstance const* _pilgrimage, PilgrimageResult::Type _pilgrimageResult)
{
	auto* pio = s_pio_first;
	while (pio)
	{
		auto* next = pio->pio_next;
		pio->on_pilgrimage_instance_end(_pilgrimage, _pilgrimageResult); // may remove itself (and only self)
		pio = next;
	}
}

