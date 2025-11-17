#pragma once

#include "..\library\libraryStored.h"

namespace Framework
{
	class ColourPalette;

	struct RemapColours
	{
		void remap(int _from, int _to);
		void prepare_colour_map(ColourPalette const* _for, REF_ Array<int> & _map);

	private:
		struct RemapColour
		{
			int from = 0;
			int to = 0;
		};
		Array<RemapColour> remapColours;
	};
	class ColourPalette
	: public LibraryStored
	{
		FAST_CAST_DECLARE(ColourPalette);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		ColourPalette(Library * _library, LibraryName const & _name);

		Array<Colour> & access_colours() { return colours; }
		Array<Colour> const& get_colours() const { return colours; }

		Optional<VectorInt2> const& get_preferred_grid() const { return preferredGrid; }

	public:
		void clear();
		void add(Colour const& _colour);
		void add_blend(int _amount, Colour const& _a, Colour const& _b, bool _blendToLast = true); // if _blendToLast is false will assume that the next colour would be "end"
		void add_hue(int _amount, float _low, float _high); // full

	public:
		void be_commodore_64();
		void be_zx_spectrum();
		void be_atari_2600_ntsc();
		void be_standard_vga();
		void be_nice_vga(); // as standard but with a better layout/grid
		void be_full_vga();

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool save_to_xml(IO::XML::Node* _node) const;

	private:
		Array<Colour> colours;
		Optional<VectorInt2> preferredGrid;
	};
};
