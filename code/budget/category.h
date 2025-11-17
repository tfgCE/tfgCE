#pragma once

#include "..\core\containers\map.h"
#include "..\core\types\name.h"

namespace IO
{
	class File;
};

namespace Budget
{
	struct Category;
	struct Categories;

	struct Category
	{
	public:
		Category(String const & _name, Array<String> const & _akas, Category* _belongsTo);
		~Category();

		Name const & get_name() const { return name; }
		Category* get_belongs_to() const { return belongsTo; }
		Array<Category*> const & get_has() const { return has; }

		bool does_belong_to(Category const * _category) const;

	private:
		Name name;
		Array<Name> akas;
		Category* belongsTo;
		Array<Category*> has;

	private:
		void set_belongs_to(Category* _category);

		friend struct Categories;
	};

	struct Categories
	{
	public:
		static void initialise_static();
		static void close_static();

		static void build();

		static Category* find_category(Name const & _name);

		static bool load_from(IO::File* _file);

	private: friend struct Category;
		static void add(Category* _category);
		static void remove(Category* _category);

	private:
		static Categories* s_categories;

		Array<Category*> categories;
		Map<Name, Category*> map;
	};
};
