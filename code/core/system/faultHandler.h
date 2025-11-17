#pragma once

#include "..\globalDefinitions.h"

#include "..\types\optional.h"

#include <functional>

struct String;

namespace IO
{
	class File;
};

namespace System
{
	class FaultHandler
	{
	public:
		static void initialise_static();
		static void close_static();

		static void disable();
		static void enable();

		static void create_code_info(IO::File& _saveTo);
		static void load_code_info(IO::File& _loadFrom);

		static void output_modules_base_address(bool _justFirstOne = false, String * _outputTo = nullptr, void** _firstBaseAddr = nullptr);
		static String get_info(uint64 _relCodeAddress, Optional<bool> const& _withAddress = NP);

		static void set_report_bug_log(std::function<bool(String const& _summary, String const& _info, String const& _filename)> _reportFunc);

		static void store_custom_info(String const & _customInfo);
		static void add_custom_info(String const& _customInfo);
		static String get_custom_info();

		static void set_ignore_faults(bool _ignoreFaults);
	};
};
