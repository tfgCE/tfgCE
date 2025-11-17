#pragma once

#include "..\..\core\containers\arrayStatic.h"
#include "..\..\core\types\optional.h"

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		struct PerceptionLock
		{
			static PerceptionLock const & empty() { return s_empty; }

			enum Type
			{
				LookForNew, // always look for new
				KeepButAllowToChange,
				Lock,

				Min = LookForNew,
				Default = KeepButAllowToChange
			};

			PerceptionLock();

			tchar const* to_char(Type _type) const;

			Type get() const;
			bool is_locked() const { return get() == Lock; }

			void set_base_lock(Type _lock);
			void override_lock(Name const & _reason, Optional<Type> const & _lock = NP);
			bool is_overriden_lock_set(Name const & _reason) const;

			void log(LogInfoContext & _context) const;

		private:
			static PerceptionLock s_empty;
			Type lock = Default;
			struct LockOverride
			{
				Name reason;
				Type lock;
			};
			ArrayStatic<LockOverride, 8> lockOverrides;
		};
	};
};

DECLARE_REGISTERED_TYPE(TeaForGodEmperor::AI::PerceptionLock);
DECLARE_REGISTERED_TYPE(TeaForGodEmperor::AI::PerceptionLock*);

