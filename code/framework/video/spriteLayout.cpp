#include "spriteLayout.h"

#include "sprite.h"

#include "..\editor\editors\editorSpriteLayout.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define CONTENT_ACCESS SpriteLayoutContentAccess
//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(SpriteLayout);
LIBRARY_STORED_DEFINE_TYPE(SpriteLayout, spriteLayout);

SpriteLayout::SpriteLayout(Library* _library, LibraryName const& _name)
: base(_library, _name)
{
}

SpriteLayout::~SpriteLayout()
{
}

bool SpriteLayout::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("entry")))
	{
		Entry e;
		e.sprite.load_from_xml(node, TXT("sprite"), _lc);
		e.tags.load_from_xml_attribute_or_child_node(node, TXT("tags"));
		e.at.load_from_xml_child_node(node, TXT("at"));
		entries.push_back(e);
	}

	return result;
}

bool SpriteLayout::save_to_xml(IO::XML::Node * _node) const
{
	bool result = base::save_to_xml(_node);

	for_every(entry, entries)
	{
		auto* node = _node->add_node(TXT("entry"));
		entry->sprite.save_to_xml(node, TXT("sprite"));
		node->set_attribute(TXT("tags"), entry->tags.to_string_all());
		entry->at.save_to_xml_child_node(node, TXT("at"));
	}

	return result;
}

bool SpriteLayout::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(entry, entries)
	{
		result &= entry->sprite.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	return result;
}

void SpriteLayout::prepare_to_unload()
{
	base::prepare_to_unload();

	entries.clear();
}

void SpriteLayout::debug_draw() const
{
	CONTENT_ACCESS::RenderContext context;
	CONTENT_ACCESS::render(this, context);
}

bool SpriteLayout::editor_render() const
{
	debug_draw();
	return true;
}

Editor::Base* SpriteLayout::create_editor(Editor::Base* _current) const
{
	if (fast_cast<Editor::SpriteLayout>(_current))
	{
		return _current;
	}
	else
	{
		return new Editor::SpriteLayout();
	}
}

RangeInt2 SpriteLayout::calculate_range() const
{
	RangeInt2 range = RangeInt2::empty;
	for_every(e, entries)
	{
		if (auto* s = e->sprite.get())
		{
			RangeInt2 re = SpriteContentAccess::get_combined_range(s);
			re.offset(e->at);
			range.include(re);
		}
	}
	return range;
}

//--

#define ACCESS(what) auto& what = _this->what;
#define GET(what) auto const & what = _this->what;

void SpriteLayoutContentAccess::render(SpriteLayout const * _this, RenderContext const& _context)
{
	ACCESS(entries);
	ACCESS(editedEntryIdx);

	for_every(entry, entries)
	{
		if (auto* s = entry->sprite.get())
		{
			SpriteContentAccess::RenderContext rc;
			rc.pixelOffset = entry->at;
			rc.ignoreZeroOffset = _context.ignoreZeroOffset;
			SpriteContentAccess::render(s, rc);
		}
	}

	if (entries.is_index_valid(editedEntryIdx))
	{
		auto* entry = &entries[editedEntryIdx];
		if (auto* s = entry->sprite.get())
		{
			SpriteContentAccess::RenderContext rc;
			rc.pixelOffset = entry->at;
			rc.wholeColour = _context.editedEntryColour;
			rc.ignoreZeroOffset = _context.ignoreZeroOffset;
			SpriteContentAccess::render(s, rc);
		}
	}
}

SpriteLayout::Entry* SpriteLayoutContentAccess::access_edited_entry(SpriteLayout * _this)
{
	ACCESS(entries);
	ACCESS(editedEntryIdx);

	if (entries.is_index_valid(editedEntryIdx))
	{
		return &entries[editedEntryIdx];
	}
	return nullptr;
}

int SpriteLayoutContentAccess::get_edited_entry_index(SpriteLayout const* _this)
{
	return _this->editedEntryIdx;
}

void SpriteLayoutContentAccess::set_edited_entry_index(SpriteLayout* _this, int _index)
{
	ACCESS(entries);
	ACCESS(editedEntryIdx);

	editedEntryIdx = clamp(_index, 0, entries.get_size() - 1);
}

void SpriteLayoutContentAccess::move_edited_entry(SpriteLayout* _this, int _by)
{
	ACCESS(entries);
	ACCESS(editedEntryIdx);

	while (_by < 0 && editedEntryIdx > 0)
	{
		swap(entries[editedEntryIdx - 1], entries[editedEntryIdx]);
		--editedEntryIdx;
		++_by;
	}
	int maxEntry = entries.get_size() - 1;
	while (_by > 0 && editedEntryIdx < maxEntry)
	{
		swap(entries[editedEntryIdx + 1], entries[editedEntryIdx]);
		++editedEntryIdx;
		--_by;
	}
}

void SpriteLayoutContentAccess::remove_edited_entry(SpriteLayout* _this)
{
	ACCESS(entries);
	ACCESS(editedEntryIdx);

	entries.remove_at(editedEntryIdx);
	if (editedEntryIdx >= entries.get_size())
	{
		editedEntryIdx = entries.get_size() - 1;
	}
}

void SpriteLayoutContentAccess::add_entry(SpriteLayout* _this, Sprite* _sprite, int _offset)
{
	ACCESS(entries);
	ACCESS(editedEntryIdx);

	int at = editedEntryIdx + _offset;
	at = clamp(at, 0, entries.get_size());
	SpriteLayout::Entry* e = nullptr;
	if (entries.is_index_valid(at))
	{
		entries.insert_at(at, SpriteLayout::Entry());
		e = &entries[at];
	}
	else
	{
		entries.push_back();
		e = &entries.get_last();
		at = entries.get_size() - 1;
	}

	e->sprite = _sprite;

	editedEntryIdx = at;
}

void SpriteLayoutContentAccess::duplicate_edited_entry(SpriteLayout* _this, int _offset)
{
	ACCESS(entries);
	ACCESS(editedEntryIdx);

	if (entries.is_index_valid(editedEntryIdx))
	{
		int srcIdx = editedEntryIdx;
		add_entry(_this, entries[srcIdx].sprite.get(), _offset);
		entries[editedEntryIdx] = entries[srcIdx]; // copy everything
	}
}
