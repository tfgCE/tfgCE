#include "displayColourReplacer.h"

using namespace Framework;

DisplayColourReplacer DisplayColourReplacer::s_none;

bool DisplayColourReplacer::replace_colour_pair_by_name(REF_ Name & _colourPair) const
{
	for_every(replace, replaceColourPair)
	{
		if (replace->replace == _colourPair)
		{
			_colourPair = replace->with;
			return true;
		}
	}
	if (defaultColourPair.is_set())
	{
		_colourPair = defaultColourPair.get();
		return true;
	}
	return false;
}

bool DisplayColourReplacer::replace_colour_pair(REF_ Colour & _ink, REF_ Colour & _paper) const
{
	// replace ink
	bool replacedInk = false;
	for_every(replace, replaceColour)
	{
		if (replace->replace == _ink)
		{
			_ink = replace->with;
			replacedInk = true;
			break;
		}
	}
	bool replacedInkWithDark = false;
	bool replacedInkWithLight = false;
	if (!replacedInk)
	{
		float average = (_ink.r + _ink.g + _ink.a) / 3.0f;
		if (average >= 0.5f)
		{
			if (lightColour.is_set())
			{
				_ink = lightColour.get();
				replacedInkWithLight = true;
				replacedInk = true;
			}
		}
		else
		{
			if (darkColour.is_set())
			{
				_ink = darkColour.get();
				replacedInkWithDark = true;
				replacedInk = true;
			}
		}
	}
	bool replacedPaper = false;
	for_every(replace, replaceColour)
	{
		if (replace->replace == _paper)
		{
			_paper = replace->with;
			replacedPaper = true;
			break;
		}
	}
	if (!replacedPaper)
	{
		float average = (_paper.r + _paper.g + _paper.a) / 3.0f;
		if (replacedInkWithDark)
		{
			average = 1.0f;
		}
		else if (replacedInkWithLight)
		{
			average = 0.0f;
		}
		if (average >= 0.5f)
		{
			if (lightColour.is_set())
			{
				_paper = lightColour.get();
				replacedPaper = true;
			}
		}
		else
		{
			if (darkColour.is_set())
			{
				_paper = darkColour.get();
				replacedPaper = true;
			}
		}
	}

	return replacedInk || replacedPaper;
}

bool DisplayColourReplacer::replace_colour(REF_ Colour & _colour) const
{
	for_every(replace, replaceColour)
	{
		if (replace->replace == _colour)
		{
			_colour = replace->with;
			return true;
		}
	}
	float average = (_colour.r + _colour.g + _colour.a) / 3.0f;
	if (average >= 0.5f)
	{
		if (lightColour.is_set())
		{
			_colour = lightColour.get();
			return true;
		}
	}
	else
	{
		if (darkColour.is_set())
		{
			_colour = darkColour.get();
			return true;
		}
	}
	return false;
}

void DisplayColourReplacer::set_replace_colour_pair_by_name(Name const & _replace, Name const & _with)
{
	for_every(replace, replaceColourPair)
	{
		if (replace->replace == _replace)
		{
			replace->with = _with;
			return;
		}
	}
	Replace<Name> newReplace;
	newReplace.replace = _replace;
	newReplace.with = _with;
	replaceColourPair.push_back(newReplace);
}

void DisplayColourReplacer::set_replace_colour(Colour const & _replace, Colour const & _with)
{
	for_every(replace, replaceColour)
	{
		if (replace->replace == _replace)
		{
			replace->with = _with;
			return;
		}
	}
	Replace<Colour> newReplace;
	newReplace.replace = _replace;
	newReplace.with = _with;
	replaceColour.push_back(newReplace);
}
