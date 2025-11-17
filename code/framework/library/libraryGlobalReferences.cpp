#include "libraryGlobalReferences.h"

using namespace Framework;

void LibraryGlobalReferences::clear()
{
	references.clear();
}

bool LibraryGlobalReferences::load_from_xml(IO::XML::Node const * _containerNode, LibraryLoadingContext & _lc)
{
	bool result = true;

	for_every(node, _containerNode->all_children())
	{
		Name id = node->get_name_attribute(TXT("id"));
		if (!id.is_valid())
		{
			error_loading_xml(node, TXT("no id provided for global reference"));
			result = false;
			continue;
		}

		Entry entry;
		entry.id = id;
		entry.mayFail = node->get_bool_attribute_or_from_child_presence(TXT("mayFail"), entry.mayFail);
		if (node->get_name() == TXT("tags"))
		{
			entry.tags.load_from_string(node->get_text());
			references.push_back(entry);
		}
		else if (node->has_attribute(TXT("libName")) && entry.stored.load_from_xml(node, nullptr, TXT("libName"), _lc))
		{
			bool add = true;
			for_every(ref, references)
			{
				if (ref->id == entry.id &&
					ref->stored.get_type() == entry.stored.get_type())
				{
					*ref = entry;
					add = false;
				}
			}
			if (add)
			{
				references.push_back(entry);
			}
		}
		else
		{
			error_loading_xml(node, TXT("error loading global reference \"%S\""), id.to_char());
			result = false;
		}
	}

	return result;
}

bool LibraryGlobalReferences::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(ref, references)
	{
		result &= ref->stored.prepare_for_game(_library, _pfgContext, ref->mayFail);
	}

	return result;
}

bool LibraryGlobalReferences::unload(LibraryLoadLevel::Type _libraryLoadLevel)
{
	references.clear();
	return true;
}

Tags const& LibraryGlobalReferences::get_tags(Name const& _id) const
{
	for_every(ref, references)
	{
		if (ref->id == _id)
		{
			return ref->tags;
		}
	}
	return Tags::none;
}
