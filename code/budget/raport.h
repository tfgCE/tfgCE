#pragma once

#include "date.h"

#include "..\core\containers\array.h"
#include "..\core\types\optional.h"
#include "..\core\types\name.h"
#include "..\core\types\string.h"

namespace Budget
{
	struct Category;
	struct Entry;

	struct Raport
	{
	public:
		static bool load_raport_from(IO::File* _file, List<Raport*> & _raports);
		static bool load_dir(String const & _dir, List<Raport*> & _raports);

		void process(String const & _outputDir, List<Entry*> const & _entries);

	private:
		enum RaportPeriod
		{
			Year,
			Month,
		};
		String name;
		RaportPeriod period;
		bool inColumns;
		bool expandCategories;
		Optional<int> expandCategoriesLevel;
		bool reverseDate;
		bool costs;
		Array<Category*> categories;

		Raport();
	};
};
