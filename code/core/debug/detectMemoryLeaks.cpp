#include "..\globalSettings.h"
#include "..\globalInclude.h"
#include "detectMemoryLeaks.h"
#include "..\memory\memory.h"
#include "..\utils.h"

#include "..\concurrency\counter.h"
#include "..\concurrency\spinLock.h"
#include "..\concurrency\scopedSpinLock.h"
#include "..\concurrency\threadSystemUtils.h"

#include "..\types\string.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

//#define AN_CHECK_COUNTERS

#ifdef AN_CHECK_MEMORY_LEAKS

#define MAX_FILENAME_LENGTH 64
#define MAX_INFO_LENGTH 64

#define ALIGNMENT 16
#define ALIGN(size)		(size + ALIGNMENT-1) & ~(ALIGNMENT-1)

#ifdef AN_ANDROID
#define DADABABE_BUFFER_SIZE 0
#define MINIMAL
#else
#define DADABABE_BUFFER_SIZE 32
#define WITH_LIST
#endif

#undef new
#undef allocate_memory
#undef free_memory

#include <stdlib.h>

#include "debug.h"

// all pointers should point at actual object, we move properly from object to AllocationBook

#ifdef AN_INSPECT_MEMORY_LEAKS
void* __cdecl operator new(size_t size) { void* memo = ::operator new(size, restore_file(), restore_line(), restore_memory_leak_suspect_file(), restore_memory_leak_suspect_line(), restore_memory_leak_info()); clear_memory_leak_suspect(); return memo; }
void* __cdecl operator new[](size_t size) { void* memo = ::operator new[](size, restore_file(), restore_line(), restore_memory_leak_suspect_file(), restore_memory_leak_suspect_line(), restore_memory_leak_info()); clear_memory_leak_suspect(); return memo; }
#else
void* __cdecl operator new(size_t size) { void* memo = ::operator new(size, true); clear_memory_leak_suspect(); return memo; }
void* __cdecl operator new[](size_t size) { void* memo = ::operator new[](size, true); clear_memory_leak_suspect(); return memo; }
#endif

#define DETECT_MEMORY_LEAKS_THREAD_COUNT 64
#ifdef AN_INSPECT_MEMORY_LEAKS
#define INSPECT_MEMORY_LEAK_STRING_LENGTH 512
tchar storedFiles[DETECT_MEMORY_LEAKS_THREAD_COUNT][INSPECT_MEMORY_LEAK_STRING_LENGTH];
int storedLines[DETECT_MEMORY_LEAKS_THREAD_COUNT];
tchar storedMemoryLeakSuspectFiles[DETECT_MEMORY_LEAKS_THREAD_COUNT][INSPECT_MEMORY_LEAK_STRING_LENGTH];
int storedMemoryLeakSuspectLines[DETECT_MEMORY_LEAKS_THREAD_COUNT]; // clear if 0
int storedMemoryLeakSuspectStack[DETECT_MEMORY_LEAKS_THREAD_COUNT];
tchar storedMemoryLeakInfo[DETECT_MEMORY_LEAKS_THREAD_COUNT][INSPECT_MEMORY_LEAK_STRING_LENGTH];
int storedMemoryLeakInfoStack[DETECT_MEMORY_LEAKS_THREAD_COUNT];
#endif
int threadSystemIds[DETECT_MEMORY_LEAKS_THREAD_COUNT];
int supressedMemoryLeakDetection[DETECT_MEMORY_LEAKS_THREAD_COUNT];
int registeredThreads = 0;
bool mainThreadRunning = false;
Concurrency::SpinLock registeredThreadLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);

int get_store_from_thread_id()
{
	int32 threadSystemId = Concurrency::ThreadSystemUtils::get_current_thread_system_id();
	for_count(int, i, registeredThreads)
	{
		if (threadSystemId == threadSystemIds[i])
		{
			return i;
		}
	}
	// add new one
	registeredThreadLock.acquire();
	an_assert_immediate(registeredThreads < DETECT_MEMORY_LEAKS_THREAD_COUNT, TXT("exceeded number of threads, increase DETECT_MEMORY_LEAKS_THREAD_COUNT"));
	threadSystemIds[registeredThreads] = threadSystemId;
	supressedMemoryLeakDetection[registeredThreads] = mainThreadRunning; // by default other threads have supressed memory leak detection, check run_thread_func
																		 // this is due to fact that some third party libs are allocating and deallocating memory
#ifdef AN_INSPECT_MEMORY_LEAKS
	storedMemoryLeakSuspectLines[registeredThreads] = 0;
	storedMemoryLeakSuspectStack[registeredThreads] = 0;
	storedMemoryLeakInfoStack[registeredThreads] = 0;
#endif
	++registeredThreads;
	int retVal = registeredThreads - 1;
	registeredThreadLock.release();
	return retVal;
}

void mark_main_thread_running()
{
	// to register this thread (and previous threads, we may be running on another thread than the statics were allocated)
	get_store_from_thread_id();
	mainThreadRunning = true;
}

void* allocate_actual_memory(size_t size)
{
	// undefined, will call function in memory.h
	return allocate_memory(size);
}

void free_actual_memory(void* pointer)
{
	// undefined, will call function in memory.h
	free_memory(pointer);
}

#ifdef AN_INSPECT_MEMORY_LEAKS
static int find_to_fit(tchar const * _file)
{
	int len = (int)tstrlen(_file);
	if (len < MAX_FILENAME_LENGTH - 1)
	{
		return 0;
	}
	for_count(int, idx, len - 2)
	{
		if (_file[idx+0] == '\\' &&
			(len - idx) < MAX_FILENAME_LENGTH - 2)
		{
			return idx + 1;
		}
	}
	return 0;
}

static void copy_to_fit(tchar * _dest, tchar const * _src, int _maxLength)
{
	an_assert(_dest && _src);
	int len = min((int)tstrlen(_src), _maxLength - 1);
	memory_copy(_dest, _src, sizeof(tchar) * len);
	_dest[len] = 0;
}
#endif

/**
 *	Structure that stores information about all allocations
 */
struct AllocationBook
{
	friend struct ShowMemoryLeaks;

public:
	static void mark_static_allocations_done()
	{
#ifndef MINIMAL
		Concurrency::ScopedSpinLock lock(s_lock());
		AllocationBook* ai = s_first();
		while (ai)
		{
			ai->isStatic = true;
			ai = ai->next;
			s_staticCount().increase();
		}
		an_assert_immediate(s_staticCount() == s_count());
#endif
	}

	static void mark_static_allocations_left()
	{
		Concurrency::ScopedSpinLock lock(s_lock());
		an_assert_immediate(s_staticCount() <= s_count());
	}

	static AllocationBook * create_new_book(size_t a_size)
	{
		void * allocated = allocate_actual_memory(a_size + sizeof(AllocationBook) + 2 * sizeof(int) * DADABABE_BUFFER_SIZE);
		if (allocated)
		{
			setup_new((AllocationBook*)allocated, a_size);
		}
		an_assert(allocated, TXT("OOM"));
		{
			Concurrency::ScopedSpinLock lock(s_lock());
			s_allocatedCount().increase();
		}
#ifndef MINIMAL
		((AllocationBook*)allocated)->created = true;
		((AllocationBook*)allocated)->isStatic = false;
#endif
		report_memory_allocated(a_size);
		return (AllocationBook*)allocated;
	}

	static void delete_book(AllocationBook* ai)
	{
#ifndef MINIMAL
		an_assert_immediate(ai->created);
		ai->created = false;
#endif
		report_memory_freed(ai->size);
		{
			Concurrency::ScopedSpinLock lock(s_lock());
			s_allocatedCount().decrease();
			an_assert_immediate(s_allocatedCount().get() >= 0);
		}
		free_actual_memory(ai);
	}

	static void setup_new(AllocationBook* ai, size_t a_size)
	{
		ai->face = (int32)0x00face00;
		ai->babacafe = (int32)0xbabacafe;
		ai->checkedIn = false;
#ifdef WITH_LIST
		ai->next = nullptr;
		ai->prev = nullptr;
#endif
		ai->size = a_size;
		ai->pointer = ((int8*)ai) + sizeof(AllocationBook) + sizeof(int) * DADABABE_BUFFER_SIZE;
		if (DADABABE_BUFFER_SIZE > 0)
		{
			int8* preBufferP = (int8*)ai->pointer - sizeof(int) * DADABABE_BUFFER_SIZE;
			int8* postBufferP = (int8*)ai->pointer + ai->size;
			for (int i = 0; i < DADABABE_BUFFER_SIZE; ++i)
			{
				*((int*)preBufferP) = 0xdadababe;
				*((int*)postBufferP) = 0x00dada00;
				preBufferP += sizeof(int);
				postBufferP += sizeof(int);
			}
		}
		return;
	}

#ifdef AN_INSPECT_MEMORY_LEAKS
	static void check_in(AllocationBook *ai, tchar const *a_file, int a_line, tchar const* a_memoryLeakSuspectFile, int a_memoryLeakSuspectLine, tchar const * a_memoryLeakInfo)
#else
	static void check_in(AllocationBook *ai)
#endif
	{
		Concurrency::ScopedSpinLock lock(s_lock());
		s_count().increase();
		an_assert_immediate(ai->face == (int32)0x00face00);
		an_assert_immediate(ai->babacafe == (int32)0xbabacafe);
		an_assert_immediate(!ai->checkedIn);
#ifdef WITH_LIST
		an_assert_immediate(!ai->next);
		an_assert_immediate(!ai->prev);
#endif
		/*
		// code to catch buffer overruning
		if (String::does_contain_icase(a_file, TXT("ailogic_solarflower.h")) && a_line == 34)
		{
			int size = ai->size;
			int8* postBufferP = (int8*)ai->pointer + ai->size;
			int* postBuffer = ((int*)postBufferP);
			an_assert_immediate(*((int*)postBufferP) == 0x00dada00);
			AN_BREAK; // put a trap there
		}
		*/
		if (!ai->checkedIn)
		{
			ai->checkedIn = true;
#ifdef WITH_LIST
			ai->next = nullptr;
			ai->prev = s_last();
			if (s_last())
			{
				s_last()->next = ai;
			}
			else
			{
				s_first() = ai;
			}
			s_last() = ai;
#endif

#ifdef AN_CHECK_COUNTERS
			int counted = 0;
#ifdef WITH_LIST
			auto* go = s_first();
			while (go)
			{
				++counted;
				go = go->next;
			}
#endif
			an_assert_immediate(s_count() == counted);
#endif

#ifdef WITH_LIST
			an_assert_immediate(!s_last() || !s_last()->next);
			an_assert_immediate(!s_first() || !s_first()->prev);
#endif

#ifdef AN_INSPECT_MEMORY_LEAKS
			tchar const * a_file_cut = &a_file[find_to_fit(a_file)];
			/*
			if (tstrlen(a_file_cut) > MAX_FILENAME_LENGTH)
			{
				int len = tstrlen(a_file_cut);
				tchar const * show = &a_file_cut[max(0, tstrlen(a_file_cut) - (MAX_FILENAME_LENGTH - 1))];
				int showlen = tstrlen(show);
				AN_BREAK;
			}
			*/ 
#ifdef AN_CLANG
			tsprintf(ai->file, MAX_FILENAME_LENGTH, TXT("%S"), a_file_cut);
#else
			tsprintf_s(ai->file, TXT("%s"), a_file_cut);
#endif
			ai->line = a_line;
			ai->id = s_id++;
			if (s_doCheckpoints())
			{
				ai->threadId = get_store_from_thread_id();
			}
			tchar const * a_memoryLeakSuspectFile_cut = &a_memoryLeakSuspectFile[find_to_fit(a_memoryLeakSuspectFile)];
			/*
			if (a_memoryLeakSuspectFile_cut && tstrlen(a_memoryLeakSuspectFile_cut) > MAX_FILENAME_LENGTH)
			{
				int len = tstrlen(a_memoryLeakSuspectFile_cut);
				tchar const * show = &a_memoryLeakSuspectFile[max(0, tstrlen(a_memoryLeakSuspectFile) - (MAX_FILENAME_LENGTH - 1))];
				int showlen = tstrlen(show);
				AN_BREAK;
			}
			*/
#ifdef AN_CLANG
			tsprintf(ai->memoryLeakSuspectFile, MAX_FILENAME_LENGTH, TXT("%S"), a_memoryLeakSuspectFile_cut ? a_memoryLeakSuspectFile_cut : TXT("??"));
#else
			tsprintf_s(ai->memoryLeakSuspectFile, TXT("%s"), a_memoryLeakSuspectFile_cut ? a_memoryLeakSuspectFile_cut : TXT("??"));
#endif
			ai->memoryLeakSuspectLine = a_memoryLeakSuspectLine;
			copy_to_fit(ai->memoryLeakInfo, a_memoryLeakInfo, MAX_INFO_LENGTH);
#endif
		}
	}

	static void detect_memory_corruption_internal(AllocationBook *ai)
	{
		an_assert_immediate(s_count().get() > 0);
#ifndef MINIMAL
		if (ai->isStatic)
		{
			s_staticCount().decrease();
			an_assert_immediate(s_staticCount().get() >= 0);
		}
#endif
		an_assert_immediate(ai->face == (int32)0x00face00);
		an_assert_immediate(ai->babacafe == (int32)0xbabacafe);
		an_assert_immediate(ai->checkedIn);
		if (ai->checkedIn && DADABABE_BUFFER_SIZE > 0)
		{
			// check buffers
			int8* preBufferP = (int8*)ai->pointer - sizeof(int) * DADABABE_BUFFER_SIZE;
			int8* postBufferP = (int8*)ai->pointer + ai->size;
			for (int i = 0; i < DADABABE_BUFFER_SIZE; ++i)
			{
				an_assert_immediate(*((int*)preBufferP) == 0xdadababe);
				an_assert_immediate(*((int*)postBufferP) == 0x00dada00);
				preBufferP += sizeof(int);
				postBufferP += sizeof(int);
			}
		}
	}

	static void detect_memory_corruption(AllocationBook *ai)
	{
		Concurrency::ScopedSpinLock lock(s_lock());
		detect_memory_corruption_internal(ai);
	}

	static void check_out(AllocationBook *ai)
	{
		Concurrency::ScopedSpinLock lock(s_lock());
		detect_memory_corruption_internal(ai);
		if (ai->checkedIn)
		{
#ifdef WITH_LIST
			if (s_doCheckpoints() &&
				ai == s_checkpoint())
			{
				if (ai->prev)
				{
					s_checkpoint() = ai->prev;
				}
				else
				{
					s_checkpoint() = nullptr;
				}
			}
#endif

			ai->checkedIn = false;
#ifdef WITH_LIST
			if (ai->next)
			{
				ai->next->prev = ai->prev;
			}
			else
			{
				s_last() = ai->prev;
			}
			if (ai->prev)
			{
				ai->prev->next = ai->next;
			}
			else
			{
				s_first() = ai->next;
			}
			ai->next = nullptr;
			ai->prev = nullptr;
			an_assert_immediate(!s_last() || !s_last()->next);
			an_assert_immediate(!s_first() || !s_first()->prev);
#endif
			s_count().decrease();
			an_assert_immediate(s_count().get() >= 0);
#ifdef AN_CHECK_COUNTERS
			int counted = 0;
#ifdef WITH_LIST
			auto* go = s_first();
			while (go)
			{
				++counted;
				go = go->next;
			}
#endif
			an_assert_immediate(s_count() == counted);
#endif
		}
	}

	static AllocationBook * find(void *a_pointer) // finds beginning of object
	{
		if (a_pointer == nullptr)
		{
			return nullptr;
		}
		static size_t babacafeOffset = 0;
		if (babacafeOffset == 0)
		{
			AllocationBook temp;
			babacafeOffset = (int8*)&temp.babacafe - (int8*)&temp;
		}
		// find by going in reverse to find 4 bytes with babacafe - the odds for such value on the way are unlikely, but in any case we have two guardians to make it even less likely
		int8* pointer = (int8*)a_pointer;
		pointer -= 4;
		int tryCount = 0;
		while (tryCount < 16384) // 16k object?! maybe one day ;)
		{
			if (*((int32*)pointer) == (int32)0xbabacafe &&
				*((int32*)(pointer - babacafeOffset)) == (int32)0x00face00)
			{
				pointer -= babacafeOffset;
				AllocationBook* book = (AllocationBook*)pointer;
				an_assert_immediate(book->face == (int32)0x00face00);
				an_assert_immediate(book->babacafe == (int32)0xbabacafe);
				return book;
			}
			--pointer;
			++tryCount;
		}
		return nullptr;
	}

	void *get_pointer()
	{
		return pointer;
	}

	static void start_checkpoint()
	{
		Concurrency::ScopedSpinLock lock(s_lock());

#ifdef WITH_LIST
		s_doCheckpoints() = true;
		s_checkpoint() = s_last();
#endif
	}

	static void end_checkpoint(bool _allThreads)
	{
		Concurrency::ScopedSpinLock lock(s_lock());
#ifdef AN_INSPECT_MEMORY_LEAKS
		if (s_doCheckpoints())
		{
			supress_memory_leak_detection();
#ifdef WITH_LIST
			AllocationBook * from = s_checkpoint();
			AllocationBook * to = s_last();
			AllocationBook * ai = from;
			if (ai)
			{
				ai = ai->next; // we do not want last one
				if (ai && ai != to)
				{
					bool firstOne = true;
					int threadId = get_store_from_thread_id();
					while (ai && ai != to)
					{
						if (_allThreads || ai->threadId == threadId)
						{
							if (firstOne)
							{
								detected_memory_leaks_output(TXT("allocated since checkpoint"));
								firstOne = false;
							}
							if (ai->memoryLeakSuspectLine > 0)
							{
								detected_memory_leaks_output(TXT("  %S%i bytes allocated at 0x%X by %S (%i) [mls: %04i in %S] %S"), ai->isStatic ? TXT("[static] ") : TXT(""), ai->size, ai->pointer, ai->file, ai->line, ai->memoryLeakSuspectLine, ai->memoryLeakSuspectFile, ai->memoryLeakInfo);
							}
							else
							{
								detected_memory_leaks_output(TXT("  %S%i bytes allocated at 0x%X by %S (%i) %S"), ai->isStatic ? TXT("[static] ") : TXT(""), ai->size, ai->pointer, ai->file, ai->line, ai->memoryLeakInfo);
							}
						}
						ai = ai->next;
					}
				}
			}
#endif
			resume_memory_leak_detection();
		}
#endif
#ifdef WITH_LIST
		s_doCheckpoints() = false;
#endif
	}

private:
	static Concurrency::SpinLock& s_lock() { static Concurrency::SpinLock lock;
#ifdef AN_CONCURRENCY_STATS
		lock.do_not_report_stats();
#endif
		return lock; }
	static Concurrency::Counter& s_count() { static Concurrency::Counter counter; return counter; }
	static Concurrency::Counter& s_allocatedCount() { static Concurrency::Counter counter; return counter; }
	static Concurrency::Counter& s_staticCount() { static Concurrency::Counter counter; return counter; }
#ifdef WITH_LIST
	static AllocationBook*& s_first() { static AllocationBook* ptr = nullptr; return ptr; }
	static AllocationBook*& s_last() { static AllocationBook* ptr = nullptr; return ptr; }
	static AllocationBook*& s_checkpoint() { static AllocationBook* ptr = nullptr; return ptr; }
	static bool & s_doCheckpoints() { static bool doCheckpoints = false; return doCheckpoints; }
#endif

private:
	int32 face = 0x00face00; // to make it possible to find allocation - has to be first
#ifdef WITH_LIST
	AllocationBook *next;
    AllocationBook *prev;
#endif
#ifdef AN_INSPECT_MEMORY_LEAKS
	tchar file[MAX_FILENAME_LENGTH];
    int line;
	static int s_id;
	int id; //
	int threadId; // for checkpoint only
	tchar memoryLeakSuspectFile[MAX_FILENAME_LENGTH];
    int memoryLeakSuspectLine; // clear if 0
	tchar memoryLeakInfo[MAX_INFO_LENGTH];
#endif
	size_t size; // size of data
	void *pointer; // this is pointer to actual data but there are also buffers around it filled with 0xdadababe and 0x00dada00 (dadababe 00dada00 buffers)
#ifndef MINIMAL
	bool created = false;
	bool isStatic = false;
#endif
	bool checkedIn = false;
	int32 babacafe = 0xbabacafe; // to make it possible to find allocation (it will look for babacafe and face at proper offset - the odds are really low that this will be reproduced randomly)

};

#ifdef AN_INSPECT_MEMORY_LEAKS
int AllocationBook::s_id = 0;
#endif

// manage memory for memory leaks - create new or delete existing book

#ifdef AN_INSPECT_MEMORY_LEAKS
void* allocate_memory_for_memory_leaks_internal(size_t size, tchar const * file, int line, const tchar* memoryLeakSuspectFile, int memoryLeakSuspectLine, tchar const * memoryLeakInfo)
#else
void* allocate_memory_for_memory_leaks_internal(size_t size)
#endif
{
	if (!is_memory_leak_detection_supressed())
	{
		AllocationBook* ai = AllocationBook::create_new_book(size);
		if (ai == nullptr)
		{
#ifdef AN_INSPECT_MEMORY_LEAKS
			if (memoryLeakSuspectFile)
			{
				an_assert_immediate(false, TXT("out of memory (%iB) @ %04i in %S [mls: %04i in %S] %S"), size, line, file, memoryLeakSuspectLine, memoryLeakSuspectFile, memoryLeakInfo);
			}
			else
			{
				an_assert_immediate(false, TXT("out of memory (%iB) @ %04i in %S %S"), size, line, file, memoryLeakInfo);
			}
#else
			an_assert_immediate(false, TXT("out of memory (%iB)"), size);
#endif
			return nullptr;
		}
		else
		{
			return ai->get_pointer();
		}
	}
	else
	{
		size_t s = ALIGN(size);
		void* pointer = allocate_actual_memory(s);
		if (pointer == nullptr)
		{
#ifdef AN_INSPECT_MEMORY_LEAKS
			if (memoryLeakSuspectFile)
			{
				an_assert_immediate(false, TXT("out of memory (%iB) @ %04i in %S [mls: %04i in %S] %S"), size, line, file, memoryLeakSuspectLine, memoryLeakSuspectFile, memoryLeakInfo);
			}
			else
			{
				an_assert_immediate(false, TXT("out of memory (%iB) @ %04i in %S %S"), size, line, file, memoryLeakInfo);
			}
#else
			an_assert_immediate(false, TXT("out of memory (%iB)"), size);
#endif
		}
		return pointer;
	}
}

void free_memory_for_memory_leaks_internal(void* pointer)
{
	if (!is_memory_leak_detection_supressed())
	{
		AllocationBook * ai = AllocationBook::find(pointer);
		if (!ai)
		{
			an_assert_immediate(false, TXT("trying to delete invalid pointer"));
		}
		else
		{
			AllocationBook::delete_book(ai);
		}
	}
	else
	{
		free_actual_memory(pointer);
	}
}

// check in on new and check out on delete, don't manage memory!

#ifdef AN_INSPECT_MEMORY_LEAKS
void on_new_internal(void* pointer, tchar const * file, int line, const tchar* memoryLeakSuspectFile, int memoryLeakSuspectLine, tchar const * memoryLeakInfo)
#else
void on_new_internal(void* pointer)
#endif
{
	if (!pointer)
	{
		return;
	}
	if (is_memory_leak_detection_supressed())
	{
		return;
	}
#ifdef AN_MEMORY_STATS
	store_new_call();
#endif
	AllocationBook* ai = AllocationBook::find(pointer);
	if (!ai)
	{
		an_assert_immediate(false, TXT("trying to add new, but no memory arranged"));
	}
	else
	{
#ifdef AN_INSPECT_MEMORY_LEAKS
		AllocationBook::check_in(ai, file, line, memoryLeakSuspectFile, memoryLeakSuspectLine, memoryLeakInfo);
#else
		AllocationBook::check_in(ai);
#endif
	}
}

void on_delete_internal(void* pointer)
{
	if (!pointer)
	{
		return;
	}
	if (is_memory_leak_detection_supressed())
	{
		return;
	}
#ifdef AN_MEMORY_STATS
	store_delete_call();
#endif
	AllocationBook * ai = AllocationBook::find(pointer);
	if (!ai)
	{
		an_assert_immediate(false, TXT("trying to delete invalid pointer"));
	}
	else
	{
		AllocationBook::check_out(ai);
	}
}

void detect_memory_corruption(void* pointer)
{
	if (!pointer)
	{
		return;
	}
	AllocationBook * ai = AllocationBook::find(pointer);
	if (!ai)
	{
		an_assert_immediate(false, TXT("trying to delete invalid pointer"));
	}
	else
	{
		AllocationBook::detect_memory_corruption(ai);
	}
}


// arrange memory and check in/out

#ifdef AN_INSPECT_MEMORY_LEAKS
void* create_new_and_check_in(size_t size, tchar const * file, int line, const tchar* memoryLeakSuspectFile, int memoryLeakSuspectLine, tchar const * memoryLeakInfo)
#else
void* create_new_and_check_in(size_t size)
#endif
{
#ifdef AN_INSPECT_MEMORY_LEAKS
	void* pointer = allocate_memory_for_memory_leaks_internal(size, file, line, memoryLeakSuspectFile, memoryLeakSuspectLine, memoryLeakInfo);
#else
	void* pointer = allocate_memory_for_memory_leaks_internal(size);
#endif
	if (pointer)
	{
		if (!is_memory_leak_detection_supressed())
		{
#ifdef AN_INSPECT_MEMORY_LEAKS
			on_new_internal(pointer, file, line, memoryLeakSuspectFile, memoryLeakSuspectLine, memoryLeakInfo);
#else
			on_new_internal(pointer);
#endif
		}
	}
	return pointer;
}

void check_out_and_delete(void* pointer)
{
	if (!is_memory_leak_detection_supressed())
	{
		on_delete_internal(pointer);
	}
	free_memory_for_memory_leaks_internal(pointer);
}

// implement operators that redirect allocation/deallocation to AllocationBook
#ifdef AN_INSPECT_MEMORY_LEAKS
void* operator new(size_t size, tchar const * file, int line, const tchar* memoryLeakSuspectFile, int memoryLeakSuspectLine, tchar const * memoryLeakInfo)
#else
void* operator new(size_t size, bool _inspectMemoryLeaks)
#endif
{
#ifdef AN_INSPECT_MEMORY_LEAKS
	return create_new_and_check_in(size, file, line, memoryLeakSuspectFile, memoryLeakSuspectLine, memoryLeakInfo);
#else
	return create_new_and_check_in(size);
#endif
}

#ifdef AN_INSPECT_MEMORY_LEAKS
void * operator new[](size_t size, tchar const * file, int line, const tchar* memoryLeakSuspectFile, int memoryLeakSuspectLine, tchar const * memoryLeakInfo)
#else
void * operator new[](size_t size, bool _inspectMemoryLeaks)
#endif
{
#ifdef AN_INSPECT_MEMORY_LEAKS
	return create_new_and_check_in(size, file, line, memoryLeakSuspectFile, memoryLeakSuspectLine, memoryLeakInfo);
#else
	return create_new_and_check_in(size);
#endif
}

#ifdef AN_INSPECT_MEMORY_LEAKS
void* allocate_memory_detect_memory_leaks(size_t size, const tchar* file, int line, const tchar* memoryLeakSuspectFile, int memoryLeakSuspectLine, tchar const * memoryLeakInfo)
#else
void* allocate_memory_detect_memory_leaks(size_t size)
#endif
{
#ifdef AN_INSPECT_MEMORY_LEAKS
	return create_new_and_check_in(size, file, line, memoryLeakSuspectFile, memoryLeakSuspectLine, memoryLeakInfo);
#else
	return create_new_and_check_in(size);
#endif
}

void free_memory_detect_memory_leaks(void* pointer)
{
	check_out_and_delete(pointer);
}

void operator delete(void * pointer) AN_CLANG_NO_EXCEPT
{
	check_out_and_delete(pointer);
}

void operator delete[](void * pointer) AN_CLANG_NO_EXCEPT
{
	check_out_and_delete(pointer);
}

#ifdef AN_INSPECT_MEMORY_LEAKS
void operator delete(void* pointer, tchar const * file, int line)
#else
void operator delete(void* pointer, bool _inspectMemoryLeaks)
#endif
{
	check_out_and_delete(pointer);
}

#ifdef AN_INSPECT_MEMORY_LEAKS
void operator delete[](void* pointer, tchar const * file, int line)
#else
void operator delete[](void* pointer, bool _inspectMemoryLeaks)
#endif
{
	check_out_and_delete(pointer);
}

bool store_file_and_line(const tchar* file, int line)
{
#ifdef AN_INSPECT_MEMORY_LEAKS
	int threadId = get_store_from_thread_id();
#ifdef AN_CLANG
	tsprintf(storedFiles[threadId], INSPECT_MEMORY_LEAK_STRING_LENGTH, file);
#else
	tsprintf_s(storedFiles[threadId], file);
#endif
	storedLines[threadId] = line;
#endif
	return false;
}

bool store_memory_leak_suspect_file_and_line(const tchar* file, int line)
{
	int threadId = get_store_from_thread_id();
	an_assert_immediate(threadId >= 0 && threadId < DETECT_MEMORY_LEAKS_THREAD_COUNT);
	if (line != 0)
	{
#ifdef AN_INSPECT_MEMORY_LEAKS
		if (storedMemoryLeakSuspectStack[threadId] == 0)
		{
#ifdef AN_CLANG
			tsprintf(storedMemoryLeakSuspectFiles[threadId], INSPECT_MEMORY_LEAK_STRING_LENGTH, file ? file : TXT("??"));
#else
			tsprintf_s(storedMemoryLeakSuspectFiles[threadId], file ? file : TXT("??"));
#endif
			storedMemoryLeakSuspectLines[threadId] = line;
		}
		++storedMemoryLeakSuspectStack[threadId];
#endif
	}
	else if (line == 0)
	{
#ifdef AN_INSPECT_MEMORY_LEAKS
		--storedMemoryLeakSuspectStack[threadId];
		if (storedMemoryLeakSuspectStack[threadId] == 0)
		{
#ifdef AN_CLANG
			tsprintf(storedMemoryLeakSuspectFiles[threadId], INSPECT_MEMORY_LEAK_STRING_LENGTH, file ? file : TXT("??"));
#else
			tsprintf_s(storedMemoryLeakSuspectFiles[threadId], file ? file : TXT("??"));
#endif
			storedMemoryLeakSuspectLines[threadId] = line;
		}
#endif
	}
	return false;
}

bool store_memory_leak_info(tchar const * _info)
{
#ifdef AN_INSPECT_MEMORY_LEAKS
	int threadId = get_store_from_thread_id();
	if (_info)
	{
		if (storedMemoryLeakInfoStack[threadId] == 0)
		{
			copy_to_fit(storedMemoryLeakInfo[threadId], _info, INSPECT_MEMORY_LEAK_STRING_LENGTH);
		}
		++storedMemoryLeakInfoStack[threadId];
	}
	else
	{
		--storedMemoryLeakInfoStack[threadId];
		if (storedMemoryLeakInfoStack[threadId] == 0)
		{
#ifdef AN_CLANG
			tsprintf(storedMemoryLeakInfo[threadId], INSPECT_MEMORY_LEAK_STRING_LENGTH, TXT(""));
#else
			tsprintf_s(storedMemoryLeakInfo[threadId], TXT(""));
#endif
		}
	}
#endif
	return false;
}

void supress_memory_leak_detection()
{
#ifdef AN_CHECK_MEMORY_LEAKS
	int threadId = get_store_from_thread_id();
	++ supressedMemoryLeakDetection[threadId];
#endif
}

void resume_memory_leak_detection()
{
#ifdef AN_CHECK_MEMORY_LEAKS
	int threadId = get_store_from_thread_id();
	-- supressedMemoryLeakDetection[threadId];
#endif
}

bool is_memory_leak_detection_supressed()
{
	int threadId = get_store_from_thread_id();
	return supressedMemoryLeakDetection[threadId] != 0;
}

const tchar* restore_file()
{
#ifdef AN_INSPECT_MEMORY_LEAKS
	return storedFiles[get_store_from_thread_id()];
#else
	return TXT("");
#endif
}

int restore_line()
{
#ifdef AN_INSPECT_MEMORY_LEAKS
	return storedLines[get_store_from_thread_id()];
#else
	return 0;
#endif
}

const tchar* restore_memory_leak_suspect_file()
{
#ifdef AN_INSPECT_MEMORY_LEAKS
	return storedMemoryLeakSuspectFiles[get_store_from_thread_id()];
#else
	return TXT("");
#endif
}

int restore_memory_leak_suspect_line()
{
#ifdef AN_INSPECT_MEMORY_LEAKS
	return storedMemoryLeakSuspectLines[get_store_from_thread_id()];
#else
	return 0;
#endif
}

void clear_memory_leak_suspect()
{
	// do not auto clear now
	/*
	if (!is_memory_leak_detection_supressed())
	{
		store_memory_leak_suspect_file_and_line(nullptr, 0);
	}
	*/
}

const tchar* restore_memory_leak_info()
{
#ifdef AN_INSPECT_MEMORY_LEAKS
	return storedMemoryLeakInfo[get_store_from_thread_id()];
#else
	return TXT("");
#endif
}

#undef DETECT_MEMORY_LEAKS_THREAD_COUNT

// show allocated info

ShowMemoryLeaks ShowMemoryLeaks::s_instance;

ShowMemoryLeaks::ShowMemoryLeaks()
{
	//atexit(&check_for_memory_leaks);
}

ShowMemoryLeaks::~ShowMemoryLeaks()
{
	check_for_memory_leaks();
}

void ShowMemoryLeaks::check_for_memory_leaks()
{
	if (AllocationBook::s_count().get() || AllocationBook::s_allocatedCount().get())
	{
		detected_memory_leaks_output(TXT("memory leaks detected... (%i, of which %i static? don't trust static count)"), AllocationBook::s_count().get(), AllocationBook::s_staticCount().get());
		if (AllocationBook::s_allocatedCount().get() != AllocationBook::s_count().get())
		{
			detected_memory_leaks_output(TXT("  allocated count mismatch! %i v %i"), AllocationBook::s_allocatedCount().get(), AllocationBook::s_count().get());
		}
#ifdef WITH_LIST
		size_t bytesLost = 0;
		AllocationBook *ai = AllocationBook::s_first();
		size_t left = max(AllocationBook::s_count().get(), AllocationBook::s_allocatedCount().get());
		size_t idx = 0;
		while (ai && left > 0)
		{
			--left; // to not run into file or anything else we would create on the way
			output_memory_allocation(ai, idx ++);
			bytesLost += ai->size;
			ai = ai->next;
		}
		detected_memory_leaks_output(TXT("%i bytes lost (total)"), bytesLost);
#else
		detected_memory_leaks_output(TXT("%i bytes lost (total)"), get_memory_allocated_checked_in());
#endif
		detected_memory_leaks_end();
#ifdef AN_ASSERT
		an_assert_immediate(AllocationBook::s_count().get() == 0, TXT("memory leak... (you may want to check further asserts of static objects)"));
#else
		AN_BREAK;
#endif
	}
}

void ShowMemoryLeaks::output_recent_memory_allocations(int _howMany, bool _outputContent)
{
	Concurrency::ScopedSpinLock lock(AllocationBook::s_lock());

	static size_t lastCount = 0;
	static size_t lastSize = 0;
	static size_t lastTotal = 0;
	size_t currCount = get_memory_allocated_count();
	size_t currSize = get_memory_allocated_checked_in();
	size_t currTotal = get_memory_allocated_total();
	size_t diffCount = currCount - lastCount;
	size_t diffSize = currSize - lastSize;
	size_t diffTotal = currTotal - lastTotal;
	detected_memory_leaks_output(TXT(""));
	detected_memory_leaks_output(TXT(""));
	detected_memory_leaks_output(TXT("allocated memory"));
	detected_memory_leaks_output(TXT("  memory allocations      : %i (%S%i)"), currCount, diffCount>0? TXT("+") : TXT(""), diffCount);
	detected_memory_leaks_output(TXT("  memory used by game     : %12iB (%S%iB)"), currSize, diffSize>0? TXT("+") : TXT(""), diffSize);
	detected_memory_leaks_output(TXT("     with allocation info : %12iB (%S%iB)"), currTotal, diffTotal>0? TXT("+") : TXT(""), diffTotal);
	detected_memory_leaks_output(TXT("recent memory allocations"));

	lastCount = currCount;
	lastSize = currSize;
	lastTotal = currTotal;

	// report in order!
#ifdef WITH_LIST
	AllocationBook* ai = AllocationBook::s_last();
	size_t left = (size_t)_howMany;
	size_t reportLeft = 0;
	size_t idx = get_memory_allocated_count();
	while (ai->prev && left > 0)
	{
		--left;
		--idx;
		++reportLeft;
		ai = ai->prev;
	}
	while (ai && reportLeft > 0)
	{
		--reportLeft;
		output_memory_allocation(ai, idx ++, _outputContent);

		ai = ai->next;
	}
#endif

	detected_memory_leaks_output(TXT(""));
	detected_memory_leaks_output(TXT(""));
}

void ShowMemoryLeaks::output_memory_allocation(AllocationBook* ai, size_t _idx, bool _outputContent)
{
	tchar insight[65];
	// treat as a tchar array
	int maxInsight = 64;
	{
		int count = 0;
		tchar const* src = (tchar*)ai->pointer;
		tchar * dst = insight;
		while (*src != 0 && count < maxInsight && count * sizeof(tchar) < ai->size)
		{
			*dst = clamp<tchar>(*src, 32, 128);
			++dst;
			++src;
			++count;
		}
		*dst = 0;
	}
#ifdef AN_INSPECT_MEMORY_LEAKS
	if (ai->memoryLeakSuspectLine > 0)
	{
#ifndef MINIMAL
		detected_memory_leaks_output(TXT("  %S[%08i:%08i] %i bytes allocated at 0x%X by %S (%i) [mls: %04i in %S] %S [%S]"),
			ai->isStatic ? TXT("[static] ") : TXT(""),
			_idx, ai->id,
			ai->size, ai->pointer, ai->file, ai->line, ai->memoryLeakSuspectLine, ai->memoryLeakSuspectFile, ai->memoryLeakInfo, insight);
#else
		detected_memory_leaks_output(TXT("  %S%i bytes allocated at 0x%X by %S (%i) [mls: %04i in %S] %S [%S]"),
			TXT(""),
			ai->size, ai->pointer, ai->file, ai->line, ai->memoryLeakSuspectLine, ai->memoryLeakSuspectFile, ai->memoryLeakInfo, insight);
#endif
	}
	else
	{
#ifndef MINIMAL
		detected_memory_leaks_output(TXT("  %S[%08i:%08i] %i bytes allocated at 0x%X by %S (%i) %S [%S]"),
			ai->isStatic ? TXT("[static] ") : TXT(""),
			_idx, ai->id,
			ai->size, ai->pointer, ai->file, ai->line, ai->memoryLeakInfo, insight);
#else
		detected_memory_leaks_output(TXT("  %S%i bytes allocated at 0x%X by %S (%i) %S [%S]"),
			TXT(""),
			ai->size, ai->pointer, ai->file, ai->line, ai->memoryLeakInfo, insight);
#endif
	}
#else
	detected_memory_leaks_output(TXT("  %i bytes allocated at 0x%X"), ai->size, ai->pointer);
#endif
	if (_outputContent)
	{
		bool requiresHeader = true;
		for (uint i = 0; i < 64 && i < ai->size; i++)
		{
			int8 const& b = ((int8 const*)ai->pointer)[i];
			if (b >= 32 && b <= 127)
			{
				if (requiresHeader)
				{
					trace(TXT("    :"));
					requiresHeader = false;
				}
				trace(TXT("%c"), b);
			}
		}
		if (!requiresHeader)
		{
			output(TXT(":"));
		}
	}
}
//

void mark_static_allocations_done_for_memory_leaks()
{
	AllocationBook::mark_static_allocations_done();
}

void mark_static_allocations_left_for_memory_leaks()
{
	AllocationBook::mark_static_allocations_left();
}

#endif

#ifndef AN_CHECK_MEMORY_LEAKS
#ifdef AN_MEMORY_STATS
#ifndef AN_RELEASE
void* __cdecl operator new(size_t _size)
{
	store_new_call();
	return allocate_memory(_size);
}

void* __cdecl operator new[](size_t _size)
{
	store_new_call();
	return allocate_memory(_size);
}

void operator delete(void* _pointer) AN_CLANG_NO_EXCEPT
{
	store_delete_call();
	free_memory(_pointer);
}

void operator delete[](void* _pointer) AN_CLANG_NO_EXCEPT
{
	store_delete_call();
	free_memory(_pointer);
}
#endif
#endif
#endif

void start_memory_detect_checkpoint()
{
#ifdef AN_INSPECT_MEMORY_LEAKS
	AllocationBook::start_checkpoint();
#endif
}

void end_memory_detect_checkpoint(bool _allThreads)
{
#ifdef AN_INSPECT_MEMORY_LEAKS
	AllocationBook::end_checkpoint(_allThreads);
#endif
}

Concurrency::SpinLock memoryAllocatedCheckedInSizeLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
size_t memoryAllocatedCount = 0;

size_t memoryAllocatedCheckedInSize = 0; // actually used by game

size_t get_memory_allocated_checked_in()
{
#ifdef AN_CONCURRENCY_STATS
	memoryAllocatedCheckedInSizeLock.do_not_report_stats();
#endif
	Concurrency::ScopedSpinLock lock(memoryAllocatedCheckedInSizeLock);
	size_t memoryAllocated = memoryAllocatedCheckedInSize;
	return memoryAllocated;
}

size_t get_memory_allocated_count()
{
#ifdef AN_CONCURRENCY_STATS
	memoryAllocatedCheckedInSizeLock.do_not_report_stats();
#endif
	Concurrency::ScopedSpinLock lock(memoryAllocatedCheckedInSizeLock);
	size_t mac = memoryAllocatedCount;
	return mac;
}

size_t get_memory_allocated_total()
{
#ifdef AN_CONCURRENCY_STATS
	memoryAllocatedCheckedInSizeLock.do_not_report_stats();
#endif
	Concurrency::ScopedSpinLock lock(memoryAllocatedCheckedInSizeLock);
	size_t memoryAllocated = memoryAllocatedCheckedInSize;
#ifdef AN_CHECK_MEMORY_LEAKS
	memoryAllocated += memoryAllocatedCount * sizeof(AllocationBook) + 2 * sizeof(int) * DADABABE_BUFFER_SIZE;
#endif
	return memoryAllocated;
}

void report_memory_allocated(size_t _size)
{
#ifdef AN_CONCURRENCY_STATS
	memoryAllocatedCheckedInSizeLock.do_not_report_stats();
#endif
	Concurrency::ScopedSpinLock lock(memoryAllocatedCheckedInSizeLock);
	++memoryAllocatedCount;
	memoryAllocatedCheckedInSize += _size;
}

void report_memory_freed(size_t _size)
{
#ifdef AN_CONCURRENCY_STATS
	memoryAllocatedCheckedInSizeLock.do_not_report_stats();
#endif
	Concurrency::ScopedSpinLock lock(memoryAllocatedCheckedInSizeLock);
	--memoryAllocatedCount;
	memoryAllocatedCheckedInSize -= _size;
}
