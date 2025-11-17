#pragma once

#include "latentScope.h"
#include "latentFrame.h"

#include "..\types\optional.h"

#include "..\performance\performanceUtils.h"

/**	latent functions and latent code

	notes

	try to avoid using breaks and continues inside of loops
	it's advised to use goto and labels


	usage

	LATENT_FUNCTION(latent_do_for_index)
	{
		LATENT_SCOPE_INFO(TXT("info"));

		LATENT_PARAM(int, _index);
		LATENT_END_PARAMS();

		LATENT_BEGIN_CODE();
		output("waiting");
		LATENT_WAIT(0.5f);
		output("done waiting");
		LATENT_END_CODE();
		LATENT_RETURN();
	}

	LATENT_FUNCTION(latent_iterate)
	{
		LATENT_SCOPE();

		LATENT_PARAM(int, _count);
		LATENT_END_PARAMS();

		LATENT_VAR(int, i);

		do some stuff that is not latent and should be done before every execution

		LATENT_BEGIN_CODE();
		for (i = 0; i < _count; ++ i)
		{
			output("performing for %i", i);
			LATENT_CALL_FUNCTION_1(latent_do_for_index, i);
			output("done performing for %i", i);
			output("see you in next frame!");
			LATENT_YIELD();
			if (extremely strange condition)
			{
				LATENT_ABORT(); // will skip LATENT_ON_BREAK and LATENT_ON_END
			}
		}
		LATENT_END(); // will skip LATENT_ON_BREAK

		LATENT_ON_BREAK(); // optional, to skip it, use LATENT_END() or LATENT_ABORT(), it will go straight to end, to hit it, use LATENT_BREAK()
		do something when latent function breaks
		LATENT_ON_END(); // optional, to skip it, use LATENT_ABORT(), it will go straight to end, to hit it, use LATENT_END()
		do something when latent function ends
		LATENT_END_CODE();

		do some stuff that is not latent and should be done after every execution

		LATENT_RETURN();
	}

	...

	// declaration of frame
	::Latent::Frame latentFrame;

	...

	// new call
	SETUP_LATENT(latentFrame);
	ADD_LATENT_PARAM(latentFrame, int, 5);

	...

	if (PERFORM_LATENT(latentFrame, deltaTime, latent_iterate))
	{
		// finished latent
	}

	...

	BREAK_LATENT(latentFrame, latent_iterate);

 */

typedef bool(*LatentFunction)(REF_::Latent::Frame & _frame);

#define LATENT_FUNCTION(_functionName) \
	bool _functionName(REF_ ::Latent::Frame & _frame)

#define LATENT_FUNCTION_VARIABLE(_functionName) \
	LatentFunction _functionName

#define LATENT_FUNCTION_TYPE \
	LatentFunction

#include "functions\latentFunctions.h"

// utils:
#define LATENT_CALL_FUNCTION_INIT() \
			latentScope.append_block(); \

#define LATENT_CALL_FUNCTION_PARAM(param) \
			latentScope.next_function_param(param);

#define LATENT_CALL_FUNCTION_PERFORM(functionName) \
			latentScope.set_current_state(__LINE__); \
		case __LINE__: \
			if (! functionName(_frame)) \
			{ \
				if (_frame.is_breaking()) \
				{ \
					goto LATENT_END_CODE_LABEL; \
				} \
				return false; /* we will be continuing in next call */ \
			} \
			if (_frame.is_breaking()) /* don't continue, quit immediately */ \
			{ \
				goto LATENT_END_CODE_LABEL; \
			}

// custom ids

#define LATENT_BREAK_ID -1
#define LATENT_END_ID -2

// inside function:

#define LATENT_SCOPE() \
	::Latent::Scope latentScope(_frame);

#define LATENT_TASK_LIMIT 0.0005f
#define LATENT_TASK_ELEMENT_LIMIT 0.0001f

#ifdef AN_INSPECT_LATENT
#define LATENT_SCOPE_INFO(info) latentScope.add_info(info); PERFORMANCE_GUARD_LIMIT(0.005f, info); MEASURE_PERFORMANCE_STR(info) SCOPED_PERFORMANCE_LIMIT_GUARD(LATENT_TASK_LIMIT, info)
#else
#ifdef AN_MEASURE_LATENT
#define LATENT_SCOPE_INFO(info) MEASURE_PERFORMANCE_STR(info) SCOPED_PERFORMANCE_LIMIT_GUARD(LATENT_TASK_LIMIT, info)
#else
#define LATENT_SCOPE_INFO(info) SCOPED_PERFORMANCE_LIMIT_GUARD(LATENT_TASK_LIMIT, info)
#endif
#endif
#define LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(info) SCOPED_PERFORMANCE_LIMIT_GUARD(LATENT_TASK_ELEMENT_LIMIT, info)

#ifdef AN_INSPECT_LATENT
	#define LATENT_CLEAR_LOG() _frame.access_log_lock().acquire(); _frame.access_log().clear(); _frame.access_log_lock().release(); 
	#ifdef AN_CLANG
		#define LATENT_LOG(...) _frame.access_log_lock().acquire(); _frame.access_log().log(__VA_ARGS__); _frame.access_log_lock().release(); 
		#define LATENT_SECTION_INFO(...) _frame.set_section_info(String::printf(__VA_ARGS__).to_char());
	#else
		#define LATENT_LOG(...) _frame.access_log_lock().acquire(); _frame.access_log().log(##__VA_ARGS__); _frame.access_log_lock().release(); 
		#define LATENT_SECTION_INFO(...) _frame.set_section_info(String::printf(##__VA_ARGS__).to_char());
	#endif
#else
	#define LATENT_CLEAR_LOG()
	#define LATENT_LOG(...)
	#define LATENT_SECTION_INFO(...)
#endif

// only existing params
#define LATENT_PARAM(_type, _var) \
	_type & _var = latentScope.next_existing_param<_type>(TXT(#_var));

#define LATENT_PARAM_NOT_USED(_type, _var) \
	latentScope.next_existing_param<_type>(TXT(#_var));

// can be skipped
#define LATENT_PARAM_OPTIONAL(_type, _var, _def) \
	_type const * optionalPtr##_var = latentScope.next_optional_param<_type>(TXT(#_var)); \
	_type const & _var = optionalPtr##_var? *optionalPtr##_var : _def;

// this is optional call to jump straight to vars
#define LATENT_END_PARAMS() \
	latentScope.end_params();

// may add var (well, actually on first use it should add and then just access existing
#define LATENT_VAR(_type, _var) \
	_type & _var = latentScope.next_var<_type>(TXT(#_var));

// latent function breaking is done in two steps - first finish current block (just pass breaking there), second, call on_break. if it is last block, then just go to on_break
#define LATENT_BEGIN_CODE() \
	Optional<bool> latentCodeResult; /* if not provided, will check whether scope has finished */ \
	for (int latentPass = (_frame.is_breaking() && latentScope.is_last_block()? 1 : 0); \
		 latentPass < (_frame.is_breaking()? 2 : 1); \
		 ++ latentPass) \
	{ \
		switch (latentPass == 1? LATENT_BREAK_ID : latentScope.get_current_state()) \
		{ \
			case 0: \
				LATENT_SECTION_INFO(TXT(""));


// break latent execution with BREAK_LATENT (or LATENT_BREAK if form inside), if LATENT_END not issued earlier, it will be run anyway
#define LATENT_ON_BREAK() \
			goto LATENT_BREAK_LABEL; /* to avoid unreferenced label */  \
		LATENT_BREAK_LABEL:; \
			latentScope.set_current_state(LATENT_BREAK_ID); \
		case LATENT_BREAK_ID:

// accessed only with normal run or with LATENT_END
#define LATENT_ON_END() \
			goto LATENT_END_LABEL; /* to avoid unreferenced label */  \
		LATENT_END_LABEL:; \
			latentScope.set_current_state(LATENT_END_ID); \
		case LATENT_END_ID:

// end of latent code, you can still run some stuff afterwards
#define LATENT_END_CODE() \
			latentScope.set_current_state(__LINE__); \
		case __LINE__: \
		default: /* default to catch lack of on_break */ \
			latentScope.mark_has_finished(); \
			goto LATENT_END_CODE_LABEL; /* just go to end to not have unreferenced */ \
		} \
		LATENT_END_CODE_LABEL:; \
	}

// very end of latent function, return with result
#define LATENT_RETURN() \
	return latentCodeResult.get(latentScope.has_finished());

// latent delta time is not accumulated! it is updated every call
// it can be used inside latent code only with LATENT_YIELD or before LATENT_BEGIN_CODE
#define LATENT_DELTA_TIME latentScope.get_delta_time()
#define LATENT_DECREASE_DELTA_TIME_BY(_by) latentScope.decrease_delta_time(_by)
#define LATENT_ZERO_DELTA_TIME() latentScope.decrease_delta_time(latentScope.get_delta_time())

// pass execution back and wait for next frame to continue after this "yield"
#define LATENT_YIELD() \
			latentScope.set_current_state(__LINE__); \
			if (_frame.is_breaking()) \
			{ \
				goto LATENT_END_CODE_LABEL; /* just go to end to break */ \
			} \
			latentCodeResult = false; /* provide result "false" to continue execution */ \
			goto LATENT_END_CODE_LABEL; \
		case __LINE__:

// similar to yield but checks call time
#define LATENT_YIELD_IF_CALL_TIME_EXCEEDS(_time) \
			latentScope.set_current_state(__LINE__); \
			if (_frame.is_breaking()) \
			{ \
				goto LATENT_END_CODE_LABEL; /* just go to end to break */ \
			} \
			if (_frame.get_call_time() >= _time) \
			{ \
				latentCodeResult = false; /* provide result "false" to continue execution */ \
				goto LATENT_END_CODE_LABEL; \
			} \
		case __LINE__:

// end execution, go through "on break" (and "on end"), we will end at code's end anyway
#define LATENT_BREAK() \
			goto LATENT_BREAK_LABEL;

// end execution, go through "on end", we will end at code's end anyway
#define LATENT_END() \
			goto LATENT_END_LABEL;

// end execution, skip "on break", skip "on end"
#define LATENT_ABORT() \
			latentScope.set_current_state(__LINE__); \
		case __LINE__: \
			latentScope.mark_has_finished(); \
			goto LATENT_END_CODE_LABEL;

#define LATENT_CALL_FUNCTION(functionName) \
	LATENT_CALL_FUNCTION_INIT() \
	LATENT_CALL_FUNCTION_PERFORM(functionName)

#define LATENT_CALL_FUNCTION_1(functionName, param0) \
	LATENT_CALL_FUNCTION_INIT() \
	LATENT_CALL_FUNCTION_PARAM(param0) \
	LATENT_CALL_FUNCTION_PERFORM(functionName)

#define LATENT_CALL_FUNCTION_2(functionName, param0, param1) \
	LATENT_CALL_FUNCTION_INIT() \
	LATENT_CALL_FUNCTION_PARAM(param0) \
	LATENT_CALL_FUNCTION_PARAM(param1) \
	LATENT_CALL_FUNCTION_PERFORM(functionName)

#define LATENT_CALL_FUNCTION_3(functionName, param0, param1, param2) \
	LATENT_CALL_FUNCTION_INIT() \
	LATENT_CALL_FUNCTION_PARAM(param0) \
	LATENT_CALL_FUNCTION_PARAM(param1) \
	LATENT_CALL_FUNCTION_PARAM(param2) \
	LATENT_CALL_FUNCTION_PERFORM(functionName)

// built-in functions

#define LATENT_WANTS_TO_WAIT_FOR(_howLong) \
	_frame.wants_to_wait_for(_howLong);

#define LATENT_WAIT(_howLong) \
	LATENT_CALL_FUNCTION_2(::Latent::Functions::wait, _howLong, true)

// with "no rare advance", logic (whole logic!) is updated every second, normal LATENT_WAIT may skip until required
#define LATENT_WAIT_NO_RARE_ADVANCE(_howLong) \
	LATENT_CALL_FUNCTION_2(::Latent::Functions::wait, _howLong, false)

#define LATENT_WAIT_PRECISE(_howLong) \
	LATENT_CALL_FUNCTION_1(::Latent::Functions::wait_precise, _howLong)

//

#define SETUP_LATENT(latentFrame) \
	(latentFrame).reset();

#define ADD_LATENT_PARAM(latentFrame, type, value) \
	(latentFrame).add_param<type>(value);

#define ADD_LATENT_PARAMS(latentFrame, latentStackVariables) \
	(latentFrame).add_params(&latentStackVariables);

#define PERFORM_LATENT(latentFrame, deltaTime, functionName) \
	functionName((latentFrame).fresh_call().with_delta_time(deltaTime))

// will break and immediately perform what there is to break
#define BREAK_LATENT(latentFrame, functionName) \
	functionName((latentFrame).fresh_call().break_now())
