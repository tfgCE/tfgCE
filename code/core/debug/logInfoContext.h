#pragma once

#include "..\types\string.h"
#include "..\containers\list.h"

#include "..\types\colour.h"
#include "..\types\optional.h"

#include <functional>

namespace IO
{
	class File;
};

struct LogInfoContext
{
	typedef std::function<void()> Do;
	struct LogLine
	{
		int frame;
		float coreTime;
		String text;
		Optional<Colour> colour;
		Do on_select = nullptr;
		Do on_deselect = nullptr;
		int indentDepth = 0;
		LogLine() {}
		LogLine(String const & _text, Optional<Colour> const & _colour) : frame(NONE), coreTime(0.0f), text(_text.trim_right()), colour(_colour) {}
	};

	LogInfoContext(bool _storeTimeInfo = false) : storeTimeInfo(_storeTimeInfo) {}
	~LogInfoContext();

	void be_fault_log(bool _faultLog = true) { faultLog = _faultLog; }

	Optional<Colour> const& get_colour() const { return colour; }
	void set_colour(Optional<Colour> const & _colour = NP) { colour = _colour; }
	void log(LogInfoContext const & _log);
	void log(LogLine const & _line);
	void log(tchar const * const _text = nullptr, ...);
	void on_last_log_select(Do _on_select, Do _on_deselect = nullptr);
	void increase_indent();
	void decrease_indent();
	String const & get_indent() const { return indent; }
	
	void clear();
	List<LogLine> const & get_lines() const { return lines; }

	void output_to_output(bool _withColours = true) const;
	void output_to_output_as_single_line() const;

	void output_to_string(String & _string) const;

	void set_output_to_file(String const & _fileName);

private:
	bool faultLog = false;
	bool storeTimeInfo = false;

	String indent;
	int indentDepth = 0;

	Optional<Colour> colour;

	List<LogLine> lines;

	IO::File* outputFile = nullptr;
	String outputFileName; // open file only if something logged
};

struct LogIndent
{
	LogIndent(LogInfoContext& _log)
	: log(_log)
	{
		log.increase_indent();
	}

	~LogIndent()
	{
		log.decrease_indent();
	}
private:
	LogInfoContext& log;
};

#define LOG_INDENT(_log) LogIndent temp_variable_named(indent)(_log)