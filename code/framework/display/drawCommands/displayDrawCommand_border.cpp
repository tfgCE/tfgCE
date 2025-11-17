#include "displayDrawCommand_border.h"

#include "..\display.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

using namespace Framework;
using namespace DisplayDrawCommands;

Border::Border()
{
}

Border::Border(Colour const & _colour)
: colour(_colour)
{
}

Border::Border(Name const & _useColourPair)
: useColourPair(_useColourPair)
{
}

bool Border::draw_onto(Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ DisplayDrawContext & _context) const
{
	int cost = 10;
	if (_drawCyclesAvailable > cost)
	{
		Colour useColour = colour.is_set() ? colour.get() : _display->get_current_paper();
		Colour tempInkColour = _display->get_current_ink();
		_context.process_colours(_display, false, useColourPair, REF_ tempInkColour, REF_ useColour);

		/** draw rectangles:
		 *	left
		 *	right
		 *	bottom without sides
		 *	top without sides
		 */
		::System::Video3DPrimitives::fill_rect_2d(useColour, _display->get_left_bottom_of_display(), Vector2(_display->get_left_bottom_of_screen().x - 1.0f, _display->get_right_top_of_display().y), false);
		::System::Video3DPrimitives::fill_rect_2d(useColour, Vector2(_display->get_right_top_of_screen().x + 1.0f, _display->get_left_bottom_of_display().y), _display->get_right_top_of_display(), false);
		::System::Video3DPrimitives::fill_rect_2d(useColour, Vector2(_display->get_left_bottom_of_screen().x, _display->get_left_bottom_of_display().y), Vector2(_display->get_right_top_of_screen().x, _display->get_left_bottom_of_screen().y - 1.0f), false);
		::System::Video3DPrimitives::fill_rect_2d(useColour, Vector2(_display->get_left_bottom_of_screen().x, _display->get_right_top_of_screen().y + 1.0f), Vector2(_display->get_right_top_of_screen().x, _display->get_right_top_of_display().y), false);

		_drawCyclesUsed += cost;
		_drawCyclesAvailable -= cost;
		return true;
	}
	else
	{
		return false;
	}
}
