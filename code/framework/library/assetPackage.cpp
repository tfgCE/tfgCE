#include "assetPackage.h"

#include "..\library\library.h"

#include "..\editor\editors\editorBasicInfo.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(AssetPackage);
LIBRARY_STORED_DEFINE_TYPE(AssetPackage, assetPackage);

AssetPackage::AssetPackage(Library* _library, LibraryName const& _name)
: base(_library, _name)
{
}

AssetPackage::~AssetPackage()
{
}

bool AssetPackage::save_to_xml(IO::XML::Node * _node) const
{
	bool result = base::save_to_xml(_node);

	if (noIndices) _node->add_node(TXT("noIndices"));
	if (keepEntries) _node->add_node(TXT("keepEntries"));

	if (auto* node = _node->add_node(TXT("assets")))
	{
		node->add_comment(TXT("<type id=\"id\" index=\"0\" name=\"group:name\"/>"));
		for_every(e, entries)
		{
			// don't use libraryStored's save_to_xml_child_node as we want to keep id+index+name order
			if (auto* enode = node->add_node(e->libraryStored.get_type().to_char()))
			{
				enode->set_attribute(TXT("id"), e->id.to_char());
				if (!noIndices)
				{
					enode->set_int_attribute(TXT("index"), e->idx);
				}
				enode->set_attribute(TXT("name"), e->libraryStored.get_name().to_string());
				if (e->isDefault)
				{
					enode->set_bool_attribute(TXT("default"), true);
				}
			}
		}
	}

	return result;
}


bool AssetPackage::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	noIndices = false;
	keepEntries = false;

	noIndices = _node->get_bool_attribute_or_from_child_presence(TXT("noIndices"), noIndices);
	noIndices = _node->get_bool_attribute_or_from_child_presence(TXT("ignoreIndices"), noIndices);
	keepEntries = _node->get_bool_attribute_or_from_child_presence(TXT("keepEntries"), keepEntries);

	for_every(n, _node->children_named(TXT("assets")))
	{
		noIndices = n->get_bool_attribute(TXT("noIndices"), noIndices);
		noIndices = n->get_bool_attribute(TXT("ignoreIndices"), noIndices);
		keepEntries = n->get_bool_attribute(TXT("keepEntries"), keepEntries);
		if (! keepEntries)
		{
			entries.clear();
		}
		for_every(node, n->all_children())
		{
			Entry entry;

			entry.isDefault = node->get_bool_attribute_or_from_child_presence(TXT("default"));
			if (!entry.libraryStored.load_from_xml(node, nullptr, TXT("name"), _lc))
			{
				result = false;
				continue;
			}

			if (node->get_bool_attribute_or_from_child_presence(TXT("clearAllIndicesFirst")))
			{
				for (int i = 0; i < entries.get_size(); ++i)
				{
					auto& e = entries[i];
					if (e.id == entry.id &&
						e.libraryStored.get_type() == entry.libraryStored.get_type())
					{
						entries.remove_at(i);
						--i;
					}
				}
			}

			entry.id = node->get_name_attribute(TXT("id"), entry.id);
			entry.idx = node->get_int_attribute(TXT("idx"), entry.idx);
			entry.idx = node->get_int_attribute(TXT("index"), entry.idx);
			if (node->get_bool_attribute(TXT("nextIndex")))
			{
				entry.idx = 0;
				for_every(e, entries)
				{
					if (e->id == entry.id &&
						e->libraryStored.get_type() == entry.libraryStored.get_type())
					{
						entry.idx = max(entry.idx, e->idx + 1);
					}
				}
			}
			error_loading_xml_on_assert(entry.idx >= 0, result, node, TXT("index has to be a non-negative number"));

			bool alreadyExists = false;
			if (!noIndices)
			{
				for_every(e, entries)
				{
					if (e->id == entry.id &&
						e->libraryStored.get_type() == entry.libraryStored.get_type() &&
						e->idx == entry.idx)
					{
						*e = entry;
						alreadyExists = true;
						break;
					}
				}
			}

			if (!alreadyExists)
			{
				entries.push_back(entry);
			}
		}
	}

	if (! entries.is_empty())
	{
		bool hasDefault = false;
		for_every(e, entries)
		{
			if (e->isDefault)
			{
				hasDefault = true;
				break;
			}
		}
		if (!hasDefault)
		{
			entries[0].isDefault = true;
		}
	}

	return result;
}

bool AssetPackage::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		// cache "count"
		{
			struct Processed
			{
				Name id;
				Name type;

				Processed() {}
				Processed(Name const& _id, Name const & _type) : id(_id), type(_type) {}
				bool operator==(Processed const& _other) const { return id == _other.id && type == _other.type; }
			};
			Array<Processed> processedIds;
			for (int eIdx = 0; eIdx < entries.get_size(); ++eIdx)
			{
				auto& e = entries[eIdx];
				Processed pe(e.id, e.libraryStored.get_type());
				if (processedIds.does_contain(pe))
				{
					// skip
					continue;
				}

				// check for missing indices and add all between
				// also, check for min and max indices
				int idx = 0;
				int minIdx = 100000;
				int maxIdx = 0;
				auto last = entries[eIdx];
				while (true)
				{
					bool foundIdx = false;
					for_every(me, entries)
					{
						if (me->id == pe.id &&
							me->libraryStored.get_type() == pe.type)
						{
							minIdx = min(me->idx, minIdx);
							maxIdx = max(me->idx, maxIdx);
							if (me->idx == idx)
							{
								last = *me;
								foundIdx = true;
							}
						}
					}
					if (minIdx > 0)
					{
						error(TXT("entries should start at 0 for \"%S\""), get_name().to_string().to_char());
						return false;
					}
					if (!foundIdx && last.idx == idx - 1 && idx < maxIdx)
					{
						entries.push_back(last);
						++entries.get_last().idx;
						last = entries.get_last();
					}
					++idx;
					if (idx > maxIdx)
					{
						break;
					}
				}

				// fill count
				for_every(me, entries)
				{
					if (me->id == pe.id &&
						me->libraryStored.get_type() == pe.type)
					{
						me->count = maxIdx + 1;
					}
				}

				processedIds.push_back(pe);
			}
		}
	}

	for_every(e, entries)
	{
		result &= e->libraryStored.prepare_for_game(_library, _pfgContext);
	}

	return result;
}

void AssetPackage::prepare_to_unload()
{
	base::prepare_to_unload();

	entries.clear();
}

void AssetPackage::copy_from_for_temporary_copy(LibraryStored const * _source)
{
	base::copy_from_for_temporary_copy(_source);

	if (auto* src = fast_cast<AssetPackage>(_source))
	{
		entries = src->entries;
	}
}

void AssetPackage::create_temporary_copies_of_type(Name const& libType)
{
	struct Processed
	{
		LibraryStored* source;
		LibraryStored* copy;
		Processed() {}
		Processed(LibraryStored* _source, LibraryStored* _copy) : source(_source), copy(_copy) {}
	};
	Array<Processed> processed;

	for_every(e, entries)
	{
		if (libType.is_valid() && libType != e->libraryStored.get_type())
		{
			continue;
		}
		if (LibraryStored* src = e->libraryStored.get())
		{
			LibraryStored* nls = nullptr;
			for_every(p, processed)
			{
				if (src == p->source)
				{
					nls = p->copy;
					break;
				}
			}
			if (!nls)
			{
				nls = src->create_temporary_copy();
				nls->set_name(LibraryName(src->get_name().to_string() + TXT(" [ap copy]"))); // change name first before storing
				processed.push_back(Processed(src, nls));
			}
			e->libraryStored.set(nls);
		}
	}
}

void AssetPackage::create_temporary_copies()
{
	create_temporary_copies_of_type(Name::invalid());
}

void AssetPackage::do_for_all(std::function<void(LibraryStored* _stored, Name const& _id, int _idx)> _do)
{
	for_every(e, entries)
	{
		if (auto* ls = e->libraryStored.get())
		{
			_do(ls, e->id, e->idx);
		}
	}
}

void AssetPackage::do_for_all_unique_of_type(Name const & libType, std::function<void(LibraryStored* _stored, Name const& _id, int _idx)> _do)
{
	Array<LibraryStored*> processed;
	for_every(e, entries)
	{
		if (libType.is_valid() && libType != e->libraryStored.get_type())
		{
			continue;
		}
		if (auto* ls = e->libraryStored.get())
		{
			if (processed.does_contain(ls))
			{
				continue;
			}
			_do(ls, e->id, e->idx);
			processed.push_back(ls);
		}
	}
}

void AssetPackage::do_for_all_unique(std::function<void(LibraryStored* _stored, Name const& _id, int _idx)> _do)
{
	do_for_all_unique_of_type(Name::invalid(), _do);
}

Editor::Base* AssetPackage::create_editor(Editor::Base* _current) const
{
	return new Editor::BasicInfo();
}
