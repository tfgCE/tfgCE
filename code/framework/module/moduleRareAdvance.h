#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\types\optional.h"

namespace Framework
{
	class Module;
	class ModuleData;

	struct ModuleRareAdvance
	{
		void force_no_rare() { noRareForced = true; } // this can't be reset, used by very specific items (game script executors)
		void clear_no_rare() { noRareForced = false; }

		void override_interval(Optional<Range> const& _interval) { overrideInterval = _interval; }
		Range get_used_interval() const { return overrideInterval.get(interval); }
		void setup_with(Module * _module, ModuleData const* _data, Name const& _rareAdvanceParam);
		bool update(float _deltaTime); // returns true if should be advanced
		void reset(); // won't clear no rare

		void reset_to_no_rare_advancement(); // won't force no rare

		bool should_advance() const { return advanceNow; }
		float get_advance_delta_time() const { return useDeltaTime; }

		void set_long_wait(float _longWait) { longWait = _longWait; }
		void stop_long_wait() { longWait = 0.0f; }

	public: // for debug
		float get_accumulated_delta_time() const { return accumulatedDeltaTime; }
		float get_current_interval() const { return currentInterval; }
	private:
		bool noRareForced = false;

		Range interval = Range::empty;
		Optional<Range> overrideInterval;
		float currentInterval = 0.0f;
		float accumulatedDeltaTime = 0.0f;
		float accumulatedLongWaitDeltaTime = 0.0f;
		bool firstAdvance = true;

		float useDeltaTime = 0.0f;
		bool advanceNow = true;

		float longWait = 0.0f; // if we force longer wait (AI does so)
	};

};
