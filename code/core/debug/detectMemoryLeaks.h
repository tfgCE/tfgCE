#pragma once

#include "..\globalSettings.h"
#include "..\memory\memory.h"

void store_new_call();
void store_delete_call();

#ifndef MEMORY_H_INLCUDED
	#error include memory.h
#endif

// put:
//	memory_leak_suspect;
//		suspected_code()
//	forget_memory_leak_suspect;
// where you think might be source of memory leak

// put:
//	memory_leak_info(TXT("info"));
//		suspected_code()
//	forget_memory_leak_info;
// where you think might be source of memory leak

// this is to catch if detect memory leaks has been included
#ifdef new
#undef new
#endif

#ifdef AN_CHECK_MEMORY_LEAKS

struct AllocationBook;

void mark_main_thread_running();

void mark_static_allocations_done_for_memory_leaks();
void mark_static_allocations_left_for_memory_leaks();

// this is useful when we want to work on pure memory
void* allocate_actual_memory(size_t size);
void free_actual_memory(void* pointer);

size_t get_memory_allocated_count();
size_t get_memory_allocated_checked_in();
size_t get_memory_allocated_total();
void report_memory_allocated(size_t _size);
void report_memory_freed(size_t _size);

void start_memory_detect_checkpoint();
void end_memory_detect_checkpoint(bool _allThreads = false);

// this allocates empty book to be used with check in and check out

#ifdef AN_INSPECT_MEMORY_LEAKS
void on_new_internal(void* pointer, const tchar* file, int line, const tchar* memoryLeakSuspectFile, int memoryLeakSuspectLine, const tchar* memoryLeakInfo);
void on_delete_internal(void* pointer);
void* allocate_memory_for_memory_leaks_internal(size_t size, const tchar* file, int line, const tchar* memoryLeakSuspectFile, int memoryLeakSuspectLine, const tchar* memoryLeakInfo);
void free_memory_for_memory_leaks_internal(void* pointer);
void* operator new(size_t size, const tchar* file, int line, const tchar* memoryLeakSuspectFile, int memoryLeakSuspectLine, const tchar* memoryLeakInfo);
void* operator new[](size_t size, const tchar* file, int line, const tchar* memoryLeakSuspectFile, int memoryLeakSuspectLine, const tchar* memoryLeakInfo);
void operator delete(void* pointer, const tchar* file, int line);
void operator delete[](void* pointer, const tchar* file, int line);
void operator delete(void* pointer);
void operator delete[](void* pointer);
void* allocate_memory_detect_memory_leaks(size_t size, const tchar* file, int line, const tchar* memoryLeakSuspectFile, int memoryLeakSuspectLine, const tchar* memoryLeakInfo);
void free_memory_detect_memory_leaks(void* pointer);
void detect_memory_corruption(void* pointer);
#else
void on_new_internal(void* pointer);
void on_delete_internal(void* pointer);
void* allocate_memory_for_memory_leaks_internal(size_t size);
void free_memory_for_memory_leaks_internal(void* pointer);
void* operator new(size_t size, bool _inspectMemoryLeaks);
void* operator new[](size_t size, bool _inspectMemoryLeaks);
void operator delete(void* pointer, bool _inspectMemoryLeaks);
void operator delete[](void* pointer, bool _inspectMemoryLeaks);
void operator delete(void* pointer) AN_CLANG_NO_EXCEPT;
void operator delete[](void* pointer) AN_CLANG_NO_EXCEPT;
void* allocate_memory_detect_memory_leaks(size_t size);
void free_memory_detect_memory_leaks(void* pointer);
void detect_memory_corruption(void* pointer);
#endif

// store file and line, this method returns bool, to work with macro below
#ifdef AN_INSPECT_MEMORY_LEAKS
bool store_file_and_line(const tchar* file, int line);
bool store_memory_leak_suspect_file_and_line(const tchar* file, int line);
bool store_memory_leak_info(const tchar* info);
#endif
void supress_memory_leak_detection();
void resume_memory_leak_detection();
bool is_memory_leak_detection_supressed();
#ifdef AN_INSPECT_MEMORY_LEAKS
const tchar* restore_file();
int restore_line();
const tchar* restore_memory_leak_suspect_file();
int restore_memory_leak_suspect_line();
const tchar* restore_memory_leak_info();
#endif
void clear_memory_leak_suspect();

// to override default operator new (leave placement new as it is) and redirect to our
// but to get proper file and line of caller, we need to depend on stored (see macro below) values
void* __cdecl operator new(size_t size);
void* __cdecl operator new[](size_t size);

// this trickery is to store file and line of caller and to continue with execution
#ifdef AN_INSPECT_MEMORY_LEAKS
#define new store_file_and_line(TXT(__FILE__), __LINE__)? nullptr : new
#define memory_leak_suspect store_memory_leak_suspect_file_and_line(TXT(__FILE__), __LINE__)
#define forget_memory_leak_suspect store_memory_leak_suspect_file_and_line(nullptr, 0)
#define memory_leak_info(txt) store_memory_leak_info(txt)
#define forget_memory_leak_info store_memory_leak_info(nullptr)
#else
#define memory_leak_suspect
#define forget_memory_leak_suspect
#define memory_leak_info(txt)
#define forget_memory_leak_info
#endif

/*
use this if we need to use new
#pragma push_macro("new")
#undef new
#pragma pop_macro("new")
*/

// this is to override functions from memory.h
#ifdef AN_INSPECT_MEMORY_LEAKS
#define allocate_memory(size) allocate_memory_detect_memory_leaks(size, TXT(__FILE__), __LINE__, restore_memory_leak_suspect_file(), restore_memory_leak_suspect_line(), restore_memory_leak_info())
#define free_memory(pointer) free_memory_detect_memory_leaks(pointer)
#else
#define allocate_memory(size) allocate_memory_detect_memory_leaks(size)
#define free_memory(pointer) free_memory_detect_memory_leaks(pointer)
#endif

#ifdef AN_INSPECT_MEMORY_LEAKS
#define allocate_memory_for_memory_leaks(size) allocate_memory_for_memory_leaks_internal(size, TXT(__FILE__), __LINE__, restore_memory_leak_suspect_file(), restore_memory_leak_suspect_line(), restore_memory_leak_info())
#define free_memory_for_memory_leaks(pointer) free_memory_for_memory_leaks_internal(pointer)
#else
#define allocate_memory_for_memory_leaks(size) allocate_memory_for_memory_leaks_internal(size)
#define free_memory_for_memory_leaks(pointer) free_memory_for_memory_leaks_internal(pointer)
#endif

#ifdef AN_INSPECT_MEMORY_LEAKS
#define on_new_memory(size) on_new_internal(size, TXT(__FILE__), __LINE__, restore_memory_leak_suspect_file(), restore_memory_leak_suspect_line(), restore_memory_leak_info())
#define on_delete_memory(pointer) on_delete_internal(pointer)
#else
#define on_new_memory(size) on_new_internal(size)
#define on_delete_memory(pointer) on_delete_internal(pointer)
#endif

/**
 *	Structure to handle display of memory leaks
 */
struct ShowMemoryLeaks
{
public:
	static void output_recent_memory_allocations(int _howMany, bool _outputContent = false);

private:
	static ShowMemoryLeaks s_instance;

	ShowMemoryLeaks();
	~ShowMemoryLeaks();
	static void check_for_memory_leaks();
	static void output_memory_allocation(AllocationBook* ai, size_t _idx, bool _outputContent = false);
};

#else
	#ifndef AN_DONT_CHECK_MEMORY_LEAKS
	#error please include globalSettings.h
	#endif
	#define memory_leak_suspect
	#define forget_memory_leak_suspect
	#define memory_leak_info(txt)
	#define forget_memory_leak_info
#endif

// avoid clashing with memory (was in memory.h)
#ifndef AN_CHECK_MEMORY_LEAKS
#ifdef AN_MEMORY_STATS
#ifndef AN_RELEASE
void* __cdecl operator new(size_t _size);
void* __cdecl operator new[](size_t _size);
void operator delete(void* _pointer) _NOEXCEPT;
void operator delete[](void* _pointer) _NOEXCEPT;
#endif
#endif
#endif

inline void output_recent_memory_allocations(int _howMany, bool _outputContent = false)
{
#ifdef AN_CHECK_MEMORY_LEAKS
	ShowMemoryLeaks::output_recent_memory_allocations(_howMany, _outputContent);
#endif
}
