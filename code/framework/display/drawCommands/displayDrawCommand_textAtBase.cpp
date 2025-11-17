#include "displayDrawCommand_textAtBase.h"

#include "..\display.h"

#include "..\..\pipelines\displayPipeline.h"

#include "..\..\video\font.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace DisplayDrawCommands;

//

TextAtBase::TextAtBase()
: inRegion(nullptr)
{
	textStoredParam[0] = 0;
	textParamLength = 0;
	textParam = nullptr;
	textExtParam = nullptr;
}

TextAtBase* TextAtBase::in_whole_display(Display* _display)
{
	return in_region(Framework::RangeCharCoord2(RangeInt(0, _display->get_char_resolution().x - 1),
												RangeInt(0, _display->get_char_resolution().y - 1)));
}

TextAtBase* TextAtBase::text(tchar const * _text)
{
	int length = min<int>((int)tstrlen(_text), MAX_LENGTH);
	an_assert(length == (int)tstrlen(_text), TXT("this text is too long, consider using text_ext"));
	memory_copy(textStoredParam, _text, (uint32)length * sizeof(tchar));
	textStoredParam[length] = 0;
	textParamLength = length;
	textParam = textStoredParam;
	return this;
}

TextAtBase* TextAtBase::text_ext(tchar const * _text)
{
	textExtParam = _text;
	textParamLength = (int)tstrlen(_text);
	textParam = textExtParam;
	return this;
}

bool TextAtBase::draw_onto(Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ DisplayDrawContext & _context) const
{
	int tempDrawCyclesRequiredForWholeText;
	return draw_text_onto(textParam ? textParam : TXT(""), textParam ? textParamLength : 0,
		atFinal,
		VectorInt2::zero,
		get_scale(),
		_display, _v3d, _drawCyclesUsed, _drawCyclesAvailable, tempDrawCyclesRequiredForWholeText, _context);
}

CharCoords const & TextAtBase::get_scale() const
{
	if (scaleParam.is_set())
	{
		return scaleParam.get();
	}
	else if (inRegion)
	{
		return inRegion->scale;
	}
	else
	{
		return CharCoords::one;
	}
}

CharCoord TextAtBase::calculate_field_length(int _textLength, int _scaleX) const
{
	return limitParam.is_set() ? limitParam.get() * _scaleX :
		(inRegion ? inRegion->get_length() :
		(inRect.is_set() ? inRect.get().x.length() : _textLength * _scaleX));
}

bool TextAtBase::draw_text_onto(tchar const * _text, int _textLength, CharCoords const & _at, VectorInt2 const & _fineOffset, VectorInt2 const & _scale, Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ int & _drawCyclesRequiredForWholeText, REF_ DisplayDrawContext & _context) const
{
	bool result = false;
	
	int const charCost = 5;
	CharCoord fieldLength = calculate_field_length(_textLength, _scale.x);
	_drawCyclesRequiredForWholeText = charCost * fieldLength;

	if (Font* font = (useFont? useFont : _display->get_font(alphabetParam)))
	{
		todo_note(TXT("get fallback colours from display"));
		Colour useBackgroundColour = paperParam.is_set() ? paperParam.get() : _display->get_current_paper();
		Colour useColour = inkParam.is_set() ? inkParam.get() : _display->get_current_ink();

		_context.process_colours(_display, useDefaultColourPairParam.is_set() && useDefaultColourPairParam.get(), useColourPair, REF_ useColour, REF_ useBackgroundColour);

		CharCoords printAt = _at;

		Vector2 charSize = calculate_char_size(_display);
		Vector2 loc;
		if (get_use_coordinates(_display) == DisplayCoordinates::Char)
		{
			loc = printAt.to_vector2() * charSize + _display->get_left_bottom_of_screen();
		}
		else
		{
			loc = printAt.to_vector2() + _display->get_left_bottom_of_screen();
		}

		if (pixelOffsetParam.is_set())
		{
			loc += pixelOffsetParam.get().to_vector2();
		}
		
		loc += _fineOffset.to_vector2();

		if (rotatedParam == 1)
		{
			an_assert(!inRegion);

			Matrix44 matFrom = Matrix44::identity;
			matFrom.set_translation(_display->get_left_bottom_of_screen().to_vector3());

			Matrix44 matTo = Matrix44::identity;
			matTo.set_x_axis(-Vector3::yAxis);
			matTo.set_y_axis(Vector3::xAxis);
			matTo.set_translation(_display->get_left_bottom_of_screen().to_vector3() + Vector3(0.0, -charSize.y + 1, 0.0f));

			loc = matTo.location_to_world(matFrom.location_to_local(loc.to_vector3())).to_vector2();
		}

		{
			int allowedTextLength = fieldLength / _scale.x;
			float fieldOffsetDueToScale = 0.0f;
			// calculate in chars how much did we use and when we'll land next
			int charsUsed = _drawCyclesUsed / charCost;
			if (charsUsed >= allowedTextLength)
			{
				// already done
				return true;
			}
			int charsNext = min(allowedTextLength, charsUsed + _drawCyclesAvailable / charCost);
			int charsOffset = 0;
			Vector2 fineOffset = Vector2::zero;
			if (hAlignmentParam.is_set())
			{
				if (hAlignmentParam.get() == DisplayHAlignment::Right)
				{
					charsOffset = -max(0, allowedTextLength - _textLength);
					fieldOffsetDueToScale = (float)((fieldLength - _textLength * _scale.x) % _scale.x) / (float)(_scale.x);
				}
				else if (hAlignmentParam.get() == DisplayHAlignment::Centre)
				{
					charsOffset = -max(0, (allowedTextLength - _textLength) / 2);
					fieldOffsetDueToScale = (float)(((fieldLength - _textLength * _scale.x) / 2) % _scale.x) / (float)(_scale.x);
				}
				else if (hAlignmentParam.get() == DisplayHAlignment::CentreRight)
				{
					charsOffset = -max(0, (allowedTextLength - _textLength) / 2);
					fieldOffsetDueToScale = (float)(((fieldLength - _textLength * _scale.x) / 2) % _scale.x) / (float)(_scale.x);
					if ((allowedTextLength - _textLength) % 2 == 1)
					{
						charsOffset -= 1;
					}
					an_assert(_scale.x == 1.0f, TXT("implement_ for scale!"));
				}
				else if (hAlignmentParam.get() == DisplayHAlignment::CentreFine)
				{
					charsOffset = -max(0, (allowedTextLength - _textLength) / 2);
					fieldOffsetDueToScale = (float)(((fieldLength - _textLength * _scale.x) / 2) % _scale.x) / (float)(_scale.x);
					if ((allowedTextLength - _textLength) % 2 == 1)
					{
						fineOffset.x += (charSize.x / 2) * (float)_scale.x;
					}
				}
			}

			int charsUsedWithOffset = charsUsed + charsOffset;
			int charsNextWithOffset = charsNext + charsOffset;
			int charsUsedWithOffsetClamped = max(charsUsedWithOffset, 0);
			int charsNextWithOffsetClamped = min(charsNextWithOffset, _textLength);

			float left = loc.x + charSize.x * _scale.x * (float)charsUsed;
			float right = loc.x + charSize.x * _scale.x * (float)charsNext - 1.0f;
			float top = loc.y + charSize.y * _scale.y - 1.0f;
			float bottom = loc.y;

			if ((!inRegion && !inRect.is_set()) ||
				(inRegion && printAt.y >= inRegion->get_left_bottom().y && printAt.y <= inRegion->get_top_right().y) ||
				(inRect.is_set() && printAt.y >= inRect.get().y.min && printAt.y <= inRect.get().y.max))
			{
				if (rotatedParam == 1)
				{
					Matrix44 matFrom = Matrix44::identity;
					matFrom.set_translation(_display->get_left_bottom_of_screen().to_vector3());

					Matrix44 matTo = Matrix44::identity;
					matTo.set_x_axis(-Vector3::yAxis);
					matTo.set_y_axis(Vector3::xAxis);
					matTo.set_translation(_display->get_left_bottom_of_screen().to_vector3());

					_v3d->access_model_view_matrix_stack().push_set(matFrom.to_world(matTo.inverted()));
				}

				// clear background for text
				if (!backgroundParam.is_set() || backgroundParam.get())
				{
					if (compactParam.is_set() && compactParam.get())
					{
						float compactLeft = loc.x + charSize.x * _scale.x * (float)(charsUsedWithOffsetClamped - charsOffset);
						float compactRight = compactLeft + charSize.x * _scale.x * (float)(charsNextWithOffsetClamped - charsUsedWithOffsetClamped) - 1.0f;
						if (compactRight > compactLeft)
						{
							::System::Video3DPrimitives::fill_rect_2d(useBackgroundColour, Vector2(compactLeft, bottom) + get_offset(), Vector2(compactRight, top) + get_offset(), false);
						}
					}
					else
					{
						::System::Video3DPrimitives::fill_rect_2d(useBackgroundColour, Vector2(left, bottom) + get_offset(), Vector2(right, top) + get_offset(), false);
					}
				}

				// draw only selected part of text
				if (charsNextWithOffsetClamped >= 0 && charsUsedWithOffsetClamped < _textLength)
				{
					tchar textNow[MAX_LENGTH + 1];
					int textNowLength = charsNextWithOffsetClamped - charsUsedWithOffsetClamped;
					memory_copy(textNow, &_text[charsUsedWithOffsetClamped], (textNowLength) * sizeof(tchar));
					textNow[textNowLength] = 0;
					::System::FontDrawingContext fdc;
					fdc.shaderProgramInstance = DisplayPipeline::get_setup_shader_program_instance(::System::texture_id_none(), get_use_colourise(_display), get_use_colourise_ink(_display), get_use_colourise_paper(_display));
					font->draw_text_at(_v3d, textNow, get_use_colourise(_display)? Colour::full : useColour, Vector2(loc.x + charSize.x * (float)(charsUsedWithOffsetClamped - charsOffset + fieldOffsetDueToScale) * _scale.x, bottom) + fineOffset + get_offset(), _scale.to_vector2(), Vector2::zero, NP, fdc);
				}

				if (rotatedParam == 1)
				{
					_v3d->access_model_view_matrix_stack().pop();
				}
			}

			// update draw cycles
			int nextDrawCyclesUsed = charsNext * charCost;
			_drawCyclesAvailable -= nextDrawCyclesUsed - _drawCyclesUsed;
			_drawCyclesUsed = nextDrawCyclesUsed;

			result = charsNext == allowedTextLength;
		}
	}

	return result;
}

void TextAtBase::prepare(Display* _display)
{
	base::prepare(_display);

	atFinal = atParam.is_set() ? atParam.get() : CharCoords::zero;

	if (inRegionName.is_set())
	{
		inRegion = _display->get_region(inRegionName.get());
	}

	if (inRegion)
	{
		if (atParam.is_set())
		{
			atFinal = atParam.get() + inRegion->get_left_bottom();
		}
		else
		{
			atFinal = inRegion->get_left_bottom();
		}
		if (fromTopLeftParam.is_set())
		{
			atFinal = CharCoords(inRegion->get_left_bottom().x, inRegion->get_top_right().y) + fromTopLeftParam.get();
		}
	}

	if (inRect.is_set())
	{
		if (atParam.is_set())
		{
			atFinal = atParam.get() + CharCoords(inRect.get().x.min, inRect.get().y.min);
		}
		else
		{
			atFinal = CharCoords(inRect.get().x.min, inRect.get().y.min);
		}
		if (fromTopLeftParam.is_set())
		{
			atFinal = CharCoords(CharCoords(inRect.get().x.min, inRect.get().y.max)) + fromTopLeftParam.get();
		}
	}
}

Colour const & TextAtBase::get_use_colourise_ink(Display * _display) const
{
	return inkParam.is_set() ? inkParam.get() : base::get_use_colourise_ink(_display);
}

Colour const & TextAtBase::get_use_colourise_paper(Display * _display) const
{
	return paperParam.is_set() ? paperParam.get() : base::get_use_colourise_paper(_display);
}

Vector2 TextAtBase::calculate_char_size(Display* _display) const
{
	if (useFont)
	{
		return useFont->calculate_char_size();
	}
	else
	{
		return _display->get_char_size();
	}
}
