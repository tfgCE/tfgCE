#pragma once

#include "..\..\core\containers\array.h"
#include "..\..\core\containers\arrayStatic.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\types\colour.h"
#include "..\..\core\types\optional.h"

namespace Framework
{
	/**
	 *	Allows to replace colours to use same commands multiple times to render different results.
	 *	Default use should be:
	 *		1.	try to replace colour pair name (or default)
	 *		2.	try to replace colour pair
	 *				or
	 *			try to replace invididual colours
	 *
	 *	Replacing colours:
	 *		use "replace colour table", if that fails, goes for dark/light colour
	 *		when replacing a colour pair, it first replaces ink and basing on result (especially whether it used dark or light), it replaces paper accordingly
	 */
	struct DisplayColourReplacer
	: public RefCountObject
	{
	public:
		static DisplayColourReplacer const & none() { return s_none; }

		bool replace_colour_pair_by_name(REF_ Name & _colourPair) const;
		bool replace_colour_pair(REF_ Colour & _ink, REF_ Colour & _paper) const;
		bool replace_colour(REF_ Colour & _colour) const;

		void set_replace_colour_pair_by_name(Name const & _replace, Name const & _with);
		void set_default_colour_pair(Name const & _defaultColourPair) { defaultColourPair = _defaultColourPair; }
		void clear_default_colour_pair() { defaultColourPair.clear(); }
		void set_replace_colour(Colour const & _replace, Colour const & _with);
		void set_dark_colour(Colour const & _darkColour) { darkColour = _darkColour; }
		void clear_dark_colour() { darkColour.clear(); }
		void set_light_colour(Colour const & _lightColour) { lightColour = _lightColour; }
		void clear_light_colour() { lightColour.clear(); }

	private:
		static DisplayColourReplacer s_none;

		template<typename Class>
		struct Replace
		{
			Class replace;
			Class with;
		};

		Array<Replace<Name>> replaceColourPair;
		Array<Replace<Colour>> replaceColour;

		Optional<Name> defaultColourPair; // if set, all default colour pairs will be replaced by this

		Optional<Colour> darkColour; // if set, all dark colours will be replaced by this one
		Optional<Colour> lightColour; // if set, all light colours will be replaced by this one
	};
}