#include "iPhysicalSensations.h"

#include "..\concurrency\scopedSpinLock.h"

#include "..\mainConfig.h"

#include "allPhysicalSensations.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace PhysicalSensations;

//

RefCountObjectPtr<IPhysicalSensations> IPhysicalSensations::s_instance;
Sensation::ID IPhysicalSensations::nextSensationID = NONE;
Concurrency::SpinLock IPhysicalSensations::nextSenstationIDLock = Concurrency::SpinLock(TXT("PhysicalSensations.IPhysicalSensations.nextSenstationIDLock"));

REGISTER_FOR_FAST_CAST(IPhysicalSensations);

#define START_IF_SELECTED(What) \
	if (pss == PhysicalSensations::What::system_id() && \
		(!_autoInit || PhysicalSensations::What::may_auto_start())) \
	{ \
		output(TXT("initialise physical sensations system \"%S\""), pss.to_char()); \
		PhysicalSensations::What::start(); \
	}

void IPhysicalSensations::initialise(bool _autoInit)
{
	Name pss = MainConfig::access_global().get_physical_sensations_system();
	output(TXT("requested physical sensations system \"%S\""), pss.to_char());
#ifdef PHYSICAL_SENSATIONS_OWO
	START_IF_SELECTED(OWO);
#endif
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
	START_IF_SELECTED(BhapticsJava);
#endif
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
	START_IF_SELECTED(BhapticsWindows);
#endif
}

void IPhysicalSensations::terminate()
{
	s_instance.clear();
}

void IPhysicalSensations::do_async_init_thread_func(void* _data)
{
	RefCountObjectPtr<IPhysicalSensations> keepInstance;
	keepInstance = s_instance;
	if (auto* ps = keepInstance.get())
	{
		if (!ps->is_ok())
		{
			ps->async_init();
		}
	}
}

IPhysicalSensations::IPhysicalSensations()
{
	s_instance = this;
}

IPhysicalSensations::~IPhysicalSensations()
{
}

Sensation::ID IPhysicalSensations::start_sensation(SingleSensation const& _sensation)
{
	Concurrency::ScopedSpinLock lock(sensationsLock, true);

	Sensation::ID id = get_next_sensation_id();

	auto os = _sensation;
	os.id = id;

#ifdef LOG_PHYSICAL_SENSATIONS
	output(TXT("start single sensation (%i) \"%S\"%S"), id, SingleSensation::to_char(_sensation.type),
		_sensation.head.get(false) ? TXT(" @ head")
								   : (_sensation.hand.is_set() ? (_sensation.hand.get() == Hand::Left? TXT(" @ left hand") : TXT(" @ right hand"))
															   : TXT("--")));
	if (_sensation.eyeHeightOS.is_set())
	{
		output(TXT("   eye height OS %.3f"), _sensation.eyeHeightOS.get());
	}
	if (_sensation.hitLocOS.is_set())
	{
		output(TXT("   hit loc OS %S"), _sensation.hitLocOS.get().to_string().to_char());
	}
	if (_sensation.dirOS.is_set())
	{
		output(TXT("   dir OS %S"), _sensation.dirOS.get().to_string().to_char());
	}
#endif

	internal_start_sensation(os);

	return os.id;
}

Sensation::ID IPhysicalSensations::start_sensation(OngoingSensation const& _sensation)
{
	Concurrency::ScopedSpinLock lock(sensationsLock, true);

	Sensation::ID id = get_next_sensation_id();

	auto os = _sensation;
	os.id = id;

#ifdef LOG_PHYSICAL_SENSATIONS
	output(TXT("start ongoing sensation (%i) %S"), id, OngoingSensation::to_char(_sensation.type));
#endif

	internal_start_sensation(os);

	ongoingSensations.push_back(os);

	return os.id;
}

void IPhysicalSensations::stop_sensation(Sensation::ID _id)
{
	Concurrency::ScopedSpinLock lock(sensationsLock, true);

#ifdef LOG_PHYSICAL_SENSATIONS
	output(TXT("stop ongoing sensation (%i)"), _id);
#endif

	internal_stop_sensation(_id);

	for_every(s, ongoingSensations)
	{
		if (s->id == _id)
		{
			ongoingSensations.remove_at(for_everys_index(s));
			break;
		}
	}
}

void IPhysicalSensations::stop_all_sensations()
{
	Concurrency::ScopedSpinLock lock(sensationsLock, true);

#ifdef LOG_PHYSICAL_SENSATIONS
	output(TXT("stop all sensations"));
#endif

	if (!internal_stop_all_sensations())
	{
		for_every(s, ongoingSensations)
		{
			internal_stop_sensation(s->id);
		}
	}

	ongoingSensations.clear();
}

void IPhysicalSensations::advance(float _deltaTime)
{
	Concurrency::ScopedSpinLock lock(sensationsLock, true);

	Optional<Sensation::ID> stopSensation;
	for_every(og, ongoingSensations)
	{
		if (og->timeLeft.is_set())
		{
			og->timeLeft = og->timeLeft.get() - _deltaTime;
			if (og->timeLeft.get() <= 0.0f)
			{
				stopSensation.set_if_not_set(og->id);
			}
		}
		if (og->sustainable)
		{
			Concurrency::ScopedSpinLock lock(sensationsLock, true);
			internal_sustain_sensation(og->id);
		}
	}
	if (stopSensation.is_set())
	{
		stop_sensation(stopSensation.get());
	}
}

Sensation::ID IPhysicalSensations::get_next_sensation_id()
{
	Concurrency::ScopedSpinLock lock(sensationsLock, true);
	Concurrency::ScopedSpinLock idLock(nextSenstationIDLock);
	while (true)
	{
		if (nextSensationID == NONE)
		{
			++nextSensationID;
		}

		bool exists = false;
		for_every(s, ongoingSensations)
		{
			if (s->id == nextSensationID)
			{
				exists = true;
				break;
			}
		}

		if (!exists)
		{
			break;
		}
	}

	int result = nextSensationID;
	++nextSensationID;

	return result;
}
