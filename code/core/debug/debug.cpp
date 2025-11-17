#include "debug.h"

#include "..\globalInclude.h"
#include "..\containers\array.h"
#include "..\concurrency\counter.h"
#include "..\concurrency\spinLock.h"
#include "..\concurrency\scopedSpinLock.h"
#include "..\concurrency\threadManager.h"
#include "..\concurrency\threadSystemUtils.h"

#include "..\system\video\video3d.h"
#include "..\system\video\videoGLExtensions.h"

#include <time.h>

#include "..\utils.h"

#include "..\io\dir.h"
#include "..\io\file.h"

#include "..\system\core.h"
#ifdef AN_ANDROID
#include "..\system\android\androidApp.h"
#endif

#ifdef AN_LINUX_OR_ANDROID
#include <unistd.h>
#endif

#ifdef AN_ANDROID
#include <android/log.h>
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
#define KEEP_ALL_LOGS
#endif

// force keep all logs for preview - makes life much easier
#ifndef KEEP_ALL_LOGS
#ifndef BUILD_PUBLIC_RELEASE
#define KEEP_ALL_LOGS
#endif
#endif

#ifdef AN_OUTPUT

	#ifndef AN_DEBUG_USE_FORMAT
	#ifdef AN_OUTPUT_TO_LOG
	#define AN_DEBUG_USE_FORMAT
	#endif
	#endif

	#ifndef AN_DEBUG_USE_FORMAT
	#ifdef AN_OUTPUT_TO_FILE
	#define AN_DEBUG_USE_FORMAT
	#endif
	#endif

	#ifndef AN_DEBUG_USE_FORMAT
	#ifdef AN_OUTPUT_TO_CONSOLE
	#ifndef AN_RELEASE
	#ifndef AN_PROFILER
	#define AN_DEBUG_USE_FORMAT
	#endif
	#endif
	#endif
	#endif

#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#define FILE_BUF_SIZE 512
#define BUF_SIZE 16384

void tchar_to_char(tchar const* buf, char* buf8)
{
	char* dst = buf8;
	tchar const* src = buf;
	while (*src != 0)
	{
		if (*src > 128)
		{
			*dst = '?';
		}
		else
		{
			*dst = (char)* src;
		}
		++dst;
		++src;
	}
	*dst = 0;
}

#ifdef AN_OUTPUT_TO_FILE
bool filePresent = false;
IO::File file;
bool newLineStarted = true;
tchar fileName[1024] = TXT("");
tchar copyFileName[1024] = TXT("");

tchar const * get_log_file_name()
{
	return fileName;
}

void push_logs_numbers(tchar const* _withoutExtension)
{
	String fileNameFrom;
	String fileNameTo;
	for (int idx = 9; idx > 0; --idx)
	{
		if (idx > 1)
		{
			fileNameFrom = String::printf(TXT("%S.%i.log"), _withoutExtension, idx - 1);
		}
		else
		{
			fileNameFrom = String::printf(TXT("%S.log"), _withoutExtension);
		}
		fileNameFrom = IO::get_user_game_data_directory() + fileNameFrom;
		if (IO::File::does_exist(fileNameFrom))
		{
			fileNameTo = String::printf(TXT("%S.%i.log"), _withoutExtension, idx);
			fileNameTo = IO::get_user_game_data_directory() + fileNameTo;
			IO::File::delete_file(fileNameTo);
			IO::File::rename_file(fileNameFrom, fileNameTo);
			IO::File::delete_file(fileNameFrom);
		}
	}
}

void make_sure_file_exists()
{
	if (!filePresent || !file.is_open())
	{
		// maybe we have already open once, reopen same file
		if (tstrlen(fileName) == 0)
		{
#ifdef KEEP_ALL_LOGS
			time_t currentTime = time(0);
			tm currentTimeInfo;
			tm* pcurrentTimeInfo = &currentTimeInfo;
#ifdef AN_CLANG
			pcurrentTimeInfo = localtime(&currentTime);
#else
			localtime_s(&currentTimeInfo, &currentTime);
#endif
#ifdef AN_CLANG
			tsprintf(fileName, 1023, TXT("_logs%Soutput %04i-%02i-%02i %02i-%02i-%02i.log"),
#else
			tsprintf(fileName, 1023, TXT("_logs%soutput %04i-%02i-%02i %02i-%02i-%02i.log"),
#endif
				IO::get_directory_separator(),
				pcurrentTimeInfo->tm_year + 1900, pcurrentTimeInfo->tm_mon + 1, pcurrentTimeInfo->tm_mday,
				pcurrentTimeInfo->tm_hour, pcurrentTimeInfo->tm_min, pcurrentTimeInfo->tm_sec);
#else
#ifdef AN_CLANG
			tsprintf(fileName, 1023, TXT("_logs%Soutput"),
#else
			tsprintf(fileName, 1023, TXT("_logs%soutput"),
#endif
				IO::get_directory_separator());
			push_logs_numbers(fileName);
#ifdef AN_CLANG
			tsprintf(fileName, 1023, TXT("_logs%Soutput.log"),
#else
			tsprintf(fileName, 1023, TXT("_logs%soutput.log"),
#endif
				IO::get_directory_separator());
#endif
			file.create(IO::get_user_game_data_directory() + String(fileName));
		}
		else
		{
			file.append_to(IO::get_user_game_data_directory() + String(fileName));
		}
		file.set_type(IO::InterfaceType::Text);
		if (file.is_open())
		{
			filePresent = true;
		}
	}
}

void close_file()
{
	if (filePresent)
	{
		file.close();
	}
	filePresent = false;
}

void disallow_output_internal();
void allow_output_internal();
bool is_output_allowed_internal();

byte copy_log_and_switch_buffer[FILE_BUF_SIZE];

String copy_log_internal(tchar const * _fileName, bool _addCurrentTime, bool _switch)
{
	String result;

	ScopedOutputLock outputLock;

	if (file.is_open())
	{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
		output(TXT("file is open"));
#endif

#ifndef KEEP_ALL_LOGS
		_addCurrentTime = false;
#endif
		String copyToFileName;
		if (_addCurrentTime)
		{
			time_t currentTime = time(0);
			tm currentTimeInfo;
			tm* pcurrentTimeInfo = &currentTimeInfo;
#ifdef AN_CLANG
			pcurrentTimeInfo = localtime(&currentTime);
#else
			localtime_s(&currentTimeInfo, &currentTime);
#endif
			copyToFileName = String::printf(TXT("_logs%S%S %04i-%02i-%02i %02i-%02i-%02i.log"),
				IO::get_directory_separator(),
				_fileName,
				pcurrentTimeInfo->tm_year + 1900, pcurrentTimeInfo->tm_mon + 1, pcurrentTimeInfo->tm_mday,
				pcurrentTimeInfo->tm_hour, pcurrentTimeInfo->tm_min, pcurrentTimeInfo->tm_sec);
		}
		else
		{
#ifdef AN_CLANG
			tsprintf(copyFileName, 1023, TXT("_logs%S%S"),
#else
			tsprintf(copyFileName, 1023, TXT("_logs%s%s"),
#endif
				IO::get_directory_separator(), _fileName);
			push_logs_numbers(copyFileName);
			copyToFileName = String::printf(TXT("_logs%S%S.log"),
				IO::get_directory_separator(), _fileName);
		}

#ifdef AN_ALLOW_EXTENSIVE_LOGS
		output(TXT("copy to a new file \"%S\""), copyToFileName.to_char());
#endif

		file.copy_to_file(IO::get_user_game_data_directory() + copyToFileName);

		if (_switch)
		{
#ifdef AN_CLANG
			tsprintf(fileName, 1023, TXT("%S"), copyFileName);
#else
			tsprintf(fileName, 1023, TXT("%s"), copyFileName);
#endif
			// continue writing in a new file
			memory_copy(fileName, copyToFileName.to_char(), sizeof(tchar) * (copyToFileName.get_length() + 1));
			file.append_to(IO::get_user_game_data_directory() + copyToFileName);
		}

		result = copyToFileName;
	}

	return result;
}

String copy_log(tchar const* _fileName, bool _addCurrentTime)
{
	return copy_log_internal(_fileName, _addCurrentTime, false);
}

String copy_log_and_switch(tchar const* _fileName, bool _addCurrentTime)
{
	return copy_log_internal(_fileName, _addCurrentTime, true);
}

#else

String copy_log(tchar const * _fileName, bool _addCurrentTime)
{
	return String::empty();
}

String copy_log_and_switch(tchar const * _fileName, bool _addCurrentTime)
{
	return String::empty();
}

#endif

#ifdef AN_LINUX
void DebugBreak()
{
	todo_important("implement_ for linux");
	#error implement for linux
}
#endif

#ifdef AN_OUTPUT_TO_FILE
void fill_time(tchar * buf, int bufSize)
{
	System::Core::fill_time_info(buf, bufSize);
}

tchar handle_new_line_buf[BUF_SIZE + 1];
void handle_new_line()
{
	if (newLineStarted)
	{
		tchar* buf = handle_new_line_buf;
		fill_time(buf, BUF_SIZE);
		file.write_text(buf);
		tsprintf(buf, BUF_SIZE, TXT(""));
		file.write_text(buf);
		newLineStarted = false;
	}
}

tchar handle_multiline_wholeBuf[BUF_SIZE + 1];
tchar handle_multiline_singleLine[BUF_SIZE + 1];
void handle_multiline(tchar const * const _buf)
{
	tchar* wholeBuf = handle_multiline_wholeBuf;
	tchar* singleLine = handle_multiline_singleLine;
	tchar const * ch = _buf;
	tchar const * s = ch;
	int length = min<int>(BUF_SIZE, (int)tstrlen(_buf));
	memory_copy(wholeBuf, _buf, length * sizeof(tchar));
	wholeBuf[length] = 0;
	while (*ch != 0)
	{
		if (*ch == '\n')
		{
			memory_copy(singleLine, &wholeBuf[s - _buf], sizeof(tchar) * (ch - s + 1));
			singleLine[ch - s + 1] = 0;
			file.write_text(singleLine);
			if (newLineStarted)
			{
				// handle one we missed
				handle_new_line();
			}
			newLineStarted = true;
			if (*(ch + 1) != 0)
			{
				// leave handling new line if it was last one, as we will be writing time
				handle_new_line();
			}
			++ch;
			s = ch;
		}
		else
		{
			++ch;
		}
	}
	if (s != ch)
	{
		memory_copy(singleLine, &wholeBuf[s - _buf], sizeof(tchar) * (ch - s));
		singleLine[ch - s] = 0;
		file.write_text(singleLine);
	}
}
#endif

#ifdef AN_ANDROID
bool androidLineInitialised = false;
tchar androidLine[BUF_SIZE + 1];
int androidLineLength = 0;
Concurrency::SpinLock androidLineLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
Concurrency::SpinLock& android_line_lock()
{
#ifdef AN_CONCURRENCY_STATS
	androidLineLock.do_not_report_stats();
#endif
	return androidLineLock;
}

void make_sure_android_line_is_ok()
{
	if (!androidLineInitialised)
	{
		an_assert(android_line_lock().is_locked_on_this_thread());
		androidLineInitialised = true;
		androidLine[0] = 0;
		androidLineLength = 0;
	}
}

char output_android_line_buf8[BUF_SIZE + 1];
void output_android_line()
{
	an_assert(android_line_lock().is_locked_on_this_thread());
	char * buf8 = output_android_line_buf8;
	tchar_to_char(androidLine, buf8);
	char* lastAt = buf8;
	char* at = buf8;
	while (true)
	{
		if (*at == 0)
		{
			break;
		}
		if (*at == '\n')
		{
			*at = 0;
			__android_log_print(ANDROID_LOG_INFO, AN_ANDROID_LOG_TAG, "%s", lastAt);
			lastAt = at + 1;
		}
		++ at;
	}
	if (lastAt != at)
	{
		__android_log_print(ANDROID_LOG_INFO, AN_ANDROID_LOG_TAG, "%s", lastAt);
	}
	androidLine[0] = 0;
	androidLineLength = 0;
}
#endif

tchar trace_func_buf[BUF_SIZE + 1];

void trace_func(tchar const * const _text, ...)
{
#ifdef AN_OUTPUT
	if (!_text)
	{
		return;
	}
	if (!is_output_allowed())
	{
		return;
	}
	if (!is_output_allowed_internal())
	{
		return;
	}
	disallow_output_internal();
	va_list list;
	va_start(list, _text);
#ifdef AN_DEBUG_USE_FORMAT
#ifndef AN_CLANG
	int textsize = (int)(tstrlen(_text) + 1);
	int memsize = textsize * sizeof(tchar);
	allocate_stack_var(tchar, format, textsize);
	memory_copy(format, _text, memsize);
	sanitise_string_format(format);
#else
	tchar const* format = _text;
#endif
#endif
#ifdef AN_OUTPUT_TO_LOG
#ifdef AN_WINDOWS
#ifndef AN_RELEASE
#ifndef AN_PROFILER
	if (IsDebuggerPresent())
	{
		tchar buf[BUF_SIZE + 1];
		tvsprintf(buf, BUF_SIZE, format, list);
		OutputDebugString(buf);
	}
#endif
#endif
#endif
#ifdef AN_ANDROID
#ifdef AN_OUTPUT_TO_LOG
	{
		Concurrency::ScopedSpinLock lock(android_line_lock());
		make_sure_android_line_is_ok();
		tvsprintf(&androidLine[androidLineLength], BUF_SIZE - androidLineLength, format, list);
		androidLineLength = tstrlen(androidLine);
	}
#endif
#endif
#endif
#ifdef AN_OUTPUT_TO_FILE
	make_sure_file_exists();
	tchar* buf = trace_func_buf;
	handle_new_line();
	tvsprintf(buf, BUF_SIZE, format, list);
	handle_multiline(buf);
	file.flush();
#endif
#ifdef AN_OUTPUT_TO_CONSOLE
	tvprintf(format, list);
#endif
	allow_output_internal();
#endif
}

void report_break_into_lines(String const& _text)
{
	lock_output();
	List<String> lines;
	_text.split(String::new_line(), lines);
	for_every(line, lines)
	{
		output(line->to_char());
	}
	unlock_output();
}

tchar report_func_buf[BUF_SIZE + 1];
void report_func(tchar const * const _text, ...)
{
#ifdef AN_OUTPUT
	if (!_text)
	{
		return;
	}
	if (!is_output_allowed())
	{
		return;
	}
	if (!is_output_allowed_internal())
	{
		return;
	}
	disallow_output_internal();
	va_list list;
	va_start(list, _text);
#ifdef AN_DEBUG_USE_FORMAT
#ifndef AN_CLANG
	int textsize = (int)(tstrlen(_text) + 1);
	int memsize = textsize * sizeof(tchar);
	allocate_stack_var(tchar, format, textsize);
	memory_copy(format, _text, memsize);
	sanitise_string_format(format);
#else
	tchar const* format = _text;
#endif
#endif
#ifdef AN_OUTPUT_TO_LOG
#ifdef AN_WINDOWS
#ifndef AN_RELEASE
#ifndef AN_PROFILER
	if (IsDebuggerPresent())
	{
		tchar buf[BUF_SIZE + 1];
		tvsprintf(buf, BUF_SIZE, format, list);
		OutputDebugString(buf);
		OutputDebugString(TXT("\n"));
	}
#endif
#endif
#endif
#ifdef AN_ANDROID
#ifdef AN_OUTPUT_TO_LOG
	{
		Concurrency::ScopedSpinLock lock(android_line_lock());
		make_sure_android_line_is_ok();
		tvsprintf(&androidLine[androidLineLength], BUF_SIZE - androidLineLength, format, list);
		androidLineLength = tstrlen(androidLine);
		output_android_line();
	}
#endif
#endif
#endif
#ifdef AN_OUTPUT_TO_FILE
	make_sure_file_exists();
	tchar* buf = report_func_buf;
	handle_new_line();
	tvsprintf(buf, BUF_SIZE, format, list);
	/*
	char extbuf[BUF_SIZE + 1];
	WideCharToMultiByte(CP_ACP, 0, buf, -1, extbuf, BUF_SIZE, 0, 0);
	*/
	handle_multiline(buf);
	tsprintf(buf, BUF_SIZE, TXT("\n"));
	handle_multiline(buf);
	file.flush();
#endif
#ifdef AN_OUTPUT_TO_CONSOLE
	tvprintf(format, list);
	tprintf(TXT("\n"));
#endif
	allow_output_internal();
#endif
}

void report_gl_func(tchar const* const _text, ...)
{
#ifdef AN_OUTPUT
	if (!_text)
	{
		return;
	}
	va_list list;
	va_start(list, _text);
#ifdef AN_DEBUG_USE_FORMAT
#ifndef AN_CLANG
	int textsize = (int)(tstrlen(_text) + 1);
	int memsize = textsize * sizeof(tchar);
	allocate_stack_var(tchar, format, textsize);
	memory_copy(format, _text, memsize);
	sanitise_string_format(format);
#else
	tchar const* format = _text;
#endif
#endif
	// report to gl
	{
		tchar buf[256 + 1];
		tvsprintf(buf, 256, format, list);
		String formatStr(buf);
		Array<char8> asChar8;
		formatStr.to_char8_array(asChar8);
		static int id = 0;
		auto& glExtensions = ::System::VideoGLExtensions::get();
		if (glExtensions.glDebugMessageInsert)
		{
			DIRECT_GL_CALL_ glExtensions.glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, id, GL_DEBUG_SEVERITY_NOTIFICATION, -1, asChar8.get_data()); AN_GL_CHECK_FOR_ERROR
		}
		else
		{
			output(TXT("can't use glDebugMessageInsert"));
		}
		++id;
	}
#endif
}

void report_waiting_func(bool _endWaiting)
{
	if (!is_output_allowed())
	{
		return;
	}
	if (!is_output_allowed_internal())
	{
		return;
	}
	disallow_output_internal();
#ifdef AN_OUTPUT_TO_CONSOLE
	if (_endWaiting)
	{
		tprintf(TXT(" \b"));
	}
	else
	{
		static tchar const * waitingChars = TXT("|/-\\");
		static int waitingCharIdx = 0;
		waitingCharIdx = (waitingCharIdx + 1) % 4;
		tprintf(TXT("%c\b"), waitingChars[waitingCharIdx]);
	}
#endif
	allow_output_internal();
}

tchar detected_memory_leaks_report_func_buf[BUF_SIZE + 1];
void detected_memory_leaks_report_func(tchar const * const _text, ...)
{
#ifdef AN_OUTPUT
	if (!_text)
	{
		return;
	}
	if (!is_output_allowed())
	{
		return;
	}
	if (!is_output_allowed_internal())
	{
		return;
	}
	disallow_output_internal();
	va_list list;
	va_start(list, _text);
#ifdef AN_DEBUG_USE_FORMAT
#ifndef AN_CLANG
	int textsize = (int)(tstrlen(_text) + 1);
	int memsize = textsize * sizeof(tchar);
	allocate_stack_var(tchar, format, textsize);
	memory_copy(format, _text, memsize);
	sanitise_string_format(format);
#else
	tchar const* format = _text;
#endif
#endif
#ifdef AN_OUTPUT_TO_LOG
#ifdef AN_WINDOWS
#ifndef AN_RELEASE
#ifndef AN_PROFILER
	if (IsDebuggerPresent())
	{
		tchar buf[BUF_SIZE + 1];
		tvsprintf(buf, BUF_SIZE, format, list);
		OutputDebugString(buf);
		OutputDebugString(TXT("\n"));
	}
#endif
#endif
#endif
#ifdef AN_ANDROID
#ifdef AN_OUTPUT_TO_LOG
	{
		Concurrency::ScopedSpinLock lock(android_line_lock());
		make_sure_android_line_is_ok();
		tvsprintf(&androidLine[androidLineLength], BUF_SIZE - androidLineLength, format, list);
		androidLineLength = tstrlen(androidLine);
		output_android_line();
	}
#endif
#endif
#endif
#ifdef AN_OUTPUT_TO_FILE
	make_sure_file_exists();
	tchar * buf = detected_memory_leaks_report_func_buf;
	handle_new_line();
	tvsprintf(buf, BUF_SIZE, format, list);
	handle_multiline(buf);
	tsprintf(buf, BUF_SIZE, TXT("\n"));
	handle_multiline(buf);
	file.flush();
#endif
#ifdef AN_OUTPUT_TO_CONSOLE
	tvprintf(format, list);
	tprintf(TXT("\n"));
#endif
	allow_output_internal();
#endif
}

void close_output()
{
#ifdef AN_OUTPUT_TO_FILE
	close_file();
#endif
}

void close_file_after_detected_memory_leaks()
{
#ifdef AN_OUTPUT_TO_FILE
	close_file();
#endif
}

tchar report_to_file_func_buf[BUF_SIZE + 1];
void report_to_file_func(tchar const * const _text, ...)
{
#ifdef AN_OUTPUT_TO_FILE
	if (!_text)
	{
		return;
	}
	if (!is_output_allowed())
	{
		return;
	}
	if (!is_output_allowed_internal())
	{
		return;
	}
	disallow_output_internal();
	va_list list;
	va_start(list, _text);
#ifdef AN_DEBUG_USE_FORMAT
#ifndef AN_CLANG
	int textsize = (int)(tstrlen(_text) + 1);
	int memsize = textsize * sizeof(tchar);
	allocate_stack_var(tchar, format, textsize);
	memory_copy(format, _text, memsize);
	sanitise_string_format(format);
#else
	tchar const* format = _text;
#endif
#endif
#ifdef AN_OUTPUT_TO_FILE
	make_sure_file_exists();
	tchar * buf = report_to_file_func_buf;
	handle_new_line();
	tvsprintf(buf, BUF_SIZE, format, list);
	handle_multiline(buf);
	tsprintf(buf, BUF_SIZE, TXT("\n"));
	handle_multiline(buf);
	file.flush();
#endif
	allow_output_internal();
#endif
}

int get_current_thread_system_id()
{
#ifdef AN_WINDOWS
	return GetCurrentThreadId();
#else
#ifdef AN_LINUX_OR_ANDROID
	return gettid();
#else
#error TODO implement get current thread id
#endif
#endif
	return 0;
}

struct OutputLock
{
	struct CounterWithId
	{
		Concurrency::Counter counter;
		int threadSystemId;

		CounterWithId() {}
		CounterWithId(int _threadSystemId) : threadSystemId(_threadSystemId) {}
	};
	Array<CounterWithId> lockCountersPerThread;
	Concurrency::SpinLock accessLockCounters = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
	Concurrency::SpinLock mainLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);

	OutputLock()
	{
#ifdef AN_CONCURRENCY_STATS
		accessLockCounters.do_not_report_stats();
		mainLock.do_not_report_stats();
#endif
		lockCountersPerThread.make_space_for(THREAD_LIMIT * 2);
	}

	Concurrency::Counter& access_lock_counter(int _threadSystemId)
	{
		int threadSystemId = _threadSystemId;
		for_every(counter, lockCountersPerThread)
		{
			if (counter->threadSystemId == threadSystemId)
			{
				return counter->counter;
			}
		}
		lockCountersPerThread.push_back(CounterWithId(_threadSystemId));
		return lockCountersPerThread.get_last().counter;
	}

	void increase_lock_counter_for_thread(int _threadSystemId)
	{
		Concurrency::ScopedSpinLock tempAccess(accessLockCounters);
		access_lock_counter(_threadSystemId).increase();
	}

	void decrease_lock_counter_for_thread(int _threadSystemId)
	{
		Concurrency::ScopedSpinLock tempAccess(accessLockCounters);
		access_lock_counter(_threadSystemId).decrease();
	}

	bool is_lock_counter_locked_for_thread(int _threadSystemId)
	{
		Concurrency::ScopedSpinLock tempAccess(accessLockCounters);
		bool isLocked = access_lock_counter(_threadSystemId).get() != 0;
		return isLocked;
	}

	void acquire_lock()
	{
		int threadSystemId = get_current_thread_system_id();
		if (!is_lock_counter_locked_for_thread(threadSystemId))
		{
			mainLock.acquire();
		}
		increase_lock_counter_for_thread(threadSystemId);
	}

	void release_lock()
	{
		int threadSystemId = get_current_thread_system_id();
		decrease_lock_counter_for_thread(threadSystemId);
		if (!is_lock_counter_locked_for_thread(threadSystemId))
		{
			mainLock.release();
		}
	}
};

void output_locking(bool _lock)
{
	static OutputLock outputLock;
	if (_lock)
	{
		outputLock.acquire_lock();
	}
	else
	{
		outputLock.release_lock();
	}
}

void lock_output()
{
	output_locking(true);
}

void unlock_output()
{
	output_locking(false);
}

// allowance is to make sure that we're not trying to output on the same thread while we're outputing something
// it is not an output lock!
struct OutputAllowance
{
	OutputAllowance()
	{
#ifdef AN_CONCURRENCY_STATS
		accessLockCounters.do_not_report_stats();
#endif
		lockCountersPerThread.make_space_for(THREAD_LIMIT * 2);// Concurrency::ThreadSystemUtils::get_number_of_cores());
	}

	struct CounterWithId
	{
		Concurrency::Counter counter;
		int threadSystemId;

		CounterWithId() {}
		CounterWithId(int _threadSystemId) : threadSystemId(_threadSystemId) {}
	};
	Array<CounterWithId> lockCountersPerThread;
	Concurrency::SpinLock accessLockCounters = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);

	Concurrency::Counter& access_lock_counter(int _id)
	{
		int id = _id;
		for_every(counter, lockCountersPerThread)
		{
			if (counter->threadSystemId == id)
			{
				return counter->counter;
			}
		}
		lockCountersPerThread.push_back(CounterWithId(_id));
		return lockCountersPerThread.get_last().counter;
	}

	void increase_lock_counter_for_thread(int _threadSystemId)
	{
		Concurrency::ScopedSpinLock tempAccess(accessLockCounters);
		access_lock_counter(_threadSystemId).increase();
	}

	void decrease_lock_counter_for_thread(int _threadSystemId)
	{
		Concurrency::ScopedSpinLock tempAccess(accessLockCounters);
		access_lock_counter(_threadSystemId).decrease();
	}

	bool is_lock_counter_locked_for_thread(int _threadSystemId)
	{
		Concurrency::ScopedSpinLock tempAccess(accessLockCounters);
		bool isLocked = access_lock_counter(_threadSystemId).get() != 0;
		return isLocked;
	}

	void acquire_lock()
	{
		int id = get_current_thread_system_id();
		increase_lock_counter_for_thread(id);
	}

	void release_lock()
	{
		int id = get_current_thread_system_id();
		decrease_lock_counter_for_thread(id);
	}

	bool is_locked()
	{
		int id = get_current_thread_system_id();
		return is_lock_counter_locked_for_thread(id);
	}
};

//

// this is per thread

bool output_allowance_internal(bool * _allow)
{
	static OutputAllowance outputAllowance;
	if (_allow)
	{
		if (*_allow)
		{
			outputAllowance.release_lock();
		}
		else
		{
			outputAllowance.acquire_lock();
		}
	}
	return ! outputAllowance.is_locked();
}

void disallow_output_internal()
{
	bool allowance = false;
	output_allowance_internal(&allowance);
}

void allow_output_internal()
{
	bool allowance = true;
	output_allowance_internal(&allowance);
}

bool is_output_allowed_internal()
{
	return output_allowance_internal(nullptr);
}

//

// this is global

bool output_allowance(bool* _allow)
{
	static bool outputAllowed = true;
	
	if (_allow)
	{
		outputAllowed = *_allow;
	}
	return outputAllowed;
}

void disallow_output()
{
	bool allowance = false;
	output_allowance(&allowance);
}

void allow_output()
{
	bool allowance = true;
	output_allowance(&allowance);
}

bool is_output_allowed()
{
	return output_allowance(nullptr);
}

//

const int32 popUpInfoSize = BUF_SIZE;
tchar popUpInfoFormated[popUpInfoSize + 1];
tchar popUpInfoFormatedAll[popUpInfoSize + 1];

void debug_internal_pop_up_format(tchar const * const _format, ...)
{
	if (!_format)
	{
		tsprintf(popUpInfoFormated, BUF_SIZE, TXT(""));
	}
	else
	{
		va_list list;
		va_start(list, _format);
#ifndef AN_CLANG
		int textsize = (int)(tstrlen(_format) + 1);
		int memsize = textsize * sizeof(tchar);
		allocate_stack_var(tchar, format, textsize);
		memory_copy(format, _format, memsize);
		sanitise_string_format(format);
#else
		tchar const* format = _format;
#endif
		tvsprintf(popUpInfoFormated, BUF_SIZE, format, list);
	}
}

int debug_internal_pop_up_message_box(bool _stopIsObligatory, tchar const * const _condition)
{
	static bool ignorePopUps = false;

	if (ignorePopUps && !_stopIsObligatory)
	{
		return false;
	}

	int retVal = false;
#ifdef AN_WINDOWS
	tsprintf(popUpInfoFormatedAll, BUF_SIZE, TXT("%s\n\ncondition:\n%s\n\nDo you want to break?\n(cancel to ignore further messages)"), popUpInfoFormated, _condition);

	// NOTE this won't stop other threads, they'll keep running!
	int result = MessageBox(nullptr, popUpInfoFormatedAll, TXT("Hi there!"), MB_ICONEXCLAMATION | (_stopIsObligatory? MB_OK : MB_YESNOCANCEL));
	retVal = result == IDYES;
	if (result == IDCANCEL)
	{
		ignorePopUps = true;
	}
#else
#ifdef AN_ANDROID
	{
		tsprintf(popUpInfoFormatedAll, BUF_SIZE, TXT("%S\n\ncondition:\n%S"), popUpInfoFormated, _condition);
		char info8[BUF_SIZE + 1];
		tchar_to_char(popUpInfoFormatedAll, info8);
		__android_log_print(ANDROID_LOG_ERROR, AN_ANDROID_LOG_TAG, "Hi there! %s", info8);
		if (_stopIsObligatory)
		{
			retVal = true; // always stop
			std::quick_exit(0);
		}
	}
#else
#ifdef AN_ANDROID
	{
		char cond8[BUF_SIZE + 1];
		tchar_to_char(_condition, cond8);
		char info8[BUF_SIZE + 1];
		tchar_to_char(popUpInfoFormated, info8);

		__android_log_print(ANDROID_LOG_ERROR, AN_ANDROID_LOG_TAG, "cond: %s info: %s", cond8, info8);
	}
	retVal = true; // always stop
#else
#ifdef AN_LINUX_OR_ANDROID
	retVal = true; // always stop
#else
	#error TODO implement
#endif
#endif
#endif
#endif

	return retVal;
}

void show_critical_message(bool _stopIsObligatory, tchar const* _message)
{
	String message;
#ifndef AN_WINDOWS
	_stopIsObligatory = true;
#endif
	if (_stopIsObligatory)
	{
		message = String::printf(TXT("%S\n\nThe game will now stop."), _message);
	}
	else
	{
		message = String::printf(TXT("%S\n\nWould you like to continue?"), _message);
	}
	output(TXT("Critical! %S"), message.to_char());
	bool stopNow = _stopIsObligatory;
#ifdef AN_WINDOWS
	// NOTE this won't stop other threads, they'll keep running!
	int result = MessageBox(nullptr, message.to_char(), TXT("Critical"), MB_ICONEXCLAMATION | (_stopIsObligatory ? MB_OK : MB_YESNOCANCEL));
	if (result == IDYES)
	{
		stopNow = true;
	}
#else
#ifdef AN_ANDROID
	{
		char info8[BUF_SIZE + 1];
		tchar_to_char(message.to_char(), info8);
		__android_log_print(ANDROID_LOG_ERROR, AN_ANDROID_LOG_TAG, "Critical! %s", info8);
		if (_stopIsObligatory)
		{
			//?
		}
	}
#else
#ifdef AN_ANDROID
	{
		char info8[BUF_SIZE + 1];
		tchar_to_char(message.to_char(), info8);

		__android_log_print(ANDROID_LOG_ERROR, AN_ANDROID_LOG_TAG, "Critical! %s", info8);
	}
#else
#ifdef AN_LINUX_OR_ANDROID
	stopNow = true; // always stop
#else
#error TODO implement
#endif
#endif
#endif
#endif
	if (stopNow)
	{
		std::quick_exit(0);
	}
}

#ifdef AN_OUTPUT
void output_colour(int _r, int _g, int _b, int _i)
{
#ifdef AN_WINDOWS
	int c = _b + _g * 2 + _r * 4 + _i * 8;

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, c);
#else
#ifdef AN_LINUX_OR_ANDROID
	todo_note(TXT("add colours to output to console"));
#else
#error TODO implement
#endif
#endif
}

void output_colour_system()
{
	output_colour(0, 1, 0, 0);
}

void output_colour_nav()
{
	output_colour(0, 1, 0, 0);
}
#endif

// call stack info

struct CallStackInfoImpl
{
	struct SingleInfo
	{
		static const int maxlength = 256;
		tchar info[maxlength + 1];
		void const* ptr;

		void store(void const* _ptr, tchar const* _info)
		{
			if (_info)
			{
				int len = min<int>(maxlength, (int)tstrlen(_info));
				info[len] = 0;
				memory_copy(info, _info, sizeof(tchar) * len);
			}
			else
			{
				info[0] = 0;
			}
			ptr = _ptr;
		}
	};
	struct ThreadInfo
	{
		static const int maxInfos = 256;

		int threadSystemId;
		ArrayStatic<SingleInfo, maxInfos> infos;
		int overLimit = 0;
		ThreadInfo(int _threadSystemId = NONE) : threadSystemId(_threadSystemId) { SET_EXTRA_DEBUG_INFO(infos, TXT("CallStackInfoImpl.ThreadInfo.infos")); }
	};
	ArrayStatic<ThreadInfo, MAX_THREAD_COUNT> threads;
	bool locked = false;

	CallStackInfoImpl()
	{
		SET_EXTRA_DEBUG_INFO(threads, TXT("CallStackInfoImpl.threads"));
	}

	static CallStackInfoImpl& access()
	{
		static CallStackInfoImpl impl;
		return impl;
	}

	static Concurrency::SpinLock& access_threads_lock()
	{
		static Concurrency::SpinLock threadsLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
#ifdef AN_CONCURRENCY_STATS
		threadsLock.do_not_report_stats();
#endif
		return threadsLock;
	}

	ThreadInfo& access_thread()
	{
		int threadSystemId = get_current_thread_system_id();
		for_every(thread, threads)
		{
			if (thread->threadSystemId == threadSystemId)
			{
				return *thread;
			}
		}

		Concurrency::ScopedSpinLock lock(access_threads_lock(), true);
		threads.push_back(ThreadInfo(threadSystemId));
		return threads.get_last();
	}

	void lock()
	{
		access_threads_lock().acquire();
		locked = true;
	}

	void push(void const * _ptr, tchar const* _info)
	{
		if (locked) while (locked) System::Core::sleep_u(1);
		ThreadInfo& ti = access_thread();
		if (ti.infos.get_space_left() > 0)
		{
			ti.infos.grow_size(1);
			ti.infos.get_last().store(_ptr, _info);
		}
		else
		{
			++ti.overLimit;
		}
	}

	void pop()
	{
		if (locked) while (locked) System::Core::sleep_u(1);
		ThreadInfo& ti = access_thread();
		if (ti.overLimit)
		{
			--ti.overLimit;
		}
		else
		{
			ti.infos.set_size(max(0, ti.infos.get_size() - 1));
		}
	}

	String get_info(ArrayStatic<int, CallStackInfo::maxThreads> const* _threads)
	{
		// this should be called when we're already stopped
		int threadSystemId = get_current_thread_system_id();
		String info(TXT("call stack info\n"));
		Concurrency::ScopedSpinLock lock(access_threads_lock(), true);
		for_every(thread, threads)
		{
			bool isCurrentThread = false;
			if (_threads)
			{
				for_every(t, *_threads)
				{
					isCurrentThread |= (thread->threadSystemId == *t);
				}
			}
			else
			{
				isCurrentThread |= thread->threadSystemId == threadSystemId;
			}
			info += String::printf(TXT("%S %02i (%07X) : "), isCurrentThread ? TXT(">>") : TXT("  "), for_everys_index(thread), thread->threadSystemId);
			for_every(i, thread->infos)
			{
				if (for_everys_index(i) > 0)
				{
					//			  |>> 01 (0123456) : |
					info += TXT("\n                  ");
				}
				if (i->ptr)
				{
					info += String::printf(TXT("[%p]"), i->ptr);
				}
				info += i->info;
			}
			if (thread->overLimit > 0)
			{
				//			  |>> 01 (0123456) : |
				info += TXT("\n                ");
				info += String::printf(TXT("+ %i more"), thread->overLimit);
			}
			info += TXT("\n");
		}
		return info;
	}
	tchar const* get_currently_at_info(ArrayStatic<int, CallStackInfo::maxThreads> const* _threads)
	{
		// this should be called when we're already stopped
		int threadSystemId = get_current_thread_system_id();
		Concurrency::ScopedSpinLock lock(access_threads_lock(), true);
		for_every(thread, threads)
		{
			bool isCurrentThread = false;
			if (_threads)
			{
				for_every(t, *_threads)
				{
					isCurrentThread |= (thread->threadSystemId == *t);
				}
			}
			else
			{
				isCurrentThread |= thread->threadSystemId == threadSystemId;
			}
			if (isCurrentThread && ! thread->infos.is_empty())
			{
				return thread->infos.get_last().info;
			}
		}
		return TXT("");
	}
};

void CallStackInfo::lock()
{
	CallStackInfoImpl::access().lock();
}

void CallStackInfo::push(void const * _ptr, tchar const* _info)
{
	CallStackInfoImpl::access().push(_ptr, _info);
}

void CallStackInfo::pop()
{
	CallStackInfoImpl::access().pop();
}

String CallStackInfo::get_info(ArrayStatic<int, CallStackInfo::maxThreads> const* _threads)
{
	return CallStackInfoImpl::access().get_info(_threads);
}

tchar const* CallStackInfo::get_currently_at_info(ArrayStatic<int, CallStackInfo::maxThreads> const* _threads)
{
	return CallStackInfoImpl::access().get_currently_at_info(_threads);
}

#ifdef AN_DEVELOPMENT
bool ignoreDebugOnErrorWarn = false;

void ignore_debug_on_error_warn()
{
	ignoreDebugOnErrorWarn = true;
}

void debug_on_error_impl()
{
	if (ignoreDebugOnErrorWarn) return;

	AN_BREAK;
}

void debug_on_warn_impl()
{
	if (ignoreDebugOnErrorWarn) return;

	AN_BREAK;
}
#endif

//

Concurrency::Counter failedAssertCount;
Concurrency::SpinLock failedAssertLock;
String failedAssertInfo;

int failed_assert_count()
{
	return failedAssertCount.get();
}

void mark_failed_assert(tchar const* _info)
{
	failedAssertCount.increase();

#ifndef AN_DEVELOPMENT
	Concurrency::ScopedSpinLock lock(failedAssertLock);

	failedAssertInfo += '\n';
	failedAssertInfo += _info;
	failedAssertInfo += '\n';

	String csi = get_call_stack_info();
	failedAssertInfo += csi;
	failedAssertInfo += '\n';

	output(TXT("failed assert call stack info:\n%S"), csi.to_char());
#endif
#ifdef AN_ASSERT
	// handled in an_assert_* definitions
#else
#ifdef AN_WINDOWS
	AN_DEVELOPMENT_BREAK
#endif
#endif
}

String get_failed_assert_info()
{
	String result;
	{
		Concurrency::ScopedSpinLock lock(failedAssertLock);
		result = failedAssertInfo;
	}
	return result;
}

//

String get_general_debug_info()
{
	String result;
	int count = failed_assert_count();
	if (count > 0)
	{
		result += String::printf(TXT("failed asserts present\n"));
		result += String::printf(TXT("failed asserts count: %i\n"), count);
		result += String::printf(TXT("failed asserts info:\n"));
		result += get_failed_assert_info();
	}
	else
	{
		result += String::printf(TXT("no failed asserts\n"));
	}
	return result;
}

//

#ifdef DEBUG_GPU_MEMORY
Array<int> gpuMemoryAllocated;
size_t gpuMemoryAllocatedSum = 0;
size_t gpuMemoryAvailable = 0;

void gpu_memory_info_report()
{
#ifdef AN_GL
	auto& glExtensions = ::System::VideoGLExtensions::get();
	if (glExtensions.NVX_gpu_memory_info)
	{
#ifdef DEBUG_GPU_MEMORY_OUTPUT
		GLint dedm = 0;
		glGetIntegerv(GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &dedm); AN_GL_CHECK_FOR_ERROR
		GLint totalm = 0;
		glGetIntegerv(GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &totalm); AN_GL_CHECK_FOR_ERROR
#endif
		GLint curavm = 0;
		glGetIntegerv(GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &curavm); AN_GL_CHECK_FOR_ERROR
		gpuMemoryAvailable = curavm;
		gpuMemoryAvailable = gpuMemoryAvailable << 10; // was in kb
#ifdef DEBUG_GPU_MEMORY_OUTPUT
		output(TXT("[gpu-memory] dedicated %iMB, total %iMB, currently avaiable %iMB"), dedm >> 10, totalm >> 10, curavm >> 10);
#endif
	}
#endif
}

void gpu_memory_info_get_func(size_t& _refAllocated, size_t& _refAvailable)
{
	_refAllocated = gpuMemoryAllocatedSum;
	_refAvailable = gpuMemoryAvailable;
}

int gpu_memory_info_unified_id(int _type, int _id)
{
	return _id * GPU_MEMORY_INFO_TYPE__COUNT + _type;
}

void gpu_memory_info_allocated_func(int _type, int _id, int _amount)
{
	_id = gpu_memory_info_unified_id(_type, _id);
	gpuMemoryAllocated.make_space_for(_id + 1);
	while (gpuMemoryAllocated.get_size() <= _id)
	{
		gpuMemoryAllocated.push_back(0);
	}
	gpuMemoryAllocatedSum -= gpuMemoryAllocated[_id];
	gpuMemoryAllocated[_id] = _amount;
	gpuMemoryAllocatedSum += gpuMemoryAllocated[_id];
#ifdef DEBUG_GPU_MEMORY_OUTPUT
	output(TXT("[gpu-memory] allo + %i: %i -> %iMB"), _id, _amount, gpuMemoryAllocatedSum >> 20);
#endif
	gpu_memory_info_report(); // to update available memory
}

void gpu_memory_info_freed_func(int _type, int _id)
{
	_id = gpu_memory_info_unified_id(_type, _id);
	int freed = 0;
	if (gpuMemoryAllocated.is_index_valid(_id))
	{
		freed = gpuMemoryAllocated[_id];
		gpuMemoryAllocatedSum -= freed;
		gpuMemoryAllocated[_id] = 0;
	}
#ifdef DEBUG_GPU_MEMORY_OUTPUT
	output(TXT("[gpu-memory] free - %i: %i -> %iMB"), _id, freed, gpuMemoryAllocatedSum >> 20);
#endif
	gpu_memory_info_report(); // to update available memory
}
#endif
