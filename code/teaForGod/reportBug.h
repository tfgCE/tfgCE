#pragma once

#include "..\core\globalDefinitions.h"
#include "..\core\types\string.h"

#include <functional>

struct String;

namespace TeaForGodEmperor
{
	struct ReportBugImpl;

	struct ReportBug
	{
	public:
		// files should be relative to main directory, get_user_game_data_directory will be added automatically
		static bool send(String const& _summary, String const& _info, String const& _filename, tchar const* _reason, tchar const* _category, tchar const * _logNameInfo, std::function<void(String const& _info)> _update_info = nullptr);
		static bool send_crash(String const& _summary, String const& _info, String const& _filename) { return send(_summary, _info, _filename, TXT("a crash occured"), TXT("Crashes"), TXT("log")); }

		static bool send_quick_note_in_background(String const& _text, String const& _additionalText = String::empty());
		static bool send_full_log_info_in_background(String const& _text, String const& _additionalText = String::empty());

	private:
		ReportBugImpl* impl = nullptr;

		ReportBug();
		~ReportBug();

		bool send_report(String const& _summary, String const& _info, String const& _filename, tchar const* _reason, tchar const* _category, tchar const * _logNameInfo, std::function<void(String const& _info)> _update_info);
		
		static bool send_in_background(String const& _summary, String const& _info, String const& _filename, tchar const* _reason, tchar const* _category, tchar const * _logNameInfo);
	};
}