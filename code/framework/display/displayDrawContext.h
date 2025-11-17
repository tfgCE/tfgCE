#pragma once

#include "displayColourReplacer.h"

namespace Framework
{
	class Display;

	struct DisplayDrawContext
	{
	public:
		static DisplayDrawContext none() { return s_none; }

		DisplayDrawContext() { SET_EXTRA_DEBUG_INFO(colourReplacers, TXT("DisplayDrawContext.colourReplacers")); }

		DisplayDrawContext & push_colour_replacer(DisplayColourReplacer const * _colourReplacer) { colourReplacers.push_back(_colourReplacer); return *this; }
		DisplayDrawContext & pop_colour_replacer() { an_assert(!colourReplacers.is_empty()); colourReplacers.remove_at(colourReplacers.get_size() - 1); return *this; }

	public:
		DisplayColourReplacer const & get_colour_replacer() const { return ! colourReplacers.is_empty() ? *(colourReplacers.get_last()) : DisplayColourReplacer::none(); }

		void process_colours(Display* _display, bool _useDefaultColour, Name const & _useColourPair, REF_ Colour & _inkColour, REF_ Colour & _paperColour) const;

		// uses whole stack (starts at top, goes to bottom, as bottom should be most universal)
		bool replace_colour_pair_by_name(REF_ Name & _colourPair) const;
		bool replace_colour_pair(REF_ Colour & _ink, REF_ Colour & _paper) const;
		bool replace_colour(REF_ Colour & _colour) const;

	private:
		static DisplayDrawContext s_none;

		ArrayStatic<DisplayColourReplacer const *, 16> colourReplacers;
	};
}