#include "displayDrawContext.h"

#include "display.h"
#include "displayColourReplacer.h"

using namespace Framework;

DisplayDrawContext DisplayDrawContext::s_none;

bool DisplayDrawContext::replace_colour_pair_by_name(REF_ Name & _colourPair) const
{
	bool result = false;
	for_every_reverse_ptr(colourReplacer, colourReplacers)
	{
		result |= colourReplacer->replace_colour_pair_by_name(REF_ _colourPair);
	}
	return result;
}

bool DisplayDrawContext::replace_colour_pair(REF_ Colour & _ink, REF_ Colour & _paper) const
{
	bool result = false;
	for_every_reverse_ptr(colourReplacer, colourReplacers)
	{
		result |= colourReplacer->replace_colour_pair(REF_ _ink, REF_ _paper);
	}
	return result;
}

bool DisplayDrawContext::replace_colour(REF_ Colour & _colour) const
{
	bool result = false;
	for_every_reverse_ptr(colourReplacer, colourReplacers)
	{
		result |= colourReplacer->replace_colour(REF_ _colour);
	}
	return result;
}

void DisplayDrawContext::process_colours(Display* _display, bool _useDefaultColour, Name const & _useColourPair, REF_ Colour & _inkColour, REF_ Colour & _paperColour) const
{
	if (_useDefaultColour)
	{
		_inkColour = _display->get_setup().defaultInk;
		_paperColour = _display->get_setup().defaultPaper;
	}
	else
	{
		Name colourPair = _useColourPair;
		replace_colour_pair_by_name(REF_ colourPair);
		if (colourPair.is_valid())
		{
			if (DisplayColourPair const * cp = _display->get_colour_pair(colourPair))
			{
				_inkColour = cp->get_ink(_display);
				_paperColour = cp->get_paper(_display);
			}
		}
	}
	replace_colour_pair(REF_ _inkColour, REF_ _paperColour);
}
