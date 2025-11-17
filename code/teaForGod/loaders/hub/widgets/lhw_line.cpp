#include "lhw_line.h"

#include "..\..\..\tutorials\tutorialSystem.h"

#include "..\..\..\..\core\system\video\video3dPrimitives.h"

using namespace Loader;

//

REGISTER_FOR_FAST_CAST(HubWidgets::Line);

void HubWidgets::Line::render_to_display(Framework::Display* _display)
{
	Colour useColour = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(colour, screen, this);

	if (verticalFirst.is_set())
	{
		Vector2 mid = from;
		if (verticalFirst.get())
		{
			mid.y = to.y;
		}
		else
		{
			mid.x = to.x;
		}
		if (width > 1.0f)
		{
			{
				Vector2 s = from + Vector2::half;
				Vector2 e = mid + Vector2::half;
				Vector2 d = (e - s).normal() * (0.5f * width);
				Vector2 p = -d.rotated_right();
				s -= d;
				e += d;
				::System::Video3DPrimitives::quad_2d(useColour, s + p, e + p, e - p, s - p, false);
			}
			{
				Vector2 s = mid + Vector2::half;
				Vector2 e = to + Vector2::half;
				Vector2 d = (e - s).normal() * (0.5f * width);
				Vector2 p = -d.rotated_right();
				s -= d;
				e += d;
				::System::Video3DPrimitives::quad_2d(useColour, s + p, e + p, e - p, s - p, false);
			}
		}
		else
		{
			::System::Video3DPrimitives::line_2d(useColour, from + Vector2::half, mid + Vector2::half, false);
			::System::Video3DPrimitives::line_2d(useColour, mid + Vector2::half, to + Vector2::half, false);
		}
	}
	else
	{
		if (width > 1.0f)
		{
			Vector2 s = from + Vector2::half;
			Vector2 e = to + Vector2::half;
			Vector2 d = (e - s).normal() * (0.5f * width);
			Vector2 p = -d.rotated_right();
			s -= d;
			e += d;
			::System::Video3DPrimitives::quad_2d(useColour, s + p, e + p, e - p, s - p, false);
		}
		else
		{
			::System::Video3DPrimitives::line_2d(useColour, from + Vector2::half, to + Vector2::half, false);
		}
	}

	base::render_to_display(_display);
}

