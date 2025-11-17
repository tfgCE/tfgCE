#pragma once

#include "..\..\core\types\name.h"
#include "..\..\core\containers\map.h"
#include "..\..\core\concurrency\spinLock.h"
#include "..\..\core\concurrency\scopedSpinLock.h"

namespace Framework
{
	class TweakingContext
	{
	public:
		static TweakingContext* get() { return s_tweakingContext; }

		static void initialise_static();
		static void close_static();

		static void set(Name const & _name, void* _data) { Concurrency::ScopedSpinLock lock(get()->lock); get()->context[_name] = _data; }
		static void* get(Name const & _name) { Concurrency::ScopedSpinLock lock(get()->lock); if (void** found = get()->context.get_existing(_name)) { return *found; } else { return nullptr; } }

	private:
		static TweakingContext* s_tweakingContext;

		Concurrency::SpinLock lock = Concurrency::SpinLock(TXT("Framework.TweakingContext.lock"));
		Map<Name, void*> context;
	};

};