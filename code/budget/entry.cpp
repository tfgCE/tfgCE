#include "entry.h"

#include "category.h"

#include "..\core\io\dir.h"
#include "..\core\io\file.h"
#include "..\core\other\parserUtils.h"

using namespace Budget;

bool read_any_token(IO::Interface* _file, Token & _token)
{
	_token.clear();
	if (!_file->has_last_read_char())
	{
		_file->read_char();
	}
	while (true)
	{
		if (!_file->has_last_read_char())
		{
			return !_token.is_empty();
		}
		tchar ch = _file->get_last_read_char();
		if (ch != ' ' &&
			ch != '\t' &&
			ch != '\n' &&
			ch != 13)
		{
			_token += ch;
		}
		else
		{
			if (_token.get_length() > 0)
			{
				return true;
			}
		}
		_file->read_char();
	}
	return false;
}

bool read_quoted_token(IO::Interface* _file, Token & _token)
{
	_token.clear();
	if (!_file->has_last_read_char())
	{
		_file->read_char();
	}
	tchar ch = _file->get_last_read_char();
	bool quotes = ch == '"';
	if (quotes)
	{
		_file->read_char();
	}
	while (true)
	{
		if (!_file->has_last_read_char())
		{
			return !_token.is_empty();
		}
		tchar ch = _file->get_last_read_char();
		if ((quotes && ch == '"') ||
			(!quotes && ch == ',') ||
			ch == '\t' ||
			ch == '\n' ||
			ch == 13)
		{
			if (quotes)
			{
				_file->read_char();
			}
			return true;
		}
		else
		{
			_token += ch;
		}
		_file->read_char();
	}
	return false;
}

bool Entry::load_entries_from_gdocs_csv(IO::File* _file, List<Entry*> & _entries)
{
	_file->set_type(IO::InterfaceType::Text);

	float multiplier = 1.0f;
	Token token;
	read_any_token(_file, token);
	if (token == TXT("cost"))
	{
		multiplier = -1.0f;
	}
	else if (token == TXT("gain"))
	{
		multiplier = 1.0f;
	}
	else
	{
		error(TXT("please put cost or gain at the begining of file"));
	}
	Array<Category*> categoryColumns;
	int dateAtColumn = NONE;
	int infoAtColumn = NONE;
	int categoryAtColumn = NONE;
	int valueAtColumn = NONE;
	while (true)
	{
		read_any_token(_file, token);
		if (token == TXT("."))
		{
			break;
		}
		else if (token == TXT("-"))
		{
			categoryColumns.push_back(nullptr);
		}
		else if (token == TXT("c"))
		{
			categoryAtColumn = categoryColumns.get_size();
			categoryColumns.push_back(nullptr);
		}
		else if (token == TXT("v"))
		{
			valueAtColumn = categoryColumns.get_size();
			categoryColumns.push_back(nullptr);
		}
		else if (token == TXT("d"))
		{
			dateAtColumn = categoryColumns.get_size();
			categoryColumns.push_back(nullptr);
		}
		else if (token == TXT("i"))
		{
			infoAtColumn = categoryColumns.get_size();
			categoryColumns.push_back(nullptr);
		}
		else
		{
			Category* category = Categories::find_category(Name(token.to_char()));
			if (!category)
			{
				error(TXT("category not recognised \"%S\""), token.to_char());
				return false;
			}
			categoryColumns.push_back(category);
		}
	}
	if (categoryColumns.get_size() != 0)
	{
		if (dateAtColumn == NONE &&
			infoAtColumn == NONE)
		{
			error(TXT("not all required columns defined"));
			return false;
		}
	}
	while (true)
	{
		// read date
		Date date;
		String categoryName;
		String info;
		String value;
		Array<Name> readCategories;
		Array<int> readValues;
		if (categoryColumns.get_size() != 0)
		{
			for (int c = 0; c < categoryColumns.get_size(); ++c)
			{
				if (c == dateAtColumn)
				{
					if (!date.load_from_gdocs_csv(_file))
					{
						// end of file
						break;
					}
					_file->read_char(); // ,
				}
				else if (c == infoAtColumn)
				{
					read_quoted_token(_file, token);
					info = token.to_char();
				}
				else if (c == categoryAtColumn)
				{
					read_quoted_token(_file, token);
					categoryName = token.to_char();
				}
				else if (c == valueAtColumn)
				{
					read_quoted_token(_file, token);
					value = token.to_char();
				}
				else
				{
					read_quoted_token(_file, token);
					if (!token.is_empty() && categoryColumns[c])
					{
						readCategories.push_back(categoryColumns[c]->get_name());
						String tValue = String(token.to_char());
						tValue = tValue.replace(String(TXT("")) + (tchar)194 + (tchar)160, String(TXT("")));
						tValue = tValue.replace(String(TXT("")) + (tchar)160, String(TXT("")));
						tValue = tValue.replace(String::space(), String(TXT("")));
						tValue = tValue.replace(String(TXT(",")), String(TXT(".")));
						readValues.push_back((int)(multiplier * ParserUtils::parse_float(tValue) * 100.0f));
					}
				}
				if (_file->get_last_read_char() == ',')
				{
					_file->read_char();
				}
			}
		}
		else
		{
			if (!date.load_from_gdocs_csv(_file))
			{
				// end of file
				break;
			}

			_file->read_char(); // ,

			// category name
			_file->read_char(); // first character
			while (_file->has_last_read_char() && _file->get_last_read_char() != ',')
			{
				categoryName += _file->get_last_read_char();
				_file->read_char();
			}

			read_quoted_token(_file, token);
			info = token.to_char();

			read_quoted_token(_file, token);
			value = token.to_char();
		}

		if (! categoryName.is_empty())
		{
			readCategories.push_back(Name(categoryName));
			value = value.replace(String(TXT("")) + (tchar)194 + (tchar)160, String(TXT("")));
			value = value.replace(String(TXT("")) + (tchar)160, String(TXT("")));
			value = value.replace(String::space(), String(TXT("")));
			value = value.replace(String(TXT(",")), String(TXT(".")));
			int iVal = (int)(multiplier * ParserUtils::parse_float(value) * 100.0f);
			readValues.push_back(iVal);
		}
		for (int rc = 0; rc < readCategories.get_size(); ++rc)
		{
			Category* category = Categories::find_category(readCategories[rc]);
			if (!category)
			{
				error(TXT("category not recognised \"%S\""), categoryName.to_char());
				return false;
			}

			Entry* newEntry = new Entry();
			newEntry->category = category;
			newEntry->date = date;
			newEntry->info = info;
			newEntry->autoSum = readValues[rc];
			newEntry->values.push_back(readValues[rc]);
			_entries.push_back(newEntry);
		}

		while (true)
		{
			tchar ch = _file->get_last_read_char();
			if (ch == 13 || ch == 10)
			{
				break;
			}
			_file->read_char();
			if (!_file->has_last_read_char())
			{
				return true;
			}
		}
	}

	return true;
}

bool Entry::load_entries_from(IO::File* _file, List<Entry*> & _entries)
{
	_file->set_type(IO::InterfaceType::Text);

	bool result = true;
	Token token;
	// read first token
	read_any_token(_file, token);
	bool inSum = false;
	List<Entry*> sumFor;
	int sumValue;
	while (true)
	{
		if (!_file->has_last_read_char())
		{
			break;
		}
		// read type
		if (token == TXT("sum"))
		{
			inSum = true;
			read_any_token(_file, token);
			String value = String(token.to_char());
			value = value.replace(String(TXT("")) + (tchar)194 + (tchar)160, String(TXT("")));
			value = value.replace(String(TXT("")) + (tchar)160, String(TXT("")));
			value = value.replace(String::space(), String(TXT("")));
			value = value.replace(String(TXT(",")), String(TXT(".")));
			sumValue = (int)(ParserUtils::parse_float(value) * 100.0f);

			String info;
			tchar ch = _file->get_last_read_char();
			while (true)
			{
				if (ch == 13 || ch == 10 || !_file->has_last_read_char())
				{
					break;
				}
				if ((ch == 32 || ch == 9) &&
					info.is_empty())
				{
					// nothing
				}
				else
				{
					info += ch;
				}
				ch = _file->read_char();
			}

			// read next token
			read_any_token(_file, token);
		}
		else if (token == TXT("endsum"))
		{
			inSum = false;
			Entry* withoutValues = nullptr;
			sumValue = abs(sumValue);
			for_every_ptr(entry, sumFor)
			{
				if (entry->autoSum)
				{
					sumValue = -sumValue;
					break;
				}
			}
			for_every_ptr(entry, sumFor)
			{
				sumValue -= entry->autoSum;
				if (entry->values.is_empty())
				{
					if (withoutValues)
					{
						error(TXT("too many elements in sum without values"));
						return false;
					}
					withoutValues = entry;
				}
			}
			if (withoutValues)
			{
				withoutValues->autoSum = sumValue;
			}
			else
			{
				error(TXT("no element without values in sum"));
				return false;
			}
			sumFor.clear();
			// read next token
			read_any_token(_file, token);
		}
		else
		{
			float multiplier = 1.0f;
			if (token == TXT("cost"))
			{
				multiplier = -1.0f;
			}
			else if (token == TXT("gain"))
			{
				multiplier = 1.0f;
			}
			else
			{
				error(TXT("problem loading file, couldn't recognise \"%S\""), token.to_char());
				return false;
			}

			// read date
			Date date;
			if (!date.load_from(_file))
			{
				error(TXT("problem loading date"));
				return false;
			}

			// find category
			if (!read_any_token(_file, token))
			{
				error(TXT("problem loading category"));
				return false;
			}
			Name categoryName = Name(token.to_char());
			Category* category = Categories::find_category(categoryName);
			if (!category)
			{
				error(TXT("category not recognised \"%S\""), categoryName.to_char());
				return false;
			}

			String info;
			tchar ch = _file->get_last_read_char();
			while (true)
			{
				if (ch == 13 || ch == 10 || ! _file->has_last_read_char())
				{
					break;
				}
				if ((ch == 32 || ch == 9) &&
					info.is_empty())
				{
					// nothing
				}
				else
				{
					info += ch;
				}
				ch = _file->read_char();
			}

			Entry* newEntry = new Entry();
			newEntry->category = category;
			newEntry->date = date;
			newEntry->info = info;
			newEntry->autoSum = 0;
			while (read_any_token(_file, token))
			{
				if (token == TXT("sum") ||
					token == TXT("endsum") ||
					token == TXT("cost") ||
					token == TXT("gain"))
				{
					break;
				}
				String value = String(token.to_char());
				value = value.replace(String(TXT("")) + (tchar)194 + (tchar)160, String(TXT("")));
				value = value.replace(String(TXT("")) + (tchar)160, String(TXT("")));
				value = value.replace(String::space(), String(TXT("")));
				value = value.replace(String(TXT(",")), String(TXT(".")));
				int iVal = (int)(multiplier * ParserUtils::parse_float(value) * 100.0f);
				newEntry->values.push_back(iVal);
				newEntry->autoSum += iVal;

				// read to end of line
				tchar ch = _file->get_last_read_char();
				while (true)
				{
					if (ch == 13 || ch == 10 || !_file->has_last_read_char())
					{
						break;
					}
					ch = _file->read_char();
				}
			}
			_entries.push_back(newEntry);
			if (inSum)
			{
				sumFor.push_back(newEntry);
			}
		}
	}
	return true;
}

bool Entry::load_dir(String const & _dir, List<Entry*> & _entries)
{
	bool result = true;
	IO::Dir dir;
	if (dir.list(_dir))
	{
		Array<String> const & directories = dir.get_directories();
		for_every(directory, directories)
		{
			result &= load_dir(_dir + TXT("/") + *directory, _entries);
		}

		Array<String> const & files = dir.get_files();
		for_every(file, files)
		{
			output(TXT("  %S"), file->to_char());
			if (file->has_suffix(String(TXT(".gdocs_csv"))))
			{
				result &= load_entries_from_gdocs_csv(&IO::File(_dir + TXT("\\") + *file), _entries);
			}
			else
			{
				result &= load_entries_from(&IO::File(_dir + TXT("\\") + *file), _entries);
			}
		}
	}
	return result;
}

