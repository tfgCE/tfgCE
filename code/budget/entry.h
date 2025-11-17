#pragma once

#include "date.h"

#include "..\core\containers\array.h"
#include "..\core\types\name.h"
#include "..\core\types\string.h"

namespace Budget
{
	struct Category;

	struct Entry
	{
	public:
		static bool load_entries_from_gdocs_csv(IO::File* _file, List<Entry*> & _entries);
		static bool load_entries_from(IO::File* _file, List<Entry*> & _entries);
		static bool load_dir(String const & _dir, List<Entry*> & _entries);

		Date get_date() const { return date; }
		Category* get_category() const { return category; }
		int get_value() const { return autoSum; }
		String const & get_info() const { return info; }

	private:
		Date date;
		Category* category;
		String info;
		Array<int> values; // x100
		int autoSum;
	};
};
