#include "lhw_customDraw.h"

#include "..\..\..\..\framework\video\texturePart.h"

using namespace Loader;

//

REGISTER_FOR_FAST_CAST(HubWidgets::CustomDraw);

void HubWidgets::CustomDraw::render_to_display(Framework::Display* _display)
{
	{
		scoped_call_stack_info(id.to_char());
		if (custom_draw)
		{
			custom_draw(_display, at);
		}
	}

	base::render_to_display(_display);
}

void HubWidgets::CustomDraw::clean_up()
{
	base::clean_up();
	custom_draw = nullptr;
}
