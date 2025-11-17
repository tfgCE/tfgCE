#pragma once

#include "..\library\libraryStored.h"
#include "..\..\core\memory\refCountObject.h"



namespace System
{
	class Video3D;
	struct ShaderProgramInstance;
	struct VertexFormat;
};

namespace Framework
{
	class BasicDrawRenderingBuffer;

	class Sprite;
	class SpriteGrid;

	namespace Editor
	{
		class SpriteGrid;
	};

	struct SpriteGridContext
	{
		RangeInt2 at = RangeInt2::empty;

		VectorInt2 leftBottomCorner = VectorInt2::zero; // this is on map in pixels
	};

	struct SpriteGridContent
	{
	public:
		SpriteGridContent(SpriteGrid * _forGrid);
		~SpriteGridContent();

	public:
		RangeInt2 const& get_range() const { return range; }

		void reset(); // empties whole grid
		void clear();
		void prune();
		int& access_at(VectorInt2 const& _at); // will make it larger if required
		void clear_at(VectorInt2 const& _at); // won't make it larger
		int get_at(VectorInt2 const& _at) const; // won't make it larger, will return NONE if out of bounds

		void offset_placement(VectorInt2 const& _by); // just range change

		void get_all(int _idx, OUT_ Array<VectorInt2>& _at, bool _remove = false);

	public:
		bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		bool save_to_xml(IO::XML::Node* _node) const; // this method will save to an existing node

	public:
		void editor_render() const;

	private:
		SpriteGrid* grid = nullptr;

		RangeInt2 range = RangeInt2::empty;
		Array<int> content; // from bottom left corner

	private:
		void copy_intersecting_data_from(RangeInt2 const& _otherRange, Array<int> const& _otherContent); // copies only intersecting part, if you want to include whole thing, make space first

		friend struct SpriteGridPalette;
		friend class SpriteGrid;
	};

	/**
	 *	Sprite grid palette provides an interface to what grid is composed of.
	 *	Changing its content will also modify the grid.
	 */
	struct SpriteGridPalette
	{
		struct Entry
		{
		public:
			void clear() { sprite.clear(); }
			void set(Sprite const * _sprite) { sprite = _sprite; cache(); }
			Sprite const* get() const { return sprite.get(); }

			void cache();

		public:
			bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
			bool save_to_xml(IO::XML::Node* _node) const; // this method will save to an existing node

			bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		private:
			UsedLibraryStored<Sprite> sprite;
		};

		SpriteGridPalette(SpriteGrid* _forGrid);

		int get_count() const { return sprites.get_size(); }
		Sprite const * get(int _idx) const;
		Entry const * get_entry(int _idx) const;
		int find_index(Sprite const * _s) const;
		void set(int _idx, Sprite const * _sprite);
		void clear_at_and_in_grid(int _idx); // will remove grid content of this index. use it to remove something that will be used differently
		int add(Sprite const * _sprite); // if exists, will use existing
		void swap(int _aIdx, int _bIdx);

		void prune(); // remove empty indices

		void clear_and_clear_grid(); // will also clear the grid

		void reset(); // just removes sprites from here

		void remove_unused();
		void sort(); // used firsts, sorted alphabeticaly, then one empty and the rest

		void fill_with(SpriteGridPalette const& _other);

		void cache();

	public:
		bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		bool save_to_xml(IO::XML::Node* _node) const; // this method will save to an existing node

		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	private:
		SpriteGrid* grid = nullptr;

		Array<Entry> sprites;

		friend struct SpriteGridContent;
		friend class SpriteGrid;
	};

	/**
	 *	Sprite grid is just a grid composed out of sprites
	 *	No layers etc. It is made to be editable in editor and in runtime
	 */
	class SpriteGrid
	: public LibraryStored
	{
		FAST_CAST_DECLARE(SpriteGrid);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		SpriteGrid(Library * _library, LibraryName const & _name);
		virtual ~SpriteGrid();

	public:
		void add_to(BasicDrawRenderingBuffer & _renderingBuffer, VectorInt2 const& _zeroAt, RangeInt2 const & _clipTo = RangeInt2::empty) const; // if clip to is empty, adds everything

	public:
		VectorInt2 const& get_sprite_size() const { return spriteSize; }
		void set_sprite_size(VectorInt2 const& _spriteSize) { spriteSize = _spriteSize; }

		SpriteGridPalette & access_palette() { return palette; }
		SpriteGridPalette const& get_palette() const { return palette; }

		SpriteGridContent & access_content() { return content; }
		SpriteGridContent const& get_content() const { return content; }

	public:
		Sprite const* get_at(VectorInt2 const& _at) const;

	public:
		int pixel_to_grid_x(int const & _pixel) const;
		int pixel_to_grid_y(int const & _pixel) const;
		VectorInt2 pixel_to_grid(VectorInt2 const & _pixel) const;
		VectorInt2 grid_to_pixel(VectorInt2 const & _gridCoord) const; // just left bottom
		RangeInt2 grid_to_pixel_range(VectorInt2 const & _gridCoord) const; // of a sprite / grid node
		RangeInt2 grid_to_pixel_range(RangeInt2 const & _gridRange) const; // of a range of sprites
		RangeInt2 pixel_to_grid_range(RangeInt2 const & _pixelRange) const;

	public: // LibraryStored
#ifdef AN_DEVELOPMENT
		override_ void ready_for_reload();
#endif
		override_ void prepare_to_unload();

		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		override_ bool save_to_xml(IO::XML::Node* _node) const; // this method will save to an existing node

		override_ void debug_draw() const;

		override_ bool editor_render() const;

		override_ Editor::Base* create_editor(Editor::Base* _current) const;

	protected:
		VectorInt2 spriteSize = VectorInt2(8, 8);
		SpriteGridPalette palette;
		SpriteGridContent content;

		friend struct SpriteGridPalette;
		friend struct SpriteGridContent;
	};
};
