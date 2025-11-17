#include "textBlockRef.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\serialisation\serialiser.h"

using namespace Framework;

//

void TextBlocks::copy_from(TextBlockRef const & _source)
{
	for_every(tb, _source.get_all())
	{
		all.push_back_unique(*tb);
	}
}

bool TextBlocks::serialise(Serialiser & _serialiser)
{
	bool result = true;

	result &= serialise_data(_serialiser, all);

	return result;
}

TextBlock* TextBlocks::get_one(Random::Generator & _rg) const
{
	if (get_all().is_empty())
	{
		return nullptr;
	}

	return get_all()[_rg.get_int(get_all().get_size())].get();
}

//

bool TextBlockRef::cache_everything_from(Array<TextBlockPtr> & _array, Library* _library, bool _ignoreMain, TextBlockRef const & _source)
{
	bool result = true;

	for_every(inc, _array)
	{
		inc->find_if_required(_library);
		if (!inc->get())
		{
			error(TXT("could not find included text block \"%S\""), inc->get_name().to_string().to_char());
			result = false;
			continue;
		}
		if (!_ignoreMain || !inc->get()->is_main())
		{
			if (!all.does_contain(*inc))
			{
				all.push_back_unique(*inc);
				cache_everything(_library, _ignoreMain, inc->get()->include);
			}
		}
	}

	return result;
}

bool TextBlockRef::cache_everything(Library* _library, bool _ignoreMain, TextBlockRef const & _source)
{
	bool result = true;

	result &= cache_everything_from(includePrivate, _library, _ignoreMain, _source);
	result &= cache_everything_from(include, _library, _ignoreMain, _source);

	for_every(tags, includeTags)
	{
		for_every(tb, _library->get_text_blocks().get_all_stored())
		{
			if (!_ignoreMain || !tb->get()->is_main())
			{
				if (tags->is_contained_within(tb->get()->get_tags()))
				{
					TextBlockPtr tbptr(tb->get());
					if (!all.does_contain(tbptr))
					{
						all.push_back_unique(tbptr);
						cache_everything(_library, _ignoreMain, tbptr->include);
					}
				}
			}
		}
	}

	return result;
}

bool TextBlockRef::cache_everything(Library* _library, bool _ignoreMain)
{
	return cache_everything(_library, _ignoreMain, *this);
}

bool TextBlockRef::cache(Library* _library, bool _ignoreMain)
{
	bool result = true;
	all.clear();

	for_every(inc, includePrivate)
	{
		inc->find_if_required(_library);
		if (inc->get())
		{
			all.push_back_unique(*inc);
		}
		else
		{
			error(TXT("could not find included text block \"%S\""), inc->get_name().to_string().to_char());
			result = false;
		}
	}
	
	for_every(inc, include)
	{
		inc->find_if_required(_library);
		if (inc->get())
		{
			all.push_back_unique(*inc);
		}
		else
		{
			error(TXT("could not find included text block \"%S\""), inc->get_name().to_string().to_char());
			result = false;
		}
		all.push_back_unique(*inc);
	}

	for_every(tags, includeTags)
	{
		for_every(tb, _library->get_text_blocks().get_all_stored())
		{
			if (tags->is_contained_within(tb->get()->get_tags()))
			{
				all.push_back_unique(TextBlockPtr(tb->get()));
			}
		}
	}

	return result;
}

void TextBlockRef::prepare_to_unload()
{
}

bool TextBlockRef::load_from_xml(IO::XML::Node const * _node, tchar const * _childrenNodeName, LibraryLoadingContext & _lc, tchar const * _id, std::function<void(TextBlock*)> _perform_on_loaded_text_block)
{
	bool result = true;

	if (!_node)
	{
		return result;
	}

	for_every(node, _node->children_named(_childrenNodeName))
	{
		result &= load_from_xml(node, _lc, _id, _perform_on_loaded_text_block);
	}

	return result;
}

bool TextBlockRef::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _id, std::function<void(TextBlock*)> _perform_on_loaded_text_block)
{
	bool result = true;

	if (!_node)
	{
		return result;
	}

	result &= TextBlock::load_private_into_array_from_xml(true, _node, _lc, _id, includePrivate, _perform_on_loaded_text_block);

	for_every(includeNode, _node->children_named(TXT("include")))
	{
		if (includeNode->has_attribute(TXT("name")))
		{
			TextBlockPtr inc;
			if (inc.load_from_xml(includeNode, TXT("name"), _lc))
			{
				include.push_back(inc);
			}
			else
			{
				error_loading_xml(includeNode, TXT("error loading include-name pair"));
				result = false;
			}
		}
		if (includeNode->has_attribute(TXT("tags")))
		{
			Tags tags;
			if (tags.load_from_xml_attribute_or_child_node(includeNode, TXT("tags")))
			{
				includeTags.push_back(tags);
			}
			else
			{
				error_loading_xml(includeNode, TXT("error loading include-tags pair"));
				result = false;
			}
		}
	}

	return result;
}
