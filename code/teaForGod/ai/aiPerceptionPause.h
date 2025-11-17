#pragma once

#include "..\..\core\containers\arrayStatic.h"
#include "..\..\core\types\name.h"

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		struct PerceptionPause
		{
			static PerceptionPause const & empty() { return s_empty; }

			PerceptionPause();

			bool is_paused() const { return !pauseReasons.is_empty(); }

			void pause(Name const & _reason) { if (! ignoreReasons.does_contain(_reason)) pauseReasons.push_back_unique(_reason); }
			void unpause(Name const & _reason) { pauseReasons.remove_fast(_reason); }
			void set_pause(Name const & _reason, bool _paused) { if (_paused) pause(_reason); else unpause(_reason); }

			void ignore(Name const & _reason, bool _ignore = true) { if (_ignore) ignoreReasons.push_back(_reason); else ignoreReasons.remove_fast(_reason); }

			void log(LogInfoContext & _context) const;

		private:
			static PerceptionPause s_empty;
			ArrayStatic<Name, 16> pauseReasons;

			ArrayStatic<Name, 16> ignoreReasons;
		};
	};
};

DECLARE_REGISTERED_TYPE(TeaForGodEmperor::AI::PerceptionPause);
DECLARE_REGISTERED_TYPE(TeaForGodEmperor::AI::PerceptionPause*);

