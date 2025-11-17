#include "displayDrawCommand_holdDrawing.h"

using namespace Framework;
using namespace DisplayDrawCommands;

HoldDrawing::HoldDrawing()
{
}

bool HoldDrawing::draw_onto(Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ DisplayDrawContext & _context) const
{
	_drawCyclesAvailable = 0;
	return true;
}
