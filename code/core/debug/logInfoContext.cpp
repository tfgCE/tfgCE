#include "logInfoContext.h"

#include "..\io\file.h"
#include "..\system\core.h"

LogInfoContext::~LogInfoContext()
{
	delete_and_clear(outputFile);
}

void LogInfoContext::increase_indent()
{
	indent += TXT("  "); 
	++ indentDepth;
}

void LogInfoContext::decrease_indent()
{
	indent = indent.get_left(max(0, indent.get_length() - 2));
	indentDepth = max(0, indentDepth - 1);
}

void LogInfoContext::log(LogInfoContext const & _log)
{
	for_every(l, _log.get_lines())
	{
		log(*l);
	}
}

void LogInfoContext::log(LogLine const & _line)
{
	lines.push_back(_line);
	lines.get_last().text = String::printf(TXT("%S%S"), indent.to_char(), lines.get_last().text.to_char());
	lines.get_last().indentDepth += indentDepth;
}

#define FAULT_HANDLER_BUF_SIZE 2048
tchar faultHandlerBuf[FAULT_HANDLER_BUF_SIZE + 1];

void LogInfoContext::log(tchar const * const _text, ...)
{
	if (!_text)
	{
		return;
	}
	if (!outputFile && !outputFileName.is_empty())
	{
		outputFile = new IO::File();
		outputFile->create(IO::get_user_game_data_directory() + outputFileName);
		outputFile->set_type(IO::InterfaceType::Text);
	}

	int bufSize = faultLog? FAULT_HANDLER_BUF_SIZE : 8192;
	tchar* ubuf;
	if (faultLog)
	{
		ubuf = faultHandlerBuf;
	}
	else
	{
		ubuf = (tchar*)allocate_stack(sizeof(tchar) * bufSize);
	}

	va_list list;
	va_start(list, _text);
#ifndef AN_CLANG
	int textsize = (int)(tstrlen(_text) + 1);
	int memsize = textsize * sizeof(tchar);
	allocate_stack_var(tchar, format, textsize);
	memory_copy(format, _text, memsize);
	sanitise_string_format(format);
#else
	tchar const* format = _text;
#endif
	tvsprintf(ubuf, bufSize, format, list);

	if (! faultLog)
	{
		int at = 0;
		int lineLength = (int)tstrlen(ubuf);
		while (at < lineLength || at == 0)
		{
			int nextAt = at;
			while (nextAt < lineLength && ubuf[nextAt] != '\n')
			{
				++nextAt;
			}
			ubuf[nextAt] = 0;
			if (storeTimeInfo)
			{
				int frame = System::Core::get() ? System::Core::get_frame() : NONE;
				float coreTime = System::Core::get() ? System::Core::get_time_since_core_started() : 0.0f;
				String line = String::printf(TXT("%06i | %08.3f | %S%S"), frame, coreTime, indent.to_char(), &ubuf[at]);
				lines.push_back(LogLine(line, colour));
				lines.get_last().frame = frame;
				lines.get_last().coreTime = coreTime;
				if (outputFile)
				{
					outputFile->write_text(line.to_char());
					outputFile->write_text(TXT("\n"));
					outputFile->flush();
				}
			}
			else
			{
				String line = String::printf(TXT("%S%S"), indent.to_char(), &ubuf[at]);
				lines.push_back(LogLine(line, colour));
				if (outputFile)
				{
					outputFile->write_text(line.to_char());
					outputFile->write_text(TXT("\n"));
					outputFile->flush();
				}
			}
			at = nextAt + 1;
		}
	}
	else
	{
		an_assert(!faultLog);
		lines.push_back(LogLine(String::printf(TXT("%S%S"), indent.to_char(), ubuf), colour));
		if (outputFile)
		{
			outputFile->write_text(String::printf(TXT("%S%S"), indent.to_char(), ubuf).to_char());
			outputFile->write_text(TXT("\n"));
			outputFile->flush();
		}
	}
	lines.get_last().indentDepth = indentDepth;
}

void LogInfoContext::on_last_log_select(Do _on_select, Do _on_deselect)
{
	if (lines.is_empty())
	{
		return;
	}
	lines.get_last().on_select = _on_select;
	lines.get_last().on_deselect = _on_deselect;
}

void LogInfoContext::clear()
{
	lines.clear();
}

void LogInfoContext::output_to_string(String & _string) const
{
	for_every(line, lines)
	{
		_string += line->text;
		_string += TXT("\n");
	}
}

void LogInfoContext::output_to_output(bool _withColours) const
{
	ScopedOutputLock outputLock;
	for_every(line, lines)
	{
		if (_withColours)
		{
			if (line->colour.is_set())
			{
				output_colour(line->colour.get().r > 0.25f, line->colour.get().g > 0.25f, line->colour.get().b > 0.25f,
							  max(line->colour.get().r, max(line->colour.get().g, line->colour.get().b)) > 0.6f);
			}
			else
			{
				output_colour();
			}
		}
		output(TXT("%S"), line->text.to_char());
	}
	if (_withColours)
	{
		output_colour();
	}
}

void LogInfoContext::output_to_output_as_single_line() const
{
	ScopedOutputLock outputLock; 
	for_every(line, lines)
	{
		trace(TXT("%S; "), line->text.to_char());
	}
	trace(TXT("\n"));
}

void LogInfoContext::set_output_to_file(String const & _fileName)
{
	delete_and_clear(outputFile);
	outputFileName = _fileName;
}
