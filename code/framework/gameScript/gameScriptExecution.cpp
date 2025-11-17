#include "gameScriptExecution.h"

#include "gameScript.h"

#include "..\game\game.h"

#ifdef AN_DEVELOPMENT_OR_PROFILER
#include "..\modulesOwner\modulesOwner.h"
#endif

#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\system\core.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
#ifdef AN_DEVELOPMENT_OR_PROFILER
#define INSPECT_GAME_SCRIPT_CHAIN
#endif
#endif

//

using namespace Framework;
using namespace GameScript;

//

DEFINE_STATIC_NAME(self);

//

ScriptExecution* ScriptExecution::s_first = nullptr;
Concurrency::SpinLock ScriptExecution::s_chainLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.GameScript.ScriptExecution.s_chainLock"));

ScriptExecution::ScriptExecution()
: base(this)
{
}

ScriptExecution::~ScriptExecution()
{
	for_every_ref(sse, subScriptExecutions)
	{
		delete(sse);
	}

	remove_from_chain();
	make_safe_object_unavailable();
}

void ScriptExecution::trigger_execution_trap(Name const& _trapName)
{
	if (_trapName.is_valid())
	{
		Concurrency::ScopedSpinLock lock(ScriptExecution::s_chainLock);

		auto* se = s_first;
		while (se)
		{
			se->trigger_own_execution_trap(_trapName);
			se = se->next;
		}
	}
}

void ScriptExecution::trigger_own_execution_trap(Name const& _trapName)
{
	if (script)
	{
		bool sub = false;
		int goTo = script->get_execution_trap(*this, _trapName, &sub);
		if (goTo != NONE)
		{
			if (sub)
			{
				store_return_sub();
			}
			else
			{
				interrupted();
			}
			set_at(goTo);
		}
	}
}

void ScriptExecution::add_to_chain()
{
	Concurrency::ScopedSpinLock lock(ScriptExecution::s_chainLock);

	if (inChain)
	{
		return;
	}
	inChain = true;

#ifdef INSPECT_GAME_SCRIPT_CHAIN
	output(TXT("add to chain [%p]"), this);
#endif
	next = s_first;
	prev = nullptr;
	an_assert(!next || !next->prev);
	if (next) next->prev = this;
	s_first = this;
#ifdef INSPECT_GAME_SCRIPT_CHAIN
	{
		auto* c = s_first;
		while (c)
		{
			output(TXT("   [%p] [%p<- ->%p]"), c, c->prev, c->next);
			c = c->next;
		}
	}
#endif
}

void ScriptExecution::remove_from_chain()
{
	Concurrency::ScopedSpinLock lock(ScriptExecution::s_chainLock);

	if (! inChain)
	{
		return;
	}
	inChain = false;

#ifdef INSPECT_GAME_SCRIPT_CHAIN
	output(TXT("remove from chain [%p]"), this);
#endif
	if (next) next->prev = prev;
	if (prev) prev->next = next;
	if (s_first == this) s_first = next;
	next = nullptr;
	prev = nullptr;

#ifdef INSPECT_GAME_SCRIPT_CHAIN
	{
		auto* c = s_first;
		while (c)
		{
			an_assert(c != this);
			output(TXT("   [%p] [%p<- ->%p]"), c, c->prev, c->next);
			c = c->next;
		}
	}
#endif
}

void ScriptExecution::start_sub_script(Name const& _id, Script const* _script)
{
	::SafePtr<ScriptExecution> se;
	se = new ScriptExecution();
	se->id = _id;
	se->start(_script);
	se->variables.set_from(variables);
	subScriptExecutions.push_back(se);
}

void ScriptExecution::stop_sub_script(Name const& _id)
{
	for_every_ref(sse, subScriptExecutions)
	{
		if (sse->id == _id || ! _id.is_valid())
		{
			sse->stop();
		}
	}
}

void ScriptExecution::start(Script const* _script)
{
	if (outputInfo.is_set())
	{
		LogInfoContext log;
		log.log(TXT("start game script - %S"), outputInfo.get().to_char());
		{
			LOG_INDENT(log);
			if (_script)
			{
				log.log(TXT("script \"%S\""), _script->get_name().to_string().to_char());
				_script->log(log);
			}
			else
			{
				log.log(TXT("no script"));
			}
		}
		log.output_to_output();
	}

	interrupted(); // if we're running anything interrupt it

	add_to_chain();

	script = _script;
	returnSub.clear();
	set_at(0);
#ifdef AN_DEVELOPMENT_OR_PROFILER
	linesExecuted.clear();
	linesExecutedRecently.clear();
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	// try getting self to update ai additional info
	if (auto* exPtr = get_variables().get_existing<::SafePtr<Framework::IModulesOwner>>(NAME(self)))
	{
		if (auto* imo = exPtr->get())
		{
			imo->ai_set_additional_info(script ? script->get_name().to_string() : String(TXT("[no game script]")));
		}
	}
#endif

}

void ScriptExecution::stop()
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	// try getting self to update ai additional info
	if (auto* exPtr = get_variables().get_existing<::SafePtr<Framework::IModulesOwner>>(NAME(self)))
	{
		if (auto* imo = exPtr->get())
		{
			imo->ai_set_additional_info(String(TXT("[game script ended]")));
		}
	}
#endif

	interrupted();

	remove_from_chain();

	script = nullptr;
	returnSub.clear();
	set_at(NONE);
#ifdef AN_DEVELOPMENT_OR_PROFILER
	linesExecuted.clear();
	linesExecutedRecently.clear();
#endif
}

void ScriptExecution::stop_all()
{
	stop();
	for_every_ref(sse, subScriptExecutions)
	{
		sse->stop_all(); // will stop all and they will be removed afterwards
	}
}

void ScriptExecution::log(LogInfoContext& _log) const
{
	_log.set_colour(Colour::yellow);
	_log.log(TXT("%S"), script ? script->get_name().to_string().to_char() : TXT("no script"));
	_log.set_colour();
	{
		_log.log(TXT("script"));
		LOG_INDENT(_log);
		if (script && script->get_elements().is_index_valid(at))
		{
			int radius = 11;
			for_range(int, i, at - radius, at + radius)
			{
				if (script->get_elements().is_index_valid(i))
				{
#ifdef AN_DEVELOPMENT_OR_PROFILER
					bool wasExecuted = linesExecuted.does_contain(i);
#endif
					auto& e = script->get_elements()[i];
					String line = String::printf(TXT("%03i%c%S"), i, i == at ? '>' :
#ifdef AN_DEVELOPMENT_OR_PROFILER
						(wasExecuted ? ':' : ' ')
#else
						' '
#endif
						, e->get_debug_info());
					auto edi = e->get_extra_debug_info();
					if (!edi.is_empty())
					{
						line += String::printf(TXT(" / %S"), edi.to_char());
					}
					auto ec = e->get_colour();
					Colour c = ec.get(Colour::white);
#ifdef AN_DEVELOPMENT_OR_PROFILER
					Colour cinactive = ec.is_set()? Colour::lerp(0.5f, ec.get(), Colour::grey) : Colour::grey;
					{
						Optional<int> iatle;
						for_every(le, linesExecutedRecently)
						{
							if (*le == i)
							{
								iatle = for_everys_index(le); break;
							}
						}
						int ms = linesExecutedRecently.get_max_size();
						if (wasExecuted)
						{
							float hl = 0.1f + 0.9f * clamp(1.0f - (float)iatle.get(ms) / (float)ms, 0.0f, 1.0f);
							c = Colour::lerp(hl, cinactive.mul_rgb(0.8f), c);
						}
						else
						{
							c = cinactive.mul_rgb(0.6f);
						}
					}
#endif
					_log.set_colour(c);
					_log.log(line.to_char());
					_log.set_colour();
				}
				else
				{
					_log.log(TXT("--- "));
				}
			}
		}
		else
		{
			_log.log(TXT("not running"));
		}
	}
	{
		_log.log(TXT("variables"));
		LOG_INDENT(_log);
		variables.log(_log);
	}
	if (! subScriptExecutions.is_empty())
	{
		_log.log(TXT("sub scripts"));
		LOG_INDENT(_log);
		for_every_ref(sse, subScriptExecutions)
		{
			sse->log(_log);
		}
	}
}

void ScriptExecution::interrupted()
{
	if (!script || at == NONE)
	{
		return;
	}

	auto const& elements = script->get_elements();
	if (elements.is_index_valid(at))
	{
		elements[at]->interrupted(*this);
	}
}

void ScriptExecution::execute()
{
	Concurrency::ScopedSpinLock lock(executionLock);

	if (!timers.is_empty())
	{
		float deltaTime = ::System::Core::get_delta_time();
		for_every(timer, timers)
		{
			timer->time += deltaTime;
		}
	}

	if (script && at != NONE)
	{
		scoped_call_stack_info(TXT("script execute"));
		scoped_call_stack_info(script->get_name().get_name().to_char());

		int executedElements = 0;
		while (script && at != NONE)
		{
			int atNow = at;
			auto* scriptNow = script;
			auto const& elements = script->get_elements();
			if (!elements.is_index_valid(at))
			{
				set_at(NONE);
				break;
			}
			++executedElements;
			if (executedElements > 100)
			{
				warn(TXT("consider adding yield in %S %03i > %s"), outputInfo.get(String::empty()).to_char(), at, elements[at]->get_debug_info());
				AN_DEVELOPMENT_BREAK;
				break;
			}
			ScriptExecutionResult::Type result;
			{
				if (outputInfo.is_set() && is_flag_set(flags, ScriptExecution::Entered))
				{
					int const elementInfoLength = 512;
					allocate_stack_var(tchar, elementInfo, elementInfoLength);
#ifdef AN_CLANG
					tsprintf(elementInfo, elementInfoLength, TXT("exec game script [%S] %03i > %S"), outputInfo.get().to_char(), at, elements[at]->get_debug_info());
#else
					tsprintf(elementInfo, elementInfoLength, TXT("exec game script [%s] %03i > %s"), outputInfo.get().to_char(), at, elements[at]->get_debug_info());
#endif
#ifdef AN_ALLOW_EXTENSIVE_LOGS
					output(TXT("%S"), elementInfo);
#endif
				}
				int const elementInfoLength = 512;
				allocate_stack_var(tchar, elementInfo, elementInfoLength);
#ifdef AN_CLANG
				tsprintf(elementInfo, elementInfoLength, TXT("exec %03i > %S"), at, elements[at]->get_debug_info());
#else
				tsprintf(elementInfo, elementInfoLength, TXT("exec %03i > %s"), at, elements[at]->get_debug_info());
#endif
				scoped_call_stack_info(elementInfo);
#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (!linesExecutedRecently.is_empty() && linesExecutedRecently.get_first() == at)
				{
					lineRepeatedFor += Game::get()->get_delta_time();
					if (lineRepeatedFor > 0.1f)
					{
						lineRepeatedFor = 0.0f;
					}
				}
				else
				{
					lineRepeatedFor = 0.0f;
				}
				if (lineRepeatedFor == 0.0f)
				{
					if (!linesExecutedRecently.has_place_left())
					{
						linesExecutedRecently.pop_back();
					}
					linesExecutedRecently.push_front(at);
				}
				linesExecuted.push_back_unique(at);
#endif
				result = elements[at]->execute(*this, flags);
			}
			if (result == ScriptExecutionResult::Continue)
			{
				// go to next step
				set_at(at + 1);
#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (::System::Core::is_advancing_one_frame_only()) break;
#endif
				continue;
			}
			clear_flag(flags, ScriptExecution::Entered);
			if (result == ScriptExecutionResult::Yield)
			{
				// go to next step
				set_at(at + 1);
				break;
			}
			if (result == ScriptExecutionResult::Repeat)
			{
				break;
			}
			if (result == ScriptExecutionResult::End)
			{
				set_at(NONE);
				break;
			}
			if (result == ScriptExecutionResult::SetNextInstruction)
			{
				if (atNow == at &&
					scriptNow == script)
				{
					error(TXT("did not set next instruction or set to the same"));
					set_at(at + 1);
				}
				flags = ScriptExecution::Entered;
#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (::System::Core::is_advancing_one_frame_only()) break;
#endif
				continue;
			}

			todo_important(TXT("implement other result"));
		}
	}

	{
		bool removeOne = false;
		for_every_ref(sse, subScriptExecutions)
		{
			sse->execute();
			if (!sse->script)
			{
				removeOne = true;
			}
		}
		if (removeOne)
		{
			for_every_ref(sse, subScriptExecutions)
			{
				if (!sse->script)
				{
					subScriptExecutions.remove_at(for_everys_index(sse));
					delete(sse);
					break;
				}
			}
		}
	}
}

void ScriptExecution::set_at(int _at)
{
	at = _at;
	flags = ScriptExecution::Entered;
}

void ScriptExecution::store_return_sub(int _offset)
{
	returnSub.push_back(at + _offset);
}

void ScriptExecution::set_at_return_sub()
{
	if (returnSub.is_empty())
	{
		error(TXT("can't return from sub"));
		return;
	}
	at = returnSub.get_last();
	returnSub.pop_back();
}

float ScriptExecution::get_timer(Name const& _timerId) const
{
	Concurrency::ScopedSpinLock lock(ScriptExecution::s_chainLock);

	for_every(timer, timers)
	{
		if (timer->id == _timerId)
		{
			return timer->time;
		}
	}

	return 0.0f;
}

void ScriptExecution::reset_timer(Name const& _timerId, bool _onlyIfNotActive)
{
	Concurrency::ScopedSpinLock lock(ScriptExecution::s_chainLock);

	for_every(timer, timers)
	{
		if (timer->id == _timerId)
		{
			if (!_onlyIfNotActive) // if this is true, it won't reset the timer, just quit
			{
				timer->time = 0.0f;
			}
			return;
		}
	}

	Timer t;
	t.id = _timerId;
	t.time = 0.0f;
	timers.push_back(t);
}

void ScriptExecution::remove_timer(Name const& _timerId)
{
	Concurrency::ScopedSpinLock lock(ScriptExecution::s_chainLock);

	for_every(timer, timers)
	{
		if (timer->id == _timerId)
		{
			timers.remove_fast_at(for_everys_index(timer));
			return;
		}
	}
}
