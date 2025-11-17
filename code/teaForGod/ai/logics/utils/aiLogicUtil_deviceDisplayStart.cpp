#include "aiLogicUtil_deviceDisplayStart.h"

#include "..\..\..\..\framework\display\display.h"

#include "..\..\..\..\core\system\video\video3dPrimitives.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
#define TEST_VARIOUS_ICONS
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
#define INVESTIGATE_ISSUES
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

void Utils::draw_device_display_start(Framework::Display* _display, ::System::Video3D* _v3d, int _frame)
{
	Vector2 bl = _display->get_left_bottom_of_screen() + Vector2::half;
	Vector2 tr = _display->get_right_top_of_screen() - Vector2::half;

	// frame
	::System::Video3DPrimitives::line_2d(_display->get_current_ink(), bl, Vector2(bl.x, tr.y));
	::System::Video3DPrimitives::line_2d(_display->get_current_ink(), Vector2(bl.x, tr.y), tr);
	::System::Video3DPrimitives::line_2d(_display->get_current_ink(), tr, Vector2(tr.x, bl.y));
	::System::Video3DPrimitives::line_2d(_display->get_current_ink(), Vector2(tr.x, bl.y), bl);

	int linesLeft = _frame;
	int countx = 5;
	int county = 5;
	{
		Vector2 len = tr - bl;
		if (len.y > len.x)
		{
			county = (int)((float)county * len.y / len.x);
		}
		if (len.x > len.y)
		{
			countx = (int)((float)countx * len.x / len.y);
		}
	}
	for_range(int, i, 1, countx - 2)
	{
		if (linesLeft > 0)
		{
			float x = bl.x + ((float)(i) / (float)(countx - 1)) * (tr.x - bl.x);
			::System::Video3DPrimitives::line_2d(_display->get_current_ink(), Vector2(x, bl.y), Vector2(x, tr.y));
		}
		--linesLeft;
	}
	for_range(int, i, 1, county - 2)
	{
		if (linesLeft > 0)
		{
			float y = bl.y + ((float)(i) / (float)(county - 1)) * (tr.y - bl.y);
			::System::Video3DPrimitives::line_2d(_display->get_current_ink(), Vector2(bl.x, y), Vector2(tr.x, y));
		}
		--linesLeft;
	}
}
