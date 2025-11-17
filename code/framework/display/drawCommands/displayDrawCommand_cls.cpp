#include "displayDrawCommand_cls.h"

#include "..\display.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace DisplayDrawCommands;

//

CLS::CLS()
{
}

CLS::CLS(Colour const & _colour)
: colour(_colour)
{
}

CLS::CLS(Name const & _useColourPair)
: useColourPair(_useColourPair)
{
}

bool CLS::draw_onto(Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ DisplayDrawContext & _context) const
{
	int const wholeScreenCost = max(1, _display->get_char_resolution().y * 10); // max as it could be too small for a single character
	float wholeScreenCostAsFloat = (float)wholeScreenCost;
	float startAtPct = (float)_drawCyclesUsed / wholeScreenCostAsFloat;
	int willEndAt = min(wholeScreenCost, _drawCyclesUsed + _drawCyclesAvailable);
	float endAtPct = (float)willEndAt / wholeScreenCostAsFloat;
	int willUseNow = willEndAt - _drawCyclesUsed;
	_drawCyclesUsed += willUseNow;
	_drawCyclesAvailable -= willUseNow;
	Vector2 lb = _display->get_left_bottom_of_screen();
	Vector2 rt = _display->get_right_top_of_screen();
	float lb2rtY = rt.y - lb.y;

	Colour useColour = colour.is_set() ? colour.get() : _display->get_current_paper();
	Colour tempInkColour = _display->get_current_ink();
	_context.process_colours(_display, false, useColourPair, REF_ tempInkColour, REF_ useColour);

	::System::Video3DPrimitives::fill_rect_2d(useColour, Vector2(lb.x, lb.y + lb2rtY * (1.0f - endAtPct)), Vector2(rt.x, lb.y + lb2rtY * (1.0f - startAtPct)), false);

	return (_drawCyclesUsed >= wholeScreenCost);
}
