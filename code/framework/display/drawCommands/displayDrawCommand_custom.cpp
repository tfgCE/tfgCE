#include "displayDrawCommand_custom.h"

#include "..\display.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

using namespace Framework;
using namespace DisplayDrawCommands;

Custom::Custom()
{
}

Custom* Custom::use(DrawCustom _draw_custom)
{
	draw_custom = _draw_custom;
	return this;
}

Custom* Custom::use_simple(DrawCustomSimple _draw_custom_simple)
{
	draw_custom_simple = _draw_custom_simple;
	return this;
}

bool Custom::draw_onto(Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ DisplayDrawContext & _context) const
{
	if (draw_custom)
	{
		return draw_custom(_display, _v3d, _drawCyclesUsed, _drawCyclesAvailable, _context);
	}
	else if (draw_custom_simple)
	{
		draw_custom_simple(_display, _v3d);
		return true;
	}
	else
	{
		return true;
	}
}
