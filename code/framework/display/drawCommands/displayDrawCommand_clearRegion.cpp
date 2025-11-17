#include "displayDrawCommand_clearRegion.h"

#include "..\display.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

using namespace Framework;
using namespace DisplayDrawCommands;

bool ClearRegion::draw_onto(Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ DisplayDrawContext & _context) const
{
	bool isValid = false;
	Vector2 lb = Vector2::zero;
	Vector2 rt = Vector2::zero;
	if (regionRect.is_set())
	{
		isValid = true;
		if (get_use_coordinates(_display) == DisplayCoordinates::Char)
		{
			lb = _display->get_left_bottom_of_screen() + CharCoords(regionRect.get().x.min, regionRect.get().y.min).to_vector2() * _display->get_char_size();
			rt = _display->get_left_bottom_of_screen() + CharCoords(regionRect.get().x.max, regionRect.get().y.max).to_vector2() * _display->get_char_size() + _display->get_char_size() - Vector2::one; // and add last one
		}
		else
		{
			lb = _display->get_left_bottom_of_screen() + VectorInt2(regionRect.get().x.min, regionRect.get().y.min).to_vector2();
			rt = _display->get_left_bottom_of_screen() + VectorInt2(regionRect.get().x.max, regionRect.get().y.max).to_vector2();
		}
	}
	else if (DisplayRegion const * region = _display->get_region(regionName))
	{
		isValid = true;
		if (get_use_coordinates(_display) == DisplayCoordinates::Char)
		{
			lb = _display->get_left_bottom_of_screen() + region->get_left_bottom().to_vector2() * _display->get_char_size();
			rt = _display->get_left_bottom_of_screen() + region->get_top_right().to_vector2() * _display->get_char_size() + _display->get_char_size() - Vector2::one; // and add last one
		}
		else
		{
			lb = _display->get_left_bottom_of_screen() + region->get_left_bottom().to_vector2();
			rt = _display->get_left_bottom_of_screen() + region->get_top_right().to_vector2();
		}
		if (withinRegionRectFromTopLeft.is_set())
		{
			if (get_use_coordinates(_display) == DisplayCoordinates::Char)
			{
				lb.x = clamp(lb.x + withinRegionRectFromTopLeft.get().x.min * _display->get_char_size().x, lb.x, rt.x);
				lb.y = clamp(rt.y + withinRegionRectFromTopLeft.get().y.min * _display->get_char_size().y, lb.y, rt.y);
				rt.x = clamp(lb.x + withinRegionRectFromTopLeft.get().x.max * _display->get_char_size().x, lb.x, rt.x);
				rt.y = clamp(rt.y + withinRegionRectFromTopLeft.get().y.max * _display->get_char_size().y, lb.y, rt.y);
			}
			else
			{
				lb.x = clamp(lb.x + withinRegionRectFromTopLeft.get().x.min, lb.x, rt.x);
				lb.y = clamp(rt.y + withinRegionRectFromTopLeft.get().y.min, lb.y, rt.y);
				rt.x = clamp(lb.x + withinRegionRectFromTopLeft.get().x.max, lb.x, rt.x);
				rt.y = clamp(rt.y + withinRegionRectFromTopLeft.get().y.max, lb.y, rt.y);
			}
		}
	}

	if (isValid)
	{
		// cost is related to char size - even if size is not related
		int const wholeRegionCost = ((CharCoord)rt.y - (CharCoord)lb.y + 1) / max(1, (int)_display->get_char_size().y) * 10;
		float wholeRegionCostAsFloat = (float)wholeRegionCost;
		float startAtPct = (float)_drawCyclesUsed / wholeRegionCostAsFloat;
		int willEndAt = min(wholeRegionCost, _drawCyclesUsed + _drawCyclesAvailable);
		float endAtPct = (float)willEndAt / wholeRegionCostAsFloat;
		int willUseNow = willEndAt - _drawCyclesUsed;
		_drawCyclesUsed += willUseNow;
		_drawCyclesAvailable -= willUseNow;
		float lb2rtY = rt.y - lb.y;

		Colour useColour = colour.is_set() ? colour.get() : _display->get_current_paper();
		Colour tempInkColour = _display->get_current_ink();
		_context.process_colours(_display, useDefaultColourPairParam.is_set() && useDefaultColourPairParam.get(), useColourPair, REF_ tempInkColour, REF_ useColour);

		lb += get_offset();
		rt += get_offset();

		::System::Video3DPrimitives::fill_rect_2d(useColour, Vector2(lb.x, lb.y + lb2rtY * (1.0f - endAtPct)), Vector2(rt.x, lb.y + lb2rtY * (1.0f - startAtPct)), false);

		return (_drawCyclesUsed >= wholeRegionCost);
	}
	else
	{
		return true;
	}
}
