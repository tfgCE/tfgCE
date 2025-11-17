// main app

#include <string>
#include <iterator>
#include <iostream>

#include "..\core\coreInit.h"
#include "..\core\containers\map.h"
#include "..\core\io\file.h"
#include "..\core\io\dir.h"
#include "..\core\math\math.h"
#include "..\core\system\core.h"

#include "date.h"
#include "category.h"
#include "entry.h"
#include "raport.h"
#include "crypto.h"

void parse_account_files()
{
	IO::Dir dir;
	if (dir.list(String(TXT("account"))))
	{
		Array<String> const & files = dir.get_files();
		for_every(file, files)
		{
			if (file->get_right(4) == TXT(".csv"))
			{
				output(TXT("   %S"), file->to_char());
				IO::File fIn;
				fIn.open(String(TXT("account\\")) + *file);
				fIn.set_type(IO::InterfaceType::Text);
				IO::File fOut;
				fOut.create(String(TXT("account\\")) + file->get_left(file->get_length() - 4) + TXT(".txt"));
				fOut.set_type(IO::InterfaceType::Text);
				if (fIn.is_open() && fOut.is_open())
				{
					List<String> lines;
					fIn.read_lines(lines);
					Budget::Date prevDate;
					String prevDateTxt;
					String prevValue;
					while (true)
					{
						bool allSorted = true;
						for (int i = 1; i < lines.get_size(); ++i)
						{
							if (lines[i].get_length() > 1 &&
								lines[i - 1].get_length() > 1)
							{
								if (String::diff_icase(lines[i - 1].get_left(10), lines[i].get_left(10)) > 0)
								{
									//output(TXT("swapping..."));
									//output(TXT("  %05i %S"), i - 1, lines[i - 1].to_char());
									//output(TXT("  %05i %S"), i, lines[i].to_char());
									swap(lines[i - 1], lines[i]);
									allSorted = false;
								}
							}
						}
						if (allSorted)
						{
							break;
						}
					}
					for_every(line, lines)
					{
						if (line->get_length() > 1)
						{
							int first = line->find_first_of(';');
							Budget::Date date;
							String dateTxt = line->get_left(first);
							date.parse_yyyy_mm_dd_minus(dateTxt);
							String value;
							int at = line->get_length() - 2;
							while (at >= 0 && (*line)[at] != ';')
							{
								if ((*line)[at] != ' ')
								{
									value = String::empty() + (*line)[at] + value;
								}
								--at;
							}
							if (!prevDateTxt.is_empty())
							{
								while (String::diff_icase(prevDateTxt, dateTxt) < 0)
								{
									fOut.write_text(prevDateTxt);
									fOut.write_text(TXT("\t"));
									fOut.write_text(prevValue);
									fOut.write_text(TXT("\n"));
									prevDate.advance_day();
									prevDateTxt = prevDate.to_string_date();
								}
							}
							prevDateTxt = dateTxt;
							prevValue = value;
							prevDate = date;
						}
					}
					fOut.write_text(prevDateTxt);
					fOut.write_text(TXT("\t"));
					fOut.write_text(prevValue);
					fOut.write_text(TXT("\n"));
				}
			}
		}
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
#ifdef AN_CHECK_MEMORY_LEAKS
	mark_static_allocations_done_for_memory_leaks();
#endif

	core_initialise_static();

	Budget::Categories::initialise_static();

	/*
	Number a;
	Number b;

	a.parse(String(TXT("19929.17")));
	b.parse(String(TXT("-0.35031071")));

	//a.parse(String(TXT("3.2")));
	//b.parse(String(TXT("1.6")));
	a.parse(String(TXT("5.32")));
	b.parse(String(TXT("0.001")));

	//Number c = a + b;
	//Number c = a - b;
	//Number c = a * b;
	Number c = a / b;
	output(TXT("%S op %S = %S"), a.to_string().to_char(), b.to_string().to_char(), c.to_string().to_char());
	*/

	/*
	Budget::Date a;
	Budget::Date b;

	a.parse_yyyy_mm_dd_minus(String(TXT("2018-02-01")));
	b.parse_yyyy_mm_dd_minus(String(TXT("2018-02-01")));
	b.hour = 12;
	int dist = Budget::Date::distance_between(a, b);
	output(TXT("dist %i"), dist);
	*/

	output(TXT("crypto"));
	process_crypto(String(TXT("input.crypto")), String(TXT("output\\crypto")));

	output(TXT("account files"));
	parse_account_files();

	output(TXT("loading categories"));
	if (Budget::Categories::load_from(&IO::File(String(TXT("settings\\categories")))))
	{
		output(TXT("loading input"));
		List<Budget::Entry*> entries;
		if (Budget::Entry::load_dir(String(TXT("input")), entries))
		{
			output(TXT("loading raports"));
			List<Budget::Raport*> raports;
			if (Budget::Raport::load_dir(String(TXT("raports")), raports))
			{
				output(TXT("processing raports"));
				for_every_ptr(raport, raports)
				{
					raport->process(String(TXT("output")), entries);
				}
				output(TXT("everything processed fine"));
			}
			for_every_ptr(raport, raports)
			{
				delete raport;
			}
		}
		for_every_ptr(entry, entries)
		{
			delete entry;
		}
	}

	::System::Core::quick_exit(); // why not!

	Budget::Categories::close_static();

	core_close_static();

	return 0;
}
