#pragma once

#include "..\globalDefinitions.h"
#include "..\utils.h"

struct String;

template <typename Element, int Size> class ArrayStatic;

// undefine any previous definitions
#undef an_assert

#ifdef AN_LINUX
void DebugBreak();
#endif

// using define DEBUG_GPU_MEMORY

#define GPU_MEMORY_INFO_TYPE_TEXTURE 0
#define GPU_MEMORY_INFO_TYPE_RENDER_BUFFER 1
#define GPU_MEMORY_INFO_TYPE_BUFFER 2
#define GPU_MEMORY_INFO_TYPE__COUNT 3
#ifdef DEBUG_GPU_MEMORY
void gpu_memory_info_allocated_func(int _type, int _id, int _amount);
void gpu_memory_info_freed_func(int _type, int _id);
void gpu_memory_info_get_func(size_t & _refAllocated, size_t& _refAvailable);
#define gpu_memory_info_allocated(_type, _id, _amount) gpu_memory_info_allocated_func(_type, _id, _amount);
#define gpu_memory_info_freed(_type, _id) gpu_memory_info_freed_func(_type, _id);
#define gpu_memory_info_get(_refAllocated, _refAvailable) gpu_memory_info_get_func(_refAllocated, _refAvailable);
#else
#define gpu_memory_info_allocated(_type, _id, _amount);
#define gpu_memory_info_freed(_type, _id);
#define gpu_memory_info_get(_refAllocated, _refAvailable);
#endif

#ifdef AN_DEVELOPMENT
void ignore_debug_on_error_warn();
void debug_on_error_impl();
void debug_on_warn_impl();
#define debug_on_error() debug_on_error_impl();
#define debug_on_warn() debug_on_warn_impl();
#else
#define debug_on_error()
#define debug_on_warn()
#endif

int get_current_thread_system_id();

void trace_func(tchar const * const _text = nullptr, ...);
void report_func(tchar const * const _text = nullptr, ...);
void report_gl_func(tchar const * const _text = nullptr, ...);
void report_to_file_func(tchar const * const _text = nullptr, ...);
void report_waiting_func(bool _endWaiting);
void report_break_into_lines(String const & _text);
void detected_memory_leaks_report_func(tchar const * const _text = nullptr, ...);
void close_file_after_detected_memory_leaks();

String copy_log(tchar const * _fileName, bool _addCurrentTime);
String copy_log_and_switch(tchar const * _fileName, bool _addCurrentTime);

tchar const * get_log_file_name();

void disallow_output();
void allow_output();
bool is_output_allowed();

// lock unlock to allow continuous locking
void lock_output();
void unlock_output();

struct ScopedOutputLock
{
	ScopedOutputLock()
	{
		lock_output();
	}
	~ScopedOutputLock()
	{
		unlock_output();
	}
};

void close_output();

/**
 *	Crash/fault
 */

#define AN_FAULT abort();

/**
 *	Break
 */
#ifdef AN_DEVELOPMENT_OR_PROFILER
	#ifdef AN_CLANG
		#define AN_BREAK __builtin_trap();
	#else
		#define AN_BREAK __debugbreak();
	#endif
#else
	#define AN_BREAK AN_FAULT
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
#define AN_DEVELOPMENT_BREAK AN_BREAK
#else
#define AN_DEVELOPMENT_BREAK
#endif

#define AN_PAUSE ::System::Core::pause();

// output message with end-line character
#ifdef AN_OUTPUT
	// output message without end-line character
	#define trace(...) { ScopedOutputLock outputLock; trace_func(__VA_ARGS__); }
	// output message with end-line character and info about file and line
	#define report(...) { ScopedOutputLock outputLock; trace_func(TXT("%S (%i) : "), TXT(__FILE__), __LINE__); report_func(__VA_ARGS__); }
	#define output(...) { ScopedOutputLock outputLock; report_func(__VA_ARGS__); }
	#define output_gl(...) { report_gl_func(__VA_ARGS__); /*output(__VA_ARGS__);*/ }
	#define output_break_into_lines(_string) report_break_into_lines(_string);
	// to be used only for detected memory leaks, this is required as global file in debug is closed BEFORE memory leaks (and keep it this way - reopen just for a while)
	#define detected_memory_leaks_output(...) { ScopedOutputLock outputLock; detected_memory_leaks_report_func(__VA_ARGS__); }
	#define detected_memory_leaks_end() { close_file_after_detected_memory_leaks(); }
	#define output_waiting() { ScopedOutputLock outputLock; report_waiting_func(false); }
	#define output_waiting_done() { ScopedOutputLock outputLock; report_waiting_func(true); }
#else
	#define trace(...)
	#define report(...)
	#define output(...)
	#define output_gl(...)
	#define output_break_into_lines(...)
	#define detected_memory_leaks_output(...)
	#define detected_memory_leaks_end()
	#define output_waiting()
	#define output_waiting_done()
#endif

#define output_spam(...) output(__VA_ARGS__)

#ifdef AN_OUTPUT_TO_FILE
	#define log_to_file(...) { ScopedOutputLock outputLock; report_to_file_func(__VA_ARGS__); }
#else
	#define log_to_file(...)
#endif

#ifdef AN_OUTPUT
void output_colour(int _r = 1, int _g = 1, int _b = 1, int _i = 0);
void output_colour_system();
void output_colour_nav();
#else
inline void output_colour(int _r = 1, int _g = 1, int _b = 1, int _i = 0) {}
inline void output_colour_system() {}
inline void output_colour_nav() {}
#endif

void show_critical_message(bool _stopIsObligatory, tchar const * _message);

void debug_internal_pop_up_format(tchar const* const _format = nullptr, ...);
int debug_internal_pop_up_message_box(bool _stopIsObligatory, tchar const* const _condition);


#define error_dev_ignore(...) { ScopedOutputLock outputLock; output_colour(1,0,0,1); trace_func(TXT("[ERROR] ")); report_func(__VA_ARGS__); output_colour(); }
#define error_dev_investigate(...) { error_dev_ignore(__VA_ARGS__); debug_on_error(); }
#define error(...) { error_dev_investigate(__VA_ARGS__); }
#ifdef AN_DEVELOPMENT
#ifdef AN_ANDROID
	#define error_stop(...) { error(__VA_ARGS__); debug_internal_pop_up_format(__VA_ARGS__); if (debug_internal_pop_up_message_box(true, TXT("error stop"))) { AN_BREAK; } }
#else
	#define error_stop(...) { error(__VA_ARGS__); AN_BREAK; }
#endif
#else
	#define error_stop(...) { error(__VA_ARGS__); }
#endif

#define warn_dev_ignore(...) { ScopedOutputLock outputLock; output_colour(1,1,0,1); trace_func(TXT("[warning] ")); report_func(__VA_ARGS__); output_colour(); }
#define warn_dev_investigate(...) { warn_dev_ignore(__VA_ARGS__); debug_on_warn(); }
#define warn(...) { warn_dev_ignore(__VA_ARGS__); }

#define pop_up_message(msg, ...) \
	{ \
		ScopedOutputLock outputLock; \
		output_colour(0,1,1,1); \
		report(TXT("pop up message!")); \
		trace(msg); \
		trace(TXT(" : ")); \
		output(__VA_ARGS__); \
		debug_internal_pop_up_format(__VA_ARGS__); \
		if (debug_internal_pop_up_message_box(false, msg)) { AN_BREAK; } \
		output_colour(); \
	}

int failed_assert_count();
void mark_failed_assert(tchar const* _info);
String get_failed_assert_info();

#define is_rendering_thread() (! Concurrency::ThreadManager::get() || Concurrency::ThreadManager::get_current_thread_id() == 0)

#ifndef AN_ASSERT
	#define an_assert(cond, ...)
	#define an_assert_info(cond, ...)
	#define an_assert_immediate(cond, ...)
	#define assert_rendering_thread()
	#define an_assert_log_always(cond, ...) \
		if (! (cond)) \
		{ \
			ScopedOutputLock outputLock; \
			output_colour(1,0,1,1); \
			report(TXT(" assertion failed")); \
			trace(TXT("\tcondition : ")); \
			output(TXT(#cond)); \
			trace(TXT("\tinfo      : ")); \
			output(__VA_ARGS__); \
			output_colour(); \
			mark_failed_assert(TXT(#cond)); \
		}
#else
	#define an_assert(cond, ...) \
		if (! (cond)) \
		{ \
			ScopedOutputLock outputLock; \
			output_colour(1,0,1,1); \
			report(TXT(" assertion failed")); \
			trace(TXT("\tcondition : ")); \
			output(TXT(#cond)); \
			trace(TXT("\tinfo      : ")); \
			output(__VA_ARGS__); \
			debug_internal_pop_up_format(__VA_ARGS__); \
			if (debug_internal_pop_up_message_box(false, TXT(#cond))) { AN_BREAK; } \
			output_colour(); \
			mark_failed_assert(TXT(#cond)); \
		}
	#define an_assert_info(cond, ...) \
		if (! (cond)) \
		{ \
			ScopedOutputLock outputLock; \
			output_colour(1,0,1,1); \
			report(TXT(" assertion failed")); \
			trace(TXT("\tcondition : ")); \
			output(TXT(#cond)); \
			trace(TXT("\tinfo      : ")); \
			output(__VA_ARGS__); \
			output_colour(); \
			mark_failed_assert(TXT(#cond)); \
		}
	#define an_assert_immediate(cond, ...) \
		if (! (cond)) \
		{ \
			AN_BREAK; \
			ScopedOutputLock outputLock; \
			output_colour(1,0,1,1); \
			report(TXT(" assertion (immediate) failed")); \
			trace(TXT("\tcondition : ")); \
			output(TXT(#cond)); \
			trace(TXT("\tinfo      : ")); \
			output(__VA_ARGS__); \
			output_colour(); \
			mark_failed_assert(TXT(#cond)); \
		}
	#define assert_rendering_thread() an_assert(! Concurrency::ThreadManager::get() || Concurrency::ThreadManager::get_current_thread_id() == 0);
	#define an_assert_log_always(cond, ...) \
		if (! (cond)) \
		{ \
			ScopedOutputLock outputLock; \
			output_colour(1,0,1,1); \
			report(TXT(" assertion failed")); \
			trace(TXT("\tcondition : ")); \
			output(TXT(#cond)); \
			trace(TXT("\tinfo      : ")); \
			output(__VA_ARGS__); \
			debug_internal_pop_up_format(__VA_ARGS__); \
			if (debug_internal_pop_up_message_box(false, TXT(#cond))) { AN_BREAK; } \
			output_colour(); \
			mark_failed_assert(TXT(#cond)); \
		}
#endif

#ifdef AN_DEVELOPMENT
#define debug_stop(cond) if (cond) AN_BREAK;
#else
#define debug_stop(cond)
#endif

#ifndef AN_ASSERT_SLOW
	#define assert_slow(cond, ...)
#else
	#define assert_slow(cond, ...) \
		if (! (cond)) \
		{ \
			ScopedOutputLock outputLock; \
			output_colour(1,0,1,1); \
			report(TXT(" slow assertion failed")); \
			trace(TXT("\tcondition : ")); \
			output(TXT(#cond)); \
			debug_internal_pop_up_format(__VA_ARGS__); \
			output_colour(); \
			if (debug_internal_pop_up_message_box(false, TXT(#cond))) { AN_BREAK; } \
			mark_failed_assert(TXT(#cond)); \
		}
#endif

// use todo_implement_now when adding new things
#ifndef AN_TODO
	#define todo_implement_now
	#define todo_implement
	#define todo_important(...)
	#define todo_deprecate(...)
	#define todo_optimise(...)
	#define todo_note(...)
	#define todo_future(...)
	#define todo_hack(...)
	#define todo_header(...)
	#define todo_multiplayer_issue(...)
#else
	#define todo_implement_now pop_up_message(TXT("implement"))
	#define todo_implement pop_up_message(TXT("implement"))
	#define todo_important(...) pop_up_message(TXT("todo"), __VA_ARGS__)
	#define todo_deprecate(...)
	#define todo_optimise(...)
	#ifndef AN_TODO_NOTE
		#define todo_note(...)
	#else
		#define todo_note(...) report(__VA_ARGS__)
	#endif
	#ifndef AN_TODO_FUTURE
		#define todo_future(...)
	#else
		#define todo_future(...) report(__VA_ARGS__)
	#endif
	#define todo_hack(...)
	#define todo_header(...)
	#define todo_multiplayer_issue(...)
#endif

struct CallStackInfo
{
	static const int maxThreads = MAX_THREAD_COUNT;

	static void push(void const* _ptr, tchar const* _info);
	static void pop();
	static String get_info(ArrayStatic<int, maxThreads> const * _threads = nullptr);
	static tchar const * get_currently_at_info(ArrayStatic<int, maxThreads> const * _threads = nullptr);
	static void lock(); // cannot be unlocked!

	struct Scoped
	{
		Scoped(void const* _ptr, tchar const* _info) { push(_ptr, _info); }
		~Scoped() { pop(); }
	};
};

#ifdef AN_STORE_CALL_STACK_INFO
	#define scoped_call_stack_info_str_printf(...) String temp_variable_named(scopedCallStackInfoStr) = String::printf(__VA_ARGS__); CallStackInfo::Scoped temp_variable_named(scopedCallStackInfo)(nullptr, temp_variable_named(scopedCallStackInfoStr).to_char())
	#define scoped_call_stack_info(_info) CallStackInfo::Scoped temp_variable_named(scopedCallStackInfo)(nullptr, _info)
	#define scoped_call_stack_ptr(_ptr) CallStackInfo::Scoped temp_variable_named(scopedCallStackInfo)(_ptr, nullptr)
	#define scoped_call_stack_ptr_info(_ptr, _info) CallStackInfo::Scoped temp_variable_named(scopedCallStackInfo)(_ptr, _info)
	#define lock_call_stack_info() CallStackInfo::lock()
	#define push_call_stack_info(_info) CallStackInfo::push(nullptr, _info)
	#define push_call_stack_ptr(_ptr) CallStackInfo::push(_ptr, nullptr)
	#define pop_call_stack_info() CallStackInfo::pop()
	#define get_call_stack_info() CallStackInfo::get_info()
	#define get_call_stack_info_currently_at() CallStackInfo::get_currently_at_info()
	#define get_call_stack_info_for_threads(_threads) CallStackInfo::get_info(&_threads)
#else
	#define scoped_call_stack_info_str_printf(...)
	#define scoped_call_stack_info(_info)
	#define scoped_call_stack_ptr(_ptr)
	#define scoped_call_stack_ptr_info(_ptr, _info)
	#define lock_call_stack_info()
	#define push_call_stack_info(_info)
	#define push_call_stack_ptr(_ptr)
	#define pop_call_stack_info()
	#define get_call_stack_info() String::empty()
	#define get_call_stack_info_currently_at() TXT("")
	#define get_call_stack_info_for_threads(_threads) String::empty()
#endif

String get_general_debug_info();
