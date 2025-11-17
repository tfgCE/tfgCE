#pragma once

#include "..\..\core\memory\safeObject.h"
#include "..\..\core\other\simpleVariableStorage.h"
#include "..\..\core\types\optional.h"
#include "..\..\core\types\string.h"

namespace Framework
{
	namespace GameScript
	{
		class Script;

		namespace ScriptExecutionResult
		{
			enum Type
			{
				Continue,
				Yield, // will stop execution and go to next step
				Repeat, // will stop execution and repeat step
				End,
				SetNextInstruction,
			};
		};

		class ScriptExecution
		: public SafeObject<ScriptExecution>
		{
			typedef SafeObject<ScriptExecution> base;
		public:
			typedef int Flags;
			enum Flag
			{
				Entered = 1
			};

		public:
			static void trigger_execution_trap(Name const& _trapName); // will just jump to a trap, won't run the code there!
			void trigger_own_execution_trap(Name const& _trapName); // only in itself

			ScriptExecution();
			~ScriptExecution();

			Script const* get_script() const { return script; }
			int get_at() const { return at; }
			
			void stop_all(); // stops also sub scripts

			void start(Script const* _script);
			void stop();

			void start_sub_script(Name const & _id, Script const* _script);
			void stop_sub_script(Name const & _id);

			void execute(); // until end/wait

			SimpleVariableStorage& access_variables() { return variables; }
			SimpleVariableStorage const& get_variables() const { return variables; }

			void set_at(int _at);
			void store_return_sub(int _offset = 1); // as most likely we will be returning to the next instruction
			void set_at_return_sub();

			void dont_output_info() { outputInfo.clear(); }
			void do_output_info(String const& _info) { outputInfo = _info; }

			void log(LogInfoContext& _log) const;

		public:
			float get_timer(Name const& _timerId) const;
			void reset_timer(Name const& _timerId, bool _onlyIfNotActive = false);
			void remove_timer(Name const& _timerId);

		private:
			void interrupted();

		private:
			static ScriptExecution* s_first;
			static Concurrency::SpinLock s_chainLock;
			bool inChain = false;
			ScriptExecution* next = nullptr;
			ScriptExecution* prev = nullptr;

			void add_to_chain();
			void remove_from_chain();

		private:
			Name id;

			Optional<String> outputInfo;

#ifdef AN_DEVELOPMENT_OR_PROFILER
			Array<int> linesExecuted;
			ArrayStatic<int, 16> linesExecutedRecently;
			float lineRepeatedFor = 0.0f;
#endif

			Concurrency::SpinLock executionLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.GameScript.ScriptExecution.executionLock"));
			Script const* script = nullptr;
			int at = NONE; // if none, not executing, has to be started again
			ScriptExecution::Flags flags = 0;
			Array<int> returnSub;

			SimpleVariableStorage variables;

			struct Timer
			{
				Name id;
				float time = 0.0f;
			};
			Array<Timer> timers;

			Array<::SafePtr<ScriptExecution>> subScriptExecutions;
		};
	};
};
