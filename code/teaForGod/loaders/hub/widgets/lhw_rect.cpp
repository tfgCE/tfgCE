#include "lhw_rect.h"

#include "..\..\..\tutorials\tutorialSystem.h"

#include "..\..\..\..\core\system\video\video3dPrimitives.h"

using namespace Loader;

//

REGISTER_FOR_FAST_CAST(HubWidgets::Rect);

void HubWidgets::Rect::render_to_display(Framework::Display* _display)
{
	Colour useColour = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(colour, screen, this);

	Range2 a = at;
	a.x.min = round(a.x.min);
	a.y.min = round(a.y.min);
	a.x.max = round(a.x.max);
	a.y.max = round(a.y.max);
	if (fill)
	{
		::System::Video3DPrimitives::fill_rect_2d(useColour, a, false);
	}
	else
	{
		::System::Video3DPrimitives::rect_2d(useColour, a, false);
	}

	base::render_to_display(_display);
}

