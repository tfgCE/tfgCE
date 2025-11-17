#include "category.h"

#include "..\core\io\file.h"

using namespace Budget;

Category::Category(String const & _name, Array<String> const & _akas, Category* _belongsTo)
: name(_name)
, belongsTo(nullptr)
{
	for_every(aka, _akas)
	{
		akas.push_back(Name(*aka));
	}
	set_belongs_to(_belongsTo);
	Categories::add(this);
}

Category::~Category()
{
	while (!has.is_empty())
	{
		delete has.get_first();
	}
	set_belongs_to(nullptr);
	Categories::remove(this);
}

void Category::set_belongs_to(Category* _category)
{
	if (belongsTo)
	{
		belongsTo->has.remove(this);
	}
	belongsTo = _category;
	if (belongsTo)
	{
		belongsTo->has.push_back_unique(this);
	}
}

//

Categories* Categories::s_categories = nullptr;

void Categories::initialise_static()
{
	an_assert(s_categories == nullptr);
	s_categories = new Categories();
}

void Categories::close_static()
{
	an_assert(s_categories != nullptr);
	while (!s_categories->categories.is_empty())
	{
		delete s_categories->categories.get_first();
	}
	delete_and_clear(s_categories);
}

void Categories::build()
{
	an_assert(s_categories != nullptr);

	s_categories->map.clear();
	for_every_ptr(category, s_categories->categories)
	{
		s_categories->map[category->get_name()] = category;
		for_every(aka, category->akas)
		{
			s_categories->map[*aka] = category;
		}
	}
}

Category* Categories::find_category(Name const & _name)
{
	an_assert(s_categories != nullptr);

	if (Category* const* cptr = s_categories->map.get_existing(_name))
	{
		return *cptr;
	}
	else
	{
		return nullptr;
	}
}

void Categories::add(Category* _category)
{
	an_assert(s_categories != nullptr);

	s_categories->categories.push_back_unique(_category);
}

void Categories::remove(Category* _category)
{
	an_assert(s_categories != nullptr);

	s_categories->categories.remove(_category);
}

bool Categories::load_from(IO::File* _file)
{
	_file->set_type(IO::InterfaceType::Text);

	bool result = true;

	if (!_file->is_open())
	{
		error(TXT("no category file"));
		return false;
	}

	bool stop = false;
	Array<Category*> stack;
	while (! stop)
	{
		int indent = 0;
		tchar ch;

		while (true)
		{
			ch = _file->read_char();
			if (! _file->has_last_read_char())
			{
				stop = true;
				break;
			}
			if (ch == 9) // tab
			{
				++indent;
			}
			else if (ch == 10 || ch == 13)
			{
				// just pass it
			}
			else
			{
				break;
			}
		}
		if (stop)
		{
			break;
		}
		if (stack.get_size() < indent)
		{
			error(TXT("error loading categories"));
			return false;
		}
		stack.set_size(indent);

		Array<String> categoryNames;
		String categoryName;
		while (ch != 13 && ch != 10)
		{
			categoryName += ch;
			ch = _file->read_char();
			if (ch == '/')
			{
				ch = _file->read_char();
				categoryNames.push_back(categoryName);
				categoryName = String::empty();
			}
			if (! _file->has_last_read_char())
			{
				stop = true;
				break;
			}
		}

		if (! categoryName.is_empty())
		{
			if (categoryNames.get_size() > 0)
			{
				categoryNames.push_back(categoryName);
				categoryName = categoryNames[0];
				categoryNames.remove_at(0);
			}
			//output(TXT("  %S"), categoryName.to_char());
			if (Categories::find_category(Name(categoryName)))
			{
				error(TXT("category \"%S\" dobuled"), categoryName.to_char());
				return false;
			}
			Category* newCategory = new Category(categoryName, categoryNames, stack.get_size() > 0 ? stack.get_last() : nullptr);
			stack.push_back(newCategory);
		}
	}

	build();

	return result;
}

bool Category::does_belong_to(Category const * _category) const
{
	Category const * goUp = this;
	while (goUp)
	{
		if (goUp == _category)
		{
			return true;
		}
		goUp = goUp->belongsTo;
	}
	return false;
}

