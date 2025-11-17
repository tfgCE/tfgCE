#pragma once

#include "..\library\libraryStored.h"
#include "..\library\usedLibraryStored.h"

namespace Framework
{
	class Sprite;
	
	struct SpriteLayoutContentAccess;

	class SpriteLayout
	: public LibraryStored
	{
		FAST_CAST_DECLARE(SpriteLayout);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		SpriteLayout(Library* _library, LibraryName const& _name);
		virtual ~SpriteLayout();

	public:
		struct Entry
		{
			UsedLibraryStored<Sprite> sprite;
			Tags tags;
			VectorInt2 at = VectorInt2::zero;
		};

		Array<Entry> const& get_entries() const { return entries; }

		RangeInt2 calculate_range() const;

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		override_ bool save_to_xml(IO::XML::Node* _node) const; // this method will save to an existing node
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ void prepare_to_unload();

		override_ void debug_draw() const;
		override_ bool editor_render() const;

		override_ Editor::Base* create_editor(Editor::Base* _current) const;

	private:
		// when loading doubled asset, it will load over the existing one
		// if we want to replace something, just use the same id
		// but if we want to get rid off of all the existing indices, we should use clearAllIndicesFirst="true"
		Array<Entry> entries;

		int editedEntryIdx = NONE;

		friend struct SpriteLayoutContentAccess;
	};

	struct SpriteLayoutContentAccess
	{
		static SpriteLayout::Entry* access_entry(SpriteLayout* _this, int _idx) { return _this->entries.is_index_valid(_idx) ? &_this->entries[_idx] : nullptr; }
		static SpriteLayout::Entry* access_edited_entry(SpriteLayout * _this);
		static int get_edited_entry_index(SpriteLayout const* _this);
		static void set_edited_entry_index(SpriteLayout* _this, int _index);

		static void move_edited_entry(SpriteLayout* _this, int _by);
		static void remove_edited_entry(SpriteLayout* _this);
		static void add_entry(SpriteLayout* _this, Sprite* _sprite, int _offset = 1);
		static void duplicate_edited_entry(SpriteLayout* _this, int _offset = 1);

		struct RenderContext
		{
			Optional<Colour> editedEntryColour;
			bool ignoreZeroOffset = false;
		};
		static void render(SpriteLayout const * _this, RenderContext const& _context);

	};
};
