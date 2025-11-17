#include "faultHandler.h"

#include "timeStamp.h"

#include "video\video3d.h"

#include "..\buildInformation.h"
#include "..\mainConfig.h"
#include "..\renderDoc.h"
#include "..\concurrency\scopedSpinLock.h"
#include "..\containers\arrayStack.h"
#include "..\globalSettings.h"
#include "..\io\file.h"
#include "..\serialisation\serialiser.h"
#include "..\types\string.h"
#include "..\vr\iVR.h"

#include "core.h"

#ifndef AN_CLANG
#pragma warning( disable : 4091 )
#endif

#ifdef AN_WINDOWS
#include <WinBase.h>
#include <DbgHelp.h>
#include <windows.h>
#include <TlHelp32.h>
#endif

#ifdef AN_LINUX_OR_ANDROID
#include <iostream>
#include <iomanip>
#include <unwind.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <signal.h>
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define INVESTIGATE_FAULT_HANDLER
#define TEST_FAULT_HANDLER

//

#define USE_STACK_FRAME

#ifdef TEST_FAULT_HANDLER
	#ifdef AN_DEVELOPMENT_OR_PROFILER
		#ifdef USE_RENDER_DOC
			#define DISABLE_FAULT_HANDLER_AT_ALL
		#endif
		#define FAULT_HANDLER_USE_PDB
		#define FAULT_HANDLER_DURING_DEV
	#endif
#else
	#ifdef AN_DEVELOPMENT_OR_PROFILER
		// we want to catch everything via debugger
		#define DISABLE_FAULT_HANDLER_AT_ALL
	#endif
#endif

//

/**
 *	General Info about FaultHandler
 *
 *	It gets the stack and some additional info (use scoped_call_stack_info - and with REMOVE_AS_SOON_AS_POSSIBLE_ for nasty bugs)
 *
 *	It stores the stack in a log file, fills a bug and reports it.
 *
 *	Windows:		You just have a report with list of files and line numbers (this is handled with CodeInfo lookup)
 *
 *					If there is no CodeInfo provided, you still get address within a module (or module + offset or just address)
 *					Use VisualAssist's Address Resolver with appropriate pdb file (tea_[buildNo].pdb)
 *
 *	Quest/Linux:	In the report there is just info about function but without details at which line did it occur
 *					But there is also address within a module (libteaForGod.so, archived as tea_[buildNo].so)
 *					Use addr2line (from aarch64-linux-android-4.9 toolchain)
 *					addr2line -C -f -i -e tea_[buildNo].so [hex addr]
 *						-f shows function names
 *						-C demangles those names
 *						-i unwinds inlines
 *						-e is to point at the file to load
 */
std::function<bool(String const& _summary, String const& _info, String const& _filename)> report_bug_log = nullptr;

struct FaultHandlerCustom
{
	String info;
};
FaultHandlerCustom* faultHandlerCustom = nullptr;

bool faultHandlerEnabled = false;

Concurrency::SpinLock faultHandlerSpinLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);

bool faultHandlerActivated = false;

bool faultHandlerIgnoreFaults = false;

void on_fault_handler()
{
	output(TXT("on_fault_handler"));
	faultHandlerSpinLock.acquire(); // we won't release it

	if (faultHandlerActivated) return;
	faultHandlerActivated = true;

	lock_output();
	lock_call_stack_info();
	if (auto* tm = Concurrency::ThreadManager::get())
	{
		output(TXT("suspend_kill_other_threads"));
		tm->suspend_kill_other_threads();
		output(TXT("suspend_kill_other_threads [done]"));
	}
	output(TXT("on_fault_handler [done]"));
}

String summary;
String bugInfo;

#ifdef AN_WINDOWS
Concurrency::SpinLock symLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
bool symInitialised = false;
bool symInitialisedSuccessful = false;
HANDLE symInitialisedProcess;

bool sym_initialise()
{
	an_assert(symLock.is_locked_on_this_thread()); // has to be locked!

	if (!symInitialised)
	{
		SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

		HANDLE process = GetCurrentProcess();
		symInitialised = true;
		symInitialisedProcess = process;
		symInitialisedSuccessful = false;

		if (!SymInitialize(process, NULL, TRUE))
		{
			//DWORD error = GetLastError();
			//HRESULT hResult = HRESULT_FROM_WIN32(error);
			//output(TXT("SymInitialize failed: %d"), error);
			//output(TXT("process: %d"), process);
			return false;
		}

		symInitialisedSuccessful = true;
	}

	return symInitialisedSuccessful;
}

bool sym_clean_up()
{
	an_assert(symLock.is_locked_on_this_thread()); // has to be locked!

	if (symInitialised)
	{
		SymCleanup(symInitialisedProcess);
	}

	symInitialised = false;
	symInitialisedSuccessful = false;

	return true;
}
#endif

#define VER_BASE 0
#define CURRENT_VERSION VER_BASE

struct CodeInfo
{
	static CodeInfo* one;
	static char mingle;

	static bool serialise_string_as_char(Serialiser & _serialiser, String & _string)
	{
		bool result = true;
		if (_serialiser.is_reading())
		{
			int length;
			result &= serialise_data(_serialiser, length);
			ARRAY_PREALLOC_SIZE(char, str, length + 1);
			str.set_size(length + 1);
			result &= serialise_plain_data(_serialiser, str.get_data(), length);
			// mingle common for all transforms
			char lMingle = mingle;
			for_every(ch, str)
			{
				*ch -= lMingle;
				lMingle = ((lMingle + 5) << 2) + lMingle + 1;
			}
			mingle = lMingle;
			str[length] = 0;
			_string = String::convert_from(str.get_data());
		}
		else
		{
			int length = _string.get_length();
			result &= serialise_data(_serialiser, length);
			ARRAY_PREALLOC_SIZE(char, str, length + 1);
			str.set_size(length + 1);
			_string.convert_to(str.get_data(), length + 1);
			// mingle common for all transforms
			char lMingle = mingle;
			for_every(ch, str)
			{
				*ch += lMingle;
				lMingle = ((lMingle + 5) << 2) + lMingle + 1;
			}
			mingle = lMingle;
			result &= serialise_plain_data(_serialiser, str.get_data(), length);
		}
		return result;
	}

	struct CodeLine
	{
		uint64 codeAddress;
		int lineNumber;

		bool serialise(Serialiser & _serialiser)
		{
			bool result = true;

			result &= serialise_data(_serialiser, codeAddress);
			result &= serialise_data(_serialiser, lineNumber);

			return result;
		}
	};

	struct CodeFunction
	{
		String functionName;
		Array<CodeLine> lines;

		bool serialise(Serialiser & _serialiser)
		{
			bool result = true;

			result &= serialise_string_as_char(_serialiser, functionName);
			result &= serialise_data(_serialiser, lines);

			return result;
		}

		void add(uint64 _codeAddress, int _lineNumber)
		{
			for_every(line, lines)
			{
				if (line->codeAddress == _codeAddress)
				{
					line->lineNumber = _lineNumber;
					return;
				}
			}
			CodeLine line;
			line.codeAddress = _codeAddress;
			line.lineNumber = _lineNumber;
			lines.push_back(line);
		}
	};

	struct FileSource
	{
		String fileName;
		Array<CodeFunction> functions;

		bool serialise(Serialiser & _serialiser)
		{
			bool result = true;

			result &= serialise_string_as_char(_serialiser, fileName);
			result &= serialise_data(_serialiser, functions);

			return result;
		}

		void add(uint64 _codeAddress, int _lineNumber, char const * _functionName)
		{
			for_every(function, functions)
			{
				if (function->functionName.is_same_as(_functionName))
				{
					function->add(_codeAddress, _lineNumber);
					return;
				}
			}
			CodeFunction function;
			function.functionName = String::convert_from(_functionName);
			function.add(_codeAddress, _lineNumber);
			functions.push_back(function);
		}
	};

	uint64 baseModuleAddress = 0;
	int version = 0;
	Array<FileSource> files;

	bool serialise(Serialiser & _serialiser)
	{
		bool result = true;

		version = CURRENT_VERSION;
		result &= serialise_data(_serialiser, version);

		mingle = 43; // init for serialisation
		result &= serialise_data(_serialiser, files);

		return result;
	}

	void add(String const & _fileName, uint64 _codeAddress, int _lineNumber, char const * _functionName)
	{
		if (!_functionName)
		{
			_functionName = "<unknown>";
		}
		String fileNameStartingWithCode = _fileName;
		{
			int codeAt = fileNameStartingWithCode.find_first_of(String(TXT("\\code\\")));
			if (codeAt == NONE)
			{
				codeAt = fileNameStartingWithCode.find_first_of(String(TXT("/code/")));
			}
			if (codeAt != NONE)
			{
				fileNameStartingWithCode = fileNameStartingWithCode.get_sub(codeAt);
			}
		}
		for_every(file, files)
		{
			if (file->fileName == fileNameStartingWithCode)
			{
				file->add(_codeAddress, _lineNumber, _functionName);
				return;
			}
		}
		files.set_size(files.get_size() + 1);
		files.get_last().fileName = fileNameStartingWithCode;
		files.get_last().add(_codeAddress, _lineNumber, _functionName);
		log_to_file(TXT(" + %S"), fileNameStartingWithCode.to_char());
		if ((files.get_size() % 10) == 1)
		{
			output_waiting();
		}
	}

	void clear()
	{
		files.clear();
	}

#ifdef AN_WINDOWS
	static BOOL CALLBACK enum_line_callback(PSRCCODEINFO LineInfo, PVOID UserContext)
	{
		DWORD64 dwDisplacement = 0;

#ifdef WIN64
		char buffer[sizeof(IMAGEHLP_SYMBOL64) + (MAX_SYM_NAME + 1) * sizeof(TCHAR)];
		PIMAGEHLP_SYMBOL64 pSymbol = (PIMAGEHLP_SYMBOL64)buffer;
		pSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
		pSymbol->MaxNameLength = MAX_SYM_NAME;
#else
		char buffer[sizeof(SYMBOL_INFO) + (MAX_SYM_NAME + 1) * sizeof(TCHAR)];
		PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
		pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen = MAX_SYM_NAME;
#endif

		HANDLE process = GetCurrentProcess();
#ifdef WIN64
		if (SymGetSymFromAddr64(process, LineInfo->Address, &dwDisplacement, pSymbol))
#else
		if (SymFromAddr(process, LineInfo->Address, &dwDisplacement, pSymbol))
#endif
		{
			one->add(String::convert_from(LineInfo->FileName), LineInfo->Address - LineInfo->ModBase, LineInfo->LineNumber, pSymbol->Name);
		}
		else
		{
			DWORD error = GetLastError();
			log_to_file(TXT(": Failed to resolve address 0x%X: %u"), LineInfo->Address, error);
			one->add(String::convert_from(LineInfo->FileName), LineInfo->Address - LineInfo->ModBase, LineInfo->LineNumber, nullptr);
		}
		return true;
	}
	
#ifdef WIN64
	static BOOL CALLBACK enum_module_callback(PCSTR ModuleName, DWORD64 BaseOfDll, PVOID UserContext)
#else
	static BOOL CALLBACK enum_module_callback(PCSTR ModuleName, ULONG BaseOfDll, PVOID UserContext)
#endif
	{
		// we assume one exec/dll
		if (one->baseModuleAddress == 0)
		{
			one->baseModuleAddress = BaseOfDll;

			HANDLE process = GetCurrentProcess();
			SymEnumLines(process, one->baseModuleAddress, nullptr, nullptr, enum_line_callback, nullptr);
		}
		return true;
	}

#ifdef WIN64
	static BOOL CALLBACK enum_module_get_base_address_only_callback(PCSTR ModuleName, DWORD64 BaseOfDll, PVOID UserContext)
#else
	static BOOL CALLBACK enum_module_get_base_address_only_callback(PCSTR ModuleName, ULONG BaseOfDll, PVOID UserContext)
#endif
	{
		// we assume one exec/dll
		if (one->baseModuleAddress == 0)
		{
			one->baseModuleAddress = BaseOfDll;
		}
		return true;
	}
	
#endif

	void build()
	{
		clear();

		log_to_file(TXT("build code info"));

#ifdef AN_WINDOWS
		symLock.acquire();
		if (sym_initialise())
		{
			HANDLE process = GetCurrentProcess();

			baseModuleAddress = 0;
			SymEnumerateModules(process, enum_module_callback, nullptr);
		}

		sym_clean_up();
		symLock.release();
#endif

		output_waiting_done();
	}

	String get_info(uint64 _codeAddress, Optional<bool> const& _withAddress = NP)
	{
#ifdef AN_WINDOWS
		Concurrency::ScopedSpinLock lock(symLock, true);

		if (sym_initialise())
		{
			HANDLE process = GetCurrentProcess();

			baseModuleAddress = 0;
			SymEnumerateModules(process, enum_module_get_base_address_only_callback, nullptr);
		}
#endif
		uint64 relAddress = _codeAddress - baseModuleAddress;

		return get_info_rel(relAddress, _withAddress);
	}

	String get_info_rel(uint64 _relCodeAddress, Optional<bool> const& _withAddress = NP)
	{
		String result;

		uint64 address = _relCodeAddress;

		uint64 bestDiff = 100000;
		FileSource const * bestFile = nullptr;
		CodeFunction const * bestFunc = nullptr;
		CodeLine const * bestLine = nullptr;
		for_every(file, files)
		{
			for_every(func, file->functions)
			{
				for_every(line, func->lines)
				{
					// check only if later, this should catch all cases of multiple instructions per code line
					uint64 diff = (uint64)((int64)address - (int64)line->codeAddress);
					if (diff < bestDiff)
					{
						bestDiff = diff;
						bestFile = file;
						bestFunc = func;
						bestLine = line;
					}
				}
			}
		}

		if (_withAddress.get(true))
		{
			result += String::printf(TXT("[b:%p+"), baseModuleAddress);
			result += String::printf(TXT("%p] "), address);
		}
		if (bestFile && bestFunc && bestLine)
		{
			result += bestFile->fileName;
			result += String::printf(TXT(" (%i)"), bestLine->lineNumber);
			result += String::printf(TXT(" : %S"), bestFunc->functionName.to_char());
			if (bestDiff != 0)
			{
				result += String::printf(TXT(" (~%i)"), bestDiff);
			}
		}
		else
		{
			result += String(TXT("??"));
		}
		return result;
	}
};

CodeInfo* CodeInfo::one = nullptr;
char CodeInfo::mingle;

void System::FaultHandler::create_code_info(IO::File& _saveTo)
{
	output_colour(0, 1, 1, 1);
	output(TXT("creating code info"));
	output_colour();
	System::TimeStamp ts;

	an_assert(CodeInfo::one);
	CodeInfo::one->build();
	CodeInfo::one->serialise(Serialiser(&_saveTo, SerialiserMode::Writing).temp_ref());
	output(TXT("done in %.2fs"), ts.get_time_since());
	output_colour();
}

void System::FaultHandler::load_code_info(IO::File& _loadFrom)
{
	an_assert(CodeInfo::one);
	CodeInfo::one->clear();
	if (_loadFrom.is_open())
	{
		output_colour(0, 1, 1, 0);
		output(TXT("load code info"));
		output_colour();
		CodeInfo::one->serialise(Serialiser(&_loadFrom, SerialiserMode::Reading).temp_ref());
	}
}

#ifdef AN_WINDOWS
String System::FaultHandler::get_info(uint64 _relCodeAddress, Optional<bool> const& _withAddress)
{
	an_assert(CodeInfo::one);
	return CodeInfo::one->get_info_rel(_relCodeAddress, _withAddress);
}

#define output_now(...) \
	if (!_firstBaseAddr) \
	{ \
		if (_outputTo) \
		{ \
			*_outputTo += String::printf(__VA_ARGS__); \
		} \
		else \
		{ \
			output(__VA_ARGS__); \
		} \
	}

#ifdef WIN64
#define ADDR_DWORD DWORD64
#else
#define ADDR_DWORD DWORD
#endif

void System::FaultHandler::output_modules_base_address(bool _justFirstOne, String * _outputTo, void ** _firstBaseAddr)
{
	DWORD procId = GetCurrentProcessId();
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);

	if (hSnapShot != nullptr)
	{
		if (!_justFirstOne)
		{
			output_now(TXT("modules"));
		}
		MODULEENTRY32 me32;
		memory_zero(&me32, sizeof(me32));
		me32.dwSize = sizeof(MODULEENTRY32);

		if (Module32First(hSnapShot, &me32))
		{
			if (_firstBaseAddr)
			{
				*_firstBaseAddr = me32.modBaseAddr;
			}
			if (_justFirstOne)
			{
				output_now(TXT("%S at %p"), me32.szModule, (ADDR_DWORD)me32.modBaseAddr);
			}
			else
			{
				do
				{
					output_now(TXT("    %p : %S"), (ADDR_DWORD)me32.modBaseAddr, me32.szModule);
				} while (Module32Next(hSnapShot, &me32));
			}
		}

		CloseHandle(hSnapShot);
	}
	else
	{
		output_now(TXT("no modules info"));
	}
}
#undef output_now

#define trace_bug_info(...) \
	bugInfo += String::printf(__VA_ARGS__); \
	trace(__VA_ARGS__);

String get_info(void* codeAddress, Optional<bool> const & _forcePDB = NP, Optional<bool> const& _withAddress = NP)
{
	if (!sym_initialise())
	{
		DWORD error = GetLastError();
		HRESULT hResult = HRESULT_FROM_WIN32(error);
		return String::printf(TXT("SymInitialize failed: %d"), error);
	}

	if (CodeInfo::one && !_forcePDB.get(false))
	{
		return CodeInfo::one->get_info((uint64)codeAddress, _withAddress.get(true));
	}

	HANDLE process = symInitialisedProcess;

	DWORD64 dwDisplacement = 0;
	DWORD64 dwAddress = (DWORD64)codeAddress;

	char buffer[sizeof(SYMBOL_INFO) + (MAX_SYM_NAME + 1)* sizeof(TCHAR)];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME;

	String result;

	if (_withAddress.get(true))
	{
		result += String::printf(TXT("[%p] "), codeAddress);
	}

	if (SymFromAddr(process, (ADDR_DWORD)codeAddress, &dwDisplacement, pSymbol))
	{
		String funcName = String::convert_from(pSymbol->Name);

		DWORD dwDisplacement;
		IMAGEHLP_LINE64 line;

		line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

		if (SymGetLineFromAddr64(process, (ADDR_DWORD)codeAddress, &dwDisplacement, &line))
		{
			result += String::printf(TXT("%S (%i) : %S"), String::convert_from(line.FileName).to_char(), line.LineNumber, funcName.to_char());
			return result;
		}
		else
		{
			// maybe we just do not have source
			result += funcName;
			return result;
		}
	}
	else
	{
		DWORD error = GetLastError();
		HRESULT hResult = HRESULT_FROM_WIN32(error);
		result += String::printf(TXT("SymFromAddr returned error: %d"), error);
	}
	return result;
}

LONG fault_handler(_EXCEPTION_POINTERS* _exInfo)
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (VR::IVR::get())
	{
		// for vr, there are some exceptions thrown in OpenXR, allow that to happen
		return EXCEPTION_EXECUTE_HANDLER;
	}
#endif
	if (faultHandlerIgnoreFaults)
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}

	output(TXT("fault handler"));

	if (! faultHandlerEnabled)
	{
		int fault = _exInfo && _exInfo->ExceptionRecord ? _exInfo->ExceptionRecord->ExceptionCode : 0;
		void* codeAddress = _exInfo && _exInfo->ExceptionRecord ? _exInfo->ExceptionRecord->ExceptionAddress : 0;
		output(TXT("ignoring fault (code: %p, address: %p)"), (ADDR_DWORD)fault, (ADDR_DWORD)codeAddress);
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	if (::System::Core::is_performing_quick_exit())
	{
		output(TXT("thank you for playing..."));
		// just ignore errors on quick exit "Thank you for playing" Wing Commander style!
		return EXCEPTION_EXECUTE_HANDLER;
	}

	if (!_exInfo)
	{
		output(TXT("no exception?"));
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	on_fault_handler();

	String errorInfo;
	if (_exInfo->ExceptionRecord)
	{
		switch (_exInfo->ExceptionRecord->ExceptionCode)
		{
		case EXCEPTION_ACCESS_VIOLATION:
			errorInfo = TXT("access violation"); break;
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			errorInfo = TXT("datatype misalignment"); break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			errorInfo = TXT("flt divide by zero"); break;
		case EXCEPTION_STACK_OVERFLOW:
			errorInfo = TXT("stack overflow"); break;
		default:
			errorInfo = TXT("(unknown)"); break;
		}
	}

	int fault = _exInfo->ExceptionRecord? _exInfo->ExceptionRecord->ExceptionCode : 0;
	void* codeAddress = _exInfo->ExceptionRecord? _exInfo->ExceptionRecord->ExceptionAddress : 0;
	void* firstBaseCodeAddress = nullptr;
	output_colour(1, 0, 0, 1);
	output(TXT(""));
	output(TXT(""));
	output(TXT("FAULT!"));
	output_colour(1, 1, 1, 1);
	trace_bug_info(TXT("   code: %08X"), fault);
	trace_bug_info(TXT(": %S\n"), errorInfo.to_char());
	trace_bug_info(TXT("address: %p\n"), (ADDR_DWORD)codeAddress);
	System::FaultHandler::output_modules_base_address(true, nullptr, &firstBaseCodeAddress); // read it here
	trace_bug_info(TXT("rel-adr: %p\n"), firstBaseCodeAddress != 0? (ADDR_DWORD)codeAddress - (ADDR_DWORD)firstBaseCodeAddress : 0);
	trace_bug_info(TXT("  flags: %08X\n"), _exInfo->ExceptionRecord? _exInfo->ExceptionRecord->ExceptionFlags : 0);
	output_colour();

	String csiAt(get_call_stack_info_currently_at());
	trace_bug_info(TXT("   csi@: %S\n"), csiAt.to_char());

	String gdi;
	{
		gdi = get_general_debug_info();
		output(TXT("%S"), gdi.to_char());
	}

	summary = String::printf(TXT("%S (%S), r:%p"), get_build_version(), ::System::Core::get_system_precise_name().to_char(), (ADDR_DWORD)codeAddress - (ADDR_DWORD)firstBaseCodeAddress);

	if (faultHandlerCustom)
	{
		bugInfo += TXT("info (custom):\n");
		bugInfo += faultHandlerCustom->info;
		bugInfo += TXT("\n");
		bugInfo += TXT("\n");
	}

	output_colour(0, 0, 0, 1);
	output(TXT(""));
	System::FaultHandler::output_modules_base_address();
	System::FaultHandler::output_modules_base_address(true, &bugInfo);
	output(TXT(""));
	output_colour();

	// copy log before we get another crash
	String logName = copy_log_and_switch(TXT("fault"), true);

	// 1. stack
	try
	{
		if (_exInfo->ContextRecord)
		{
			trace_bug_info(TXT("\nstack"));
			symLock.acquire();

			trace_bug_info(TXT("."));
#ifdef USE_STACK_FRAME
			auto& c = *_exInfo->ContextRecord;

			// init STACKFRAME for first call
			STACKFRAME64 s; // in/out stackframe
			memset(&s, 0, sizeof(s));
			DWORD imageType;
			trace_bug_info(TXT("."));
#ifdef _M_IX86
			// normally, call ImageNtHeader() and use machine info from PE header
			imageType = IMAGE_FILE_MACHINE_I386;
			s.AddrPC.Offset = c.Eip;
			s.AddrPC.Mode = AddrModeFlat;
			s.AddrFrame.Offset = c.Ebp;
			s.AddrFrame.Mode = AddrModeFlat;
			s.AddrStack.Offset = c.Esp;
			s.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
			imageType = IMAGE_FILE_MACHINE_AMD64;
			s.AddrPC.Offset = c.Rip;
			s.AddrPC.Mode = AddrModeFlat;
			s.AddrFrame.Offset = c.Rsp;
			s.AddrFrame.Mode = AddrModeFlat;
			s.AddrStack.Offset = c.Rsp;
			s.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
			imageType = IMAGE_FILE_MACHINE_IA64;
			s.AddrPC.Offset = c.StIIP;
			s.AddrPC.Mode = AddrModeFlat;
			s.AddrFrame.Offset = c.IntSp;
			s.AddrFrame.Mode = AddrModeFlat;
			s.AddrBStore.Offset = c.RsBSP;
			s.AddrBStore.Mode = AddrModeFlat;
			s.AddrStack.Offset = c.IntSp;
			s.AddrStack.Mode = AddrModeFlat;
#else
#error "Platform not supported!"
#endif
#else
#ifdef WIN64
			char* eNextBP = (char*)_exInfo->ContextRecord->Rsp;
#else
			char* eNextBP = (char*)_exInfo->ContextRecord->Ebp;
#endif
			char *p, *pBP;
			static int CurrentlyInTheStackDump = 0;

			trace_bug_info(TXT("."));
			if (!eNextBP)
			{
				trace_bug_info(TXT("*"));
				_asm mov     eNextBP, eBp
			}
#endif
			trace_bug_info(TXT(".\n"));

			for (unsigned int i = 0; i < 100; i++)
			{
				trace_bug_info(TXT("%02i"), i);
				trace_bug_info(TXT(" : %p"), (ADDR_DWORD)codeAddress);
				if (i == 0)
				{
					summary += TXT(" : ");
					summary += get_info(codeAddress, NP, false);
				}
				trace_bug_info(TXT(" : %S"), get_info(codeAddress).to_char());
#ifdef FAULT_HANDLER_USE_PDB
				trace_bug_info(TXT(" {%S}"), get_info(codeAddress, true).to_char());
#endif

#ifdef USE_STACK_FRAME
				HANDLE process = symInitialisedProcess;
				if (!StackWalk64(imageType, process, GetCurrentThread(), &s, &c, nullptr, nullptr, nullptr, nullptr))
				{
					break;
				}
				codeAddress = (void*)s.AddrPC.Offset;
				if (codeAddress == nullptr)
				{
					break;
				}
				trace_bug_info(TXT("\n"));
#else
				pBP = eNextBP;           // keep current BasePointer
				eNextBP = *(char**)pBP; // dereference next BP 

				// Write 20 Bytes of potential arguments
				{
					p = pBP + 8;
					trace_bug_info(TXT(" ["));
					for (unsigned int x = 0; p < eNextBP && x < 20; p++, x++)
					{
						if (x) trace_bug_info(TXT(" "));
						trace_bug_info(TXT("%02X"), *(unsigned char *)p);
					}
					trace_bug_info(TXT("]\n"));
				}

				codeAddress = *(char **)(pBP + 4);
				if (*(char **)(pBP + 4) == NULL)
					break;
				if (!eNextBP)
				{
					break;
				}
#endif
			}
			trace_bug_info(TXT("end of stack\n"));

			symLock.release();
		}
	}
	catch (...)
	{
		trace_bug_info(TXT("---stack failed\n"));
	}

	summary += String::printf(TXT(" [csi@: %S]"), csiAt.to_char());

	// 2. call stack info (other threads)
	trace_bug_info(TXT("\n%S\n\n"), get_call_stack_info().to_char());

	TCHAR currentDirBuf[16384];
	GetCurrentDirectory(16384, currentDirBuf);

	logName = String::convert_from(currentDirBuf) + TXT("\\") + logName;

#ifdef AN_DEVELOPMENT
	AN_BREAK;
#endif

	bool inClipboard = false;
	// we need to get handle to our window, we need to access video3d to do that, if it exists
	if (auto * v3d = System::Video3D::get())
	{
		HWND hwnd;
		HDC hdc;
		v3d->get_hwnd_and_hdc(hwnd, hdc);

		// open clipboard, may fail if someone else is holding it
		if (OpenClipboard(hwnd))
		{
			if (EmptyClipboard())
			{
				char* forClipboard;
				HGLOBAL hglbCopy;
				// create handle
				hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (logName.get_length() + 1) * sizeof(char));
				if (hglbCopy == NULL)
				{
					CloseClipboard();
				}
				else
				{
					// store in handle
					forClipboard = (char*)GlobalLock(hglbCopy);
					char* out = forClipboard;
					for_every(ch, logName.get_data())
					{
						*out = (char)*ch;
						++out;
					}
					GlobalUnlock(hglbCopy);

					// into the clipboard
					if (SetClipboardData(CF_TEXT, hglbCopy))
					{
						// and we're done!
						if (CloseClipboard())
						{
							inClipboard = true;
						}
					}
				}
			}
		}
	}

	String message = String::printf(TXT("The game has crashed.\n\nLog file location:\n%S"), logName.to_char());
	if (report_bug_log)
	{
		bool sendNow = MainConfig::global().get_report_bugs() == Option::True;
		if (MainConfig::global().get_report_bugs() == Option::Ask ||
			MainConfig::global().get_report_bugs() == Option::Unknown)
		{
			message += TXT("\n\nWould you like to send a report?\n\n(you can change that to always send in options)");
			int result = MessageBox(nullptr, message.to_char(), TXT("Fault!"), MB_ICONEXCLAMATION | MB_YESNO);
			sendNow = result == IDYES;
		}
		if (sendNow)
		{
			bool sent = report_bug_log(summary, bugInfo, logName);
			MessageBox(nullptr, sent? TXT("A crash report has been sent successfully.") : TXT("There was a problem while sending a report."), sent? TXT("Report sent") : TXT("Problem"), (sent? 0 : MB_ICONEXCLAMATION) | MB_OK);
		}
	}
	else
	{
		message += TXT("\n\nPlease send to contact@voidroom.com");
		if (inClipboard)
		{
			message += TXT("\n\nPath is now in the clipboard, OK to exit.");
		}
		else
		{
			message += TXT("\n\nCTRL+C to copy, OK to exit.");
		}
		MessageBox(nullptr, message.to_char(), TXT("Fault!"), MB_ICONEXCLAMATION | MB_OK);
	}

	// exit immediately
	exit(EXIT_FAILURE);

	return EXCEPTION_EXECUTE_HANDLER;
}
#endif
#ifdef AN_LINUX_OR_ANDROID
namespace Backtrace
{
	struct BacktraceState
	{
		void** current;
		void** end;
	};

	struct Demangler
	{
		char const* symbol = "";
		void const* symbolAddr = nullptr;
		char const* base = "";
		void const* baseAddr = nullptr;
		char* demangled = nullptr;

		Demangler(void const* _addr)
		{
			symbol = "";
			symbolAddr = nullptr;
			base = "";
			baseAddr = nullptr;

			if (_addr)
			{
				Dl_info info;
				if (dladdr(_addr, &info))
				{
					if (info.dli_fname)
					{
						base = info.dli_fname;
					}
					if (info.dli_sname)
					{
						symbol = info.dli_sname;
					}
					symbolAddr = info.dli_saddr;
					baseAddr = info.dli_fbase;
				}

				int status = 0;
				char* demangled = __cxxabiv1::__cxa_demangle(symbol, 0, 0, &status);

				if (demangled && status != 0)
				{
					symbol = demangled;
				}
				if (!symbol)
				{
					symbol = "";
				}
			}
		}
		~Demangler()
		{
			if (demangled)
			{
				free(demangled);
			}
		}
	};
	static _Unwind_Reason_Code unwind_callback(struct _Unwind_Context* context, void* arg)
	{
		BacktraceState* state = static_cast<BacktraceState*>(arg);
#ifdef INVESTIGATE_FAULT_HANDLER
		output(TXT("[IFH] unwind getip"));
#endif
		uintptr_t pc = _Unwind_GetIP(context);
#ifdef INVESTIGATE_FAULT_HANDLER
		output(TXT("[IFH] unwind getip [done]"));
#endif
		if (pc)
		{
#ifdef INVESTIGATE_FAULT_HANDLER
			output(TXT("[IFH] check cp"));
#endif
			if (state->current == state->end)
			{
#ifdef INVESTIGATE_FAULT_HANDLER
				output(TXT("[IFH] end of stack"));
#endif
				return _URC_END_OF_STACK;
			}
			else
			{
#ifdef INVESTIGATE_FAULT_HANDLER
				output(TXT("[IFH] add to stack"));
#endif
				*state->current++ = reinterpret_cast<void*>(pc);
			}
		}
		return _URC_NO_REASON;
	}

	size_t capture_backtrace(void** _intoBuffer, size_t _max)
	{
		BacktraceState state;
		state.current = _intoBuffer;
		state.end = _intoBuffer + (_max - 1); // better not to overwrite the last one
#ifdef INVESTIGATE_FAULT_HANDLER
		output(TXT("[IFH] unwind backtrace"));
#endif
		_Unwind_Backtrace(unwind_callback, &state);
#ifdef INVESTIGATE_FAULT_HANDLER
		output(TXT("[IFH] unwind backtrace [done]"));
#endif

		return state.current - _intoBuffer;
	}

	void log_backtrace(LogInfoContext & _log, void** buffer, size_t count, char const * _storeInLineAfter, String& _storeIn)
	{
#ifdef INVESTIGATE_FAULT_HANDLER
		output(TXT("[IFH] log backtrace [i0]"));
#endif
		int afterIdx = NONE;
		int lastPrevEmptyIdx = NONE;
		int lastEmptyIdx = NONE;
		for (size_t idx = 0; idx < count; ++idx)
		{
			const void* addr = buffer[idx];
			Demangler demangler(addr);

			if (strcmp(demangler.symbol, _storeInLineAfter) == 0)
			{
				afterIdx = idx;
			}
			if (strlen(demangler.symbol) == 0)
			{
				if (lastEmptyIdx == NONE || lastEmptyIdx < idx - 1) // we want to catch if we had a hole
				{
					lastPrevEmptyIdx = lastEmptyIdx;
				}
				lastEmptyIdx = idx;
			}
		}

#ifdef INVESTIGATE_FAULT_HANDLER
		output(TXT("[IFH] log backtrace [i1]"));
#endif

		int faultIdx = NONE;
		if (lastPrevEmptyIdx != NONE && (lastPrevEmptyIdx < count - 1)) // surely, it can't be last?
		{
			faultIdx = lastPrevEmptyIdx + 1;
		}
		else if (afterIdx != NONE)
		{
			faultIdx = afterIdx + 1;
		}

#ifdef INVESTIGATE_FAULT_HANDLER
		output(TXT("[IFH] log backtrace [i2]"));
#endif

		for (size_t idx = 0; idx < count; ++idx)
		{
			const void* addr = buffer[idx];
			Demangler demangler(addr);

			if (faultIdx == idx)
			{
				// skip __kernel_rt_sigreturn, we're interested in the next one
				if (strcmp(demangler.symbol, "__kernel_rt_sigreturn") == 0)
				{
					++faultIdx;
				}
			}
			_log.log(TXT("  %02d : [b:%p+%p] : %s (%p in %p) (base:%s) %S"), idx, demangler.baseAddr, (pointerint)addr - (pointerint)demangler.baseAddr, demangler.symbol, addr, demangler.symbolAddr, demangler.base, faultIdx == idx? TXT(" <<<<<<<<<<<<<<<<<<<<") : TXT(""));
			if (faultIdx == idx)
			{
				_storeIn = String::printf(TXT("%p : %s"), addr, demangler.symbol);
			}
		}
	}

	void log_backtrace(LogInfoContext& _log, char const * _storeInLineAfter, String & _storeIn)
	{
		const size_t max = 80;
		void* buffer[max];

#ifdef INVESTIGATE_FAULT_HANDLER
		output(TXT("[IFH] capture backtrace"));
#endif
		int count = capture_backtrace(buffer, max);
#ifdef INVESTIGATE_FAULT_HANDLER
		output(TXT("[IFH] capture backtrace [done]"));
#endif

#ifdef INVESTIGATE_FAULT_HANDLER
		output(TXT("[IFH] log backtrace"));
#endif
		log_backtrace(_log, buffer, count, _storeInLineAfter, _storeIn);
#ifdef INVESTIGATE_FAULT_HANDLER
		output(TXT("[IFH] log backtrace [done]"));
#endif
	}
}

tchar const* signum_to_char(int signum)
{
	if (signum == SIGINT) return TXT("interrupt");
	if (signum == SIGABRT) return TXT("abort");
	if (signum == SIGFPE) return TXT("floating point exception");
	if (signum == SIGILL) return TXT("illegal instruction");
	if (signum == SIGSEGV) return TXT("segmentation fault");
	if (signum == SIGTERM) return TXT("terminate");
	return TXT("??");
}

void signal_int_handler(int signum)
{
	output(TXT("interrupt signal received (%i:%S)"), signum, signum_to_char(signum));
}

void fault_handler(int signum, struct siginfo* si, void* unused)
{
	output(TXT("fault handler"));
	output(TXT("signum: %i"), signum);

	if (! faultHandlerEnabled)
	{
		output(TXT("fault disabled, ignore (%i:%S)"), signum, signum_to_char(signum));
		return;
	}
	if (! Concurrency::ThreadManager::is_current_thread_registered())
	{
		output(TXT("thread not registered, fault (%i:%S)"), signum, signum_to_char(signum));
		//return;
	}

	on_fault_handler();

	{
		output(TXT("build fault log"));

		LogInfoContext log;
		log.be_fault_log();

		log.log(TXT("fault (%i:%S)"), signum, signum_to_char(signum));

		String possibleFault;

		// do it here so we get anything
		output(TXT("early info"));
		output(TXT("build: %S"), get_build_version_short());
		String csi;
		String csiAt;
		{
			csi = get_call_stack_info();
			output(TXT("%S"), csi.to_char());
			if (faultHandlerCustom)
			{
				output(TXT("info (custom):"));
				output(TXT("%S"), faultHandlerCustom->info.to_char());
			}
			csiAt = get_call_stack_info_currently_at();
		}
		String gdi;
		{
			gdi = get_general_debug_info();
			output(TXT("%S"), gdi.to_char());
		}

		output(TXT("log backtrace"));

		Backtrace::log_backtrace(log, "__kernel_rt_sigreturn", possibleFault);
		
		output(TXT("get summary info"));

		summary = String::printf(TXT("%S (%S), %S [csi@ %S]"), get_build_version(), ::System::Core::get_system_precise_name().to_char(), possibleFault.to_char(), csiAt.to_char());
		bugInfo = TXT("");
		bugInfo += String::printf(TXT("%S\n"), get_build_version());
		output(TXT("build: %S"), get_build_version());
		bugInfo += String::printf(TXT("%S\n"), possibleFault.to_char());
		bugInfo += String::printf(TXT("csi@: %S\n"), csiAt.to_char());
		bugInfo += TXT("\n");
		bugInfo += TXT("\n");

		if (faultHandlerCustom)
		{
			bugInfo += TXT("info (custom):\n");
			bugInfo += faultHandlerCustom->info;
			bugInfo += TXT("\n");
			bugInfo += TXT("\n");
			output(TXT("info (custom):"));
			output(TXT("%S"), faultHandlerCustom->info.to_char());
		}

		output(TXT("get stack"));

		// 1. stack
		log.output_to_output();
		for_every(line, log.get_lines())
		{
			bugInfo += line->text;
			bugInfo += TXT("\n");
		}

		// 2. call stack info (other threads)
		{
			bugInfo += csi;
			bugInfo += TXT("\n");
			bugInfo += TXT("\n");
			output(TXT("%S"), csi.to_char());
		}

		// 3. general debug info
		{
			bugInfo += gdi;
			bugInfo += TXT("\n");
			bugInfo += TXT("\n");
			output(TXT("%S"), gdi.to_char());
		}

		output(TXT("switch to fault file"));

		// copy log before we get another crash
		String logName = copy_log_and_switch(TXT("fault"), true);

		output(TXT("switched to fault file"));

		if (MainConfig::global().get_report_bugs() == Option::True ||
			MainConfig::global().get_report_bugs() == Option::Ask ||
			MainConfig::global().get_report_bugs() == Option::Unknown)
		{
			output(TXT("report a bug"));

			if (report_bug_log)
			{
				report_bug_log(summary, bugInfo, logName);
			}

			output(TXT("bug reported"));
		}
		else
		{
			output(TXT("do not report fault (%i:%S)"), signum, signum_to_char(signum));
		}
	}

#ifndef BUILD_PREVIEW_RELEASE
#ifndef BUILD_PUBLIC_RELEASE
	error_stop(TXT("signal received (fault?) (%i:%S)"), signum, signum_to_char(signum));
#endif
#endif

	// exit immediately
	std::quick_exit(signum);
}

void System::FaultHandler::output_modules_base_address(bool _justFirstOne, String* _outputTo, void** _firstBaseAddr)
{
	// nothing, not supported yet
}

String System::FaultHandler::get_info(uint64 _codeAddress, Optional<bool> const& _withAddress)
{
	return String(TXT("<unsupported>"));
}
#endif

void System::FaultHandler::initialise_static()
{
	an_assert(!faultHandlerCustom);
	faultHandlerCustom = new FaultHandlerCustom();
	an_assert(!CodeInfo::one);
	CodeInfo::one = new CodeInfo();

// register even in development builds
#ifndef DISABLE_FAULT_HANDLER_AT_ALL
#ifdef AN_WINDOWS
	// install fault handler
#ifdef FAULT_HANDLER_DURING_DEV
	AddVectoredExceptionHandler(1, (LPTOP_LEVEL_EXCEPTION_FILTER)fault_handler);
#else
	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)fault_handler);
#endif
#endif
#endif

	summary.access_data().make_space_for(16384);
	bugInfo.access_data().make_space_for(16384);

	FaultHandler::enable();
}

void System::FaultHandler::close_static()
{
	delete_and_clear(faultHandlerCustom);
	delete_and_clear(CodeInfo::one);
}

void System::FaultHandler::set_ignore_faults(bool _ignoreFaults)
{
	faultHandlerIgnoreFaults = _ignoreFaults;
}

void System::FaultHandler::set_report_bug_log(std::function<bool(String const& _summary, String const& _info, String const& _filename)> _reportFunc)
{
	report_bug_log = _reportFunc;
}

void System::FaultHandler::store_custom_info(String const& _customInfo)
{
	if (faultHandlerCustom)
	{
		faultHandlerCustom->info = _customInfo;
	}
}

void System::FaultHandler::add_custom_info(String const& _customInfo)
{
	if (faultHandlerCustom)
	{
		faultHandlerCustom->info += TXT("\n");
		faultHandlerCustom->info += _customInfo;
	}
}

String System::FaultHandler::get_custom_info()
{
	if (faultHandlerCustom)
	{
		return faultHandlerCustom->info;
	}
	else
	{
		return String::empty();
	}
}

#ifdef AN_LINUX_OR_ANDROID
struct sigaction prevAction;

void switch_sig_action(int _sig, struct sigaction* setToAction, struct sigaction* storePrevIn = nullptr)
{
	if (sigaction(_sig, setToAction, storePrevIn) < 0)
	{
		output(TXT("error setting sig action"));
	}
}

void switch_sig_action(struct sigaction* setToAction, struct sigaction* storePrevIn = nullptr)
{
	/* ... add any other signal here */
	//signal(SIGINT, signal_int_handler); - allow interruptions, maybe it is something important and if we cut in, we could make a mess

	switch_sig_action(SIGABRT, setToAction, storePrevIn); // this happens when we do bad things
	switch_sig_action(SIGFPE, setToAction, storePrevIn);
	switch_sig_action(SIGILL, setToAction, storePrevIn);
	switch_sig_action(SIGSEGV, setToAction, storePrevIn);
	//switch_sig_action(SIGTERM, setToAction, storePrevIn); // - allow to be normally terminated
}
#endif

void System::FaultHandler::disable()
{
	if (faultHandlerEnabled)
	{
#ifndef DISABLE_FAULT_HANDLER_AT_ALL
#ifdef AN_LINUX_OR_ANDROID
		switch_sig_action(&prevAction);
#endif
#endif
	}
	faultHandlerEnabled = false;
}

void System::FaultHandler::enable()
{
	if (!faultHandlerEnabled)
	{
#ifndef DISABLE_FAULT_HANDLER_AT_ALL
#ifdef AN_LINUX_OR_ANDROID
		struct sigaction sa;
		memory_zero(&sa, sizeof(sa));
		sa.sa_flags = SA_SIGINFO;
		sa.sa_sigaction = fault_handler;

		switch_sig_action(&sa, &prevAction);
#endif
#endif
	}
	faultHandlerEnabled = true;
}

