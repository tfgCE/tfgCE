#include "raport.h"

#include "entry.h"

#include "category.h"

#include "..\core\io\dir.h"
#include "..\core\io\file.h"
#include "..\core\math\math.h"
#include "..\core\other\parserUtils.h"

using namespace Budget;


bool read_any_token(IO::Interface* _file, Token & _token);
bool read_quoted_token(IO::Interface* _file, Token & _token);

Raport::Raport()
: period(Month)
, inColumns(false)
, expandCategories(false)
, reverseDate(false)
, costs(false)
{
}

bool Raport::load_raport_from(IO::File* _file, List<Raport*> & _raports)
{
	_file->set_type(IO::InterfaceType::Text);

	Raport* raport = new Raport();
	while (true)
	{
		tchar ch = _file->read_char();
		if (ch == 13 ||
			ch == 10)
		{
			break;
		}
		raport->name += ch;
	}

	bool result = true;

	Token token;
	while (true)
	{
		if (!read_any_token(_file, token))
		{
			break;
		}
		if (token == TXT("month") ||
			token == TXT("monthly") ||
			token == TXT("byMonth"))
		{
			raport->period = Month;
		}
		else if (token == TXT("year") ||
				 token == TXT("yearly") ||
				 token == TXT("annual"))
		{
			raport->period = Year;
		}
		else if (token == TXT("inColumns"))
		{
			raport->inColumns = true;
		}
		else if (token == TXT("inRows"))
		{
			raport->inColumns = false;
		}
		else if (token == TXT("expandCategories"))
		{
			raport->expandCategories = true;
		}
		else if (token == TXT("expandCategories0"))
		{
			raport->expandCategories = true;
			raport->expandCategoriesLevel = 0;
		}
		else if (token == TXT("expandCategories1"))
		{
			raport->expandCategories = true;
			raport->expandCategoriesLevel = 1;
		}
		else if (token == TXT("expandCategories2"))
		{
			raport->expandCategories = true;
			raport->expandCategoriesLevel = 2;
		}
		else if (token == TXT("expandCategories3"))
		{
			raport->expandCategories = true;
			raport->expandCategoriesLevel = 3;
		}
		else if (token == TXT("reverseDate"))
		{
			raport->reverseDate = true;
		}
		else if (token == TXT("costs"))
		{
			raport->costs = true;
		}
		else if (token == TXT("categories"))
		{
			while (true)
			{
				if (!read_any_token(_file, token))
				{
					result = false;
					error(TXT("no endCategories"));
				}
				if (token == TXT("endCategories"))
				{
					break;
				}
				Category* category = Categories::find_category(Name(token.to_char()));
				if (!category)
				{
					error(TXT("category not recognised \"%S\""), token.to_char());
					delete raport;
					return false;
				}
				raport->categories.push_back(category);
			}
		}
	}

	if (result)
	{
		_raports.push_back(raport);
	}
	else
	{
		delete raport;
	}

	return result;
}

bool Raport::load_dir(String const & _dir, List<Raport*> & _raports)
{
	bool result = true;
	IO::Dir dir;
	if (dir.list(_dir))
	{
		Array<String> const & directories = dir.get_directories();
		for_every(directory, directories)
		{
			result &= load_dir(_dir + TXT("/") + *directory, _raports);
		}

		Array<String> const & files = dir.get_files();
		for_every(file, files)
		{
			output(TXT("  %S"), file->to_char());
			result &= load_raport_from(&IO::File(_dir + TXT("\\") + *file), _raports);
		}
	}
	return result;
}

struct RaportPeriodEntry
{
	Date date;
	int totalSum = 0;
	Array<int> values;
	Array<Category*> const & doCategories;

	RaportPeriodEntry(Date & _date, Array<Category*> const & _doCategories)
	: doCategories(_doCategories)
	{
		date = _date;
		values.set_size(_doCategories.get_size());
		for_every(value, values)
		{
			*value = 0;
		}
	}

	bool add(Entry* _entry)
	{
		int index = 0;
		for_every_ptr(doCategory, doCategories)
		{
			if (_entry->get_category()->does_belong_to(doCategory))
			{
				values[index] += _entry->get_value();
				totalSum += _entry->get_value();
				return true;
			}
			++index;
		}
		return false;
	}
};

void raport_add_categories(Array<Category*> & _doCategories, Category* _category, bool _expandAll, Optional<int> _levelBelow)
{
	if (_expandAll && ! _category->get_has().is_empty() && (!_levelBelow.is_set() || _levelBelow.get() > 0))
	{
		for_every_ptr(category, _category->get_has())
		{
			raport_add_categories(_doCategories, category, _expandAll, _levelBelow.is_set() ? Optional<int>(_levelBelow.get() - 1) : NP);
		}
	}
	else
	{
		_doCategories.push_back_unique(_category);
	}
}

String format_bar(tchar _char, int _length)
{
	String result;
	while (result.get_length() < _length)
	{
		result += _char;
	}
	return result;
}

String format_centre(tchar const * _text, int _length)
{
	String result;
	int textLen = (int)tstrlen(_text);
	if (textLen > _length)
	{
		result = _text;
		return result.get_left(_length);
	}
	else
	{
		for (int i = 0; i < (_length - textLen) / 2; ++i)
		{
			result += TXT(" ");
		}
		result += _text;
		while (result.get_length() < _length)
		{
			result += TXT(" ");
		}
		return result;
	}
}

String format_left(tchar const * _text, int _length)
{
	String result;
	int textLen = (int)tstrlen(_text);
	if (textLen > _length)
	{
		result = _text;
		return result.get_left(_length);
	}
	else
	{
		result += _text;
		while (result.get_length() < _length)
		{
			result += TXT(" ");
		}
		return result;
	}
}

String format_right(tchar const * _text, int _length)
{
	String result;
	int textLen = (int)tstrlen(_text);
	if (textLen > _length)
	{
		result = _text;
		return result.get_right(_length);
	}
	else
	{
		result += _text;
		while (result.get_length() < _length)
		{
			result = String::space() + result;
		}
		return result;
	}
}

String format_value(int _value)
{
	int absValue = abs(_value);
	String integ = String::printf(TXT("%i"), absValue / 100);
	for (int i = integ.get_length() - 3; i > 0; i -= 3)
	{
		integ = integ.get_left(i) + TXT(" ") + integ.get_sub(i);
	}
	return String::printf(TXT("%S%S.%02i"), _value < 0 ? TXT("-") : TXT(""), integ.to_char(), absValue % 100);
}

void Raport::process(String const & _outputDir, List<Entry*> const & _entries)
{
	output(TXT("  %S"), name.to_char());
	// get all categories we want to have
	Array<Category*> doCategories;
	for_every_ptr(category, categories)
	{
		raport_add_categories(doCategories, category, expandCategories, expandCategoriesLevel);
	}

	Array<RaportPeriodEntry*> periodEntries;

	for_every_ptr(entry, _entries)
	{
		Date date = entry->get_date();
		// get date into valid values
		date.month = clamp(date.month, 1, 12);
		date.day = clamp(date.day, 1, 31);
		// remove if we don't need something
		if (period == Month)
		{
			date.day = 0;
		}
		else if (period == Year)
		{
			date.month = 0;
			date.day = 0;
		}
		RaportPeriodEntry* foundPeriodEntry = nullptr;
		for_every_ptr(periodEntry, periodEntries)
		{
			if (periodEntry->date == date)
			{
				foundPeriodEntry = periodEntry;
				break;
			}
		}

		if (!foundPeriodEntry)
		{
			foundPeriodEntry = new RaportPeriodEntry(date, doCategories);
			int indexAt = 0;
			for_every_ptr(periodEntry, periodEntries)
			{
				if ((reverseDate && periodEntry->date < date) ||
					(! reverseDate && date < periodEntry->date))
				{
					break;
				}
				++indexAt;
			}
			periodEntries.insert_at(indexAt, foundPeriodEntry);
		}

		if (!foundPeriodEntry->add(entry))
		{
			bool isOk = true;
			for_every_ptr(category, categories)
			{
				if (entry->get_category()->does_belong_to(category))
				{
					isOk = false;
					break;
				}
			}
			if (!isOk)
			{
				error(TXT("couldn't find entry for \"%S\" of category \"%S\""), entry->get_info().to_char(), entry->get_category()->get_name().to_char());
			}
		}
	}

	{
		IO::File outputFile;
		outputFile.create(_outputDir + TXT("\\") + name);
		outputFile.set_type(IO::InterfaceType::Text);

		if (inColumns)
		{
			int categoryWidth = 24;
			int sumWidth = 16;
			int columnWidth = 14;
			int barEveryCat = 5;
			String categoryBar = format_bar('-', categoryWidth);
			String sumBar = format_bar('-', sumWidth);
			String columnBar = format_bar('-', columnWidth);

			String barLine;
			barLine += TXT("+");
			barLine += categoryBar;
			barLine += TXT("+");
			barLine += sumBar;
			for_every_ptr(periodEntry, periodEntries)
			{
				barLine += TXT("+");
				barLine += columnBar;
			}
			barLine += TXT("+");

			// header
			outputFile.write_text(barLine);
			outputFile.write_text("\n");

			outputFile.write_text("|");
			outputFile.write_text(format_centre(TXT("category"), categoryWidth));
			outputFile.write_text("|");
			outputFile.write_text(format_centre(TXT("sum"), sumWidth));
			for_every_ptr(periodEntry, periodEntries)
			{
				outputFile.write_text("|");
				outputFile.write_text(format_centre(periodEntry->date.to_string_date().to_char(), columnWidth));
			}
			outputFile.write_text("|");
			outputFile.write_text("\n");

			if (barEveryCat == 0)
			{
				outputFile.write_text(barLine);
				outputFile.write_text("\n");
			}
			// sum of all
			{
				outputFile.write_text(barLine);
				outputFile.write_text("\n");

				outputFile.write_text("|");
				outputFile.write_text(format_centre(TXT("+++"), categoryWidth));
				outputFile.write_text("|");

				int sum = 0;
				for_every_ptr(periodEntry, periodEntries)
				{
					sum += periodEntry->totalSum * (costs ? -1 : 1);
				}
				outputFile.write_text(format_right(format_value(sum).to_char(), sumWidth));
				for_every_ptr(periodEntry, periodEntries)
				{
					outputFile.write_text("|");
					outputFile.write_text(format_right(format_value(periodEntry->totalSum * (costs ? -1 : 1)).to_char(), columnWidth));
				}
				outputFile.write_text("|");
				outputFile.write_text("\n");
			}
			// per category
			int categoryIndex = 0;
			for_every_ptr(category, doCategories)
			{
				if (barEveryCat > 0 && (categoryIndex % barEveryCat) == 0)
				{
					outputFile.write_text(barLine);
					outputFile.write_text("\n");
				}

				outputFile.write_text("|");
				outputFile.write_text(format_centre(category->get_name().to_char(), categoryWidth));
				outputFile.write_text("|");
				int sum = 0;
				for_every_ptr(periodEntry, periodEntries)
				{
					sum += periodEntry->values[categoryIndex] * (costs? -1 : 1);
				}
				outputFile.write_text(format_right(format_value(sum).to_char(), sumWidth));
				for_every_ptr(periodEntry, periodEntries)
				{
					outputFile.write_text("|");
					outputFile.write_text(format_right(format_value(periodEntry->values[categoryIndex] * (costs ? -1 : 1)).to_char(), columnWidth));
				}
				outputFile.write_text("|");
				outputFile.write_text("\n");

				++categoryIndex;
			}

			// end
			outputFile.write_text(barLine);
			outputFile.write_text("\n");
		}
	}

	for_every_ptr(periodEntry, periodEntries)
	{
		delete periodEntry;
	}
}
