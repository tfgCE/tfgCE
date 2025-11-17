#pragma once

#include "latentStackVariables.h"
#include "latentBlock.h"

#include "..\debug\logInfoContext.h"
#include "..\system\timeStamp.h"

struct LogInfoContext;

namespace Latent
{
	struct Scope;

	class Frame
	{
	public:
		Frame();
		~Frame();

		void reset();

		template <typename Class>
		void add_param(Class const & _value);
		void add_params(StackVariablesBase const * _variables);

		Frame & fresh_call();
		Frame & break_now();
		Frame & with_delta_time(float _deltaTime) { deltaTime = _deltaTime; return *this; }
		Frame & end_waiting() { endWaiting = true; return *this; }
		Frame & wants_to_wait_for(float _howLong) { wantsToWaitFor = _howLong; return *this; }

		bool is_breaking() const { return isBreaking; }
		bool should_end_waiting() const { return endWaiting; }
		float how_long_does_want_to_wait() const { return wantsToWaitFor; }

		void log(LogInfoContext & _log) const;

	private: friend struct Scope;
		// execution for one frame
		void begin_execution();
		void end_execution();

		Block & append_block();
		Block & enter_block();
		void leave_block(bool _hasFinished);
		bool is_last_block() const;

		void increase_current_variable() { ++currentVariable; }
		void set_current_variable(int _value) { currentVariable = _value; }
		int get_current_variable() const { return currentVariable; }

		float get_delta_time() const { return deltaTime; }
		void decrease_delta_time(float _by) { deltaTime = max(0.0f, deltaTime - _by); }

		float get_call_time() const { return callTS.get_time_since(); }

	private: friend struct Block;
		StackVariables const & get_stack_variables() const { return stackVariables; }
		StackVariables & access_stack_variables() { return stackVariables; }

	private:
		StackVariables stackVariables;
		Array<Block*> blocks;
		int currentBlock;
		int currentVariable;
		bool isBreaking;
		bool endWaiting = false; // end all waiting functions
		float wantsToWaitFor = 0.0f;
		float deltaTime;
		System::TimeStamp callTS; // to check how long do we run and yield if required

#ifdef AN_INSPECT_LATENT
	public:
		void add_info(tchar const * _info);
		void remove_info_at(int _idx);
		String get_info() const;
		String get_info_without_section() const;

		void set_section_info(tchar const * _info);
		String const & get_section_info() const { return sectionInfo; }

		StackVariables const & get_stack_variables_to_inspect() const { return stackVariables; }

		LogInfoContext & access_log() { return inspectLog; }
		Concurrency::SpinLock & access_log_lock() { return inspectLogLock; }

	private:
		ArrayStatic<String, 64> info;
		String sectionInfo;
		mutable Concurrency::SpinLock inspectLogLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
		mutable Concurrency::SpinLock stackVariablesLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
		LogInfoContext inspectLog;
#endif
	};

	#include "latentFrame.inl"
};
