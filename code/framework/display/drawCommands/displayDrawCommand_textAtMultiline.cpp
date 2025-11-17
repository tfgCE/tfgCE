#include "displayDrawCommand_textAtMultiline.h"

#include "..\display.h"

#include "..\..\video\font.h"

using namespace Framework;
using namespace DisplayDrawCommands;

TextAtMultiline::TextAtMultiline()
{
	textParamLines = 0;
}

bool TextAtMultiline::draw_onto(Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ DisplayDrawContext & _context) const
{
	if (textParamLines == 0)
	{
		return base::draw_onto(_display, _v3d, _drawCyclesUsed, _drawCyclesAvailable, _context);
	}
	bool allLinesDrawn = true;
	int drawCyclesUsed = _drawCyclesUsed;
	int drawCyclesAvailable = _drawCyclesAvailable;
	int drawCyclesUsedInPreviousLines = 0;
	CharCoords at = atFinal;
	VectorInt2 scale = get_scale();
	VectorInt2 fineOffset = VectorInt2::zero;
	if (vAlignmentParam.is_set())
	{
		CharCoord fieldHeight = (inRegion ? inRegion->get_height() :
						 (inRect.is_set() ? inRect.get().y.length() : 1));
		CharCoord fieldBottom = (inRegion ? inRegion->get_left_bottom().y :
						 (inRect.is_set() ? inRect.get().y.min : at.y));
		Vector2 charSize = calculate_char_size(_display);
		if (vAlignmentParam.get() == DisplayVAlignment::Top)
		{
			at.y = fieldBottom + fieldHeight - 1;
		}
		else if (vAlignmentParam.get() == DisplayVAlignment::Bottom)
		{
			at.y = fieldBottom + (textParamLines - 1) * scale.y;
		}
		else if (vAlignmentParam.get() == DisplayVAlignment::Centre)
		{
			at.y = fieldBottom + (fieldHeight + (textParamLines - 1) * scale.y) / 2;
		}
		else if (vAlignmentParam.get() == DisplayVAlignment::CentreBottom)
		{
			at.y = fieldBottom + (fieldHeight + (textParamLines - 1) * scale.y) / 2;
			if ((fieldHeight - textParamLines) % 2 == 1)
			{
				at.y -= 1;
			}
		}
		else if (vAlignmentParam.get() == DisplayVAlignment::CentreFine)
		{
			at.y = fieldBottom + (fieldHeight + (textParamLines - 1) * scale.y) / 2;
			if ((fieldHeight - textParamLines) % 2 == 1)
			{
				fineOffset.y -= (int)((charSize.x / 2) * (float)scale.x);
			}
		}
	}
	if (startAtLine < 0)
	{
		at.y += 1 * scale.y * startAtLine;
	}
	for (int i = 0; i < textParamLines; ++i)
	{
		int textStartsAt = textParamMultilineExt.is_empty() ? textParamMultiline[i].startsAt : textParamMultilineExt[i].startsAt;
		int thisLineLength = get_line_length(i);
		int drawCyclesRequiredForWholeText = 0;
		if (i >= startAtLine && (endAtLine < startAtLine || i <= endAtLine))
		{
			if (!thisLineLength)
			{
				at.y -= 1 * scale.y;
			}
			else
			{
				tchar lineText[MAX_LENGTH + 1];
				memory_copy(lineText, &textParam[textStartsAt], thisLineLength * sizeof(tchar));
				lineText[thisLineLength] = 0;
				if (draw_text_onto(lineText, thisLineLength, CharCoords(at.x, at.y), fineOffset, scale, _display, _v3d, drawCyclesUsed, drawCyclesAvailable, drawCyclesRequiredForWholeText, _context))
				{
					drawCyclesUsedInPreviousLines += drawCyclesRequiredForWholeText;
					drawCyclesUsed -= drawCyclesRequiredForWholeText;
					at.y -= 1 * scale.y;
				}
				else
				{
					allLinesDrawn = false;
					break;
				}
			}
		}
		textStartsAt += thisLineLength + 1;
	}

	_drawCyclesUsed = drawCyclesUsed + drawCyclesUsedInPreviousLines;
	_drawCyclesAvailable = drawCyclesAvailable;

	return allLinesDrawn;
}

void TextAtMultiline::prepare(Display* _display)
{
	base::prepare(_display);

	textParamLines = 0;
	if (limitParam.is_set() || inRegion || inRect.is_set())
	{
		int newLineChars = 0;
		{	// maybe we have new line character?
			
			tchar const * charParam = &textParam[0];
			for (int i = 0; i < textParamLength; ++i, ++charParam)
			{
				if (*charParam == '~' || *charParam == '\n')
				{
					++newLineChars;
				}
			}
		}

		int fieldLength = calculate_field_length(textParamLength, get_scale().x);
		if ((textParamLength > fieldLength || newLineChars > 0) && fieldLength > 0)
		{
			int textStartsAt = 0;
			while (textStartsAt < textParamLength)
			{
				// get length of line, assume that it is field's length. if we find space earlier, we will use that length
				int foundLength = fieldLength;
				int offsetAtEnd = 0;
				if (textParamLength - textStartsAt < foundLength)
				{
					foundLength = textParamLength - textStartsAt;
				}
				{	// maybe we have new line character first?
					tchar const * charParam = &textParam[textStartsAt];
					for (int i = 0; i < foundLength; ++i, ++charParam)
					{
						if (*charParam == '~' || *charParam == '\n')
						{
							foundLength = i;
							offsetAtEnd = 1;
							break;
						}
					}
				}
				if (foundLength >= fieldLength && textParamLength - textStartsAt > fieldLength) // we're still at field length and we have something more, we need something shorter
				{
					// start with following character, as we may decide that it is ok to stop there
					tchar const * charParam = &textParam[textStartsAt + fieldLength];
					for (int i = fieldLength; i > 0; --i, --charParam)
					{
						if (*charParam == 32 || *charParam == '~' || *charParam == '\n' || *charParam == 0)
						{
							foundLength = i;
							offsetAtEnd = 1;
							break;
						}
					}
				}
				if (textParamLines >= MAX_LINES)
				{
					textParamMultilineExt.make_space_for(MAX_LINES);
					while (textParamMultilineExt.get_size() < min(textParamLines, MAX_LINES))
					{
						textParamMultilineExt.push_back(textParamMultiline[textParamMultilineExt.get_size()]);
					}
					textParamMultilineExt.push_back(LineProp(textStartsAt, foundLength));
				}
				else
				{
					textParamMultiline[textParamLines] = LineProp(textStartsAt, foundLength);
				}
				textStartsAt += foundLength + offsetAtEnd;
				++textParamLines;
			}
		}
		else
		{
			textParamLines = 1;
			textParamMultiline[0] = LineProp(0, textParamLength);
		}
	}

	verticalSize = get_lines_count() * (int)calculate_char_size(_display).y * get_scale().y;
}

int TextAtMultiline::get_max_line_length() const
{
	int maxLength = 0;
	for_count(int, i, textParamLines)
	{
		maxLength = max(maxLength, get_line_length(i));
	}
	return maxLength;
}

int TextAtMultiline::get_line_length(int _idx) const
{
	if (!textParamMultilineExt.is_empty())
	{
		return (_idx < textParamLines ? textParamMultilineExt[_idx].length : textParamLength);
	}
	else
	{
		return (_idx < textParamLines ? textParamMultiline[_idx].length : textParamLength);
	}
}
