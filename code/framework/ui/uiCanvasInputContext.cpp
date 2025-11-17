#include "uiCanvasInputContext.h"

#include "uiCanvas.h"

#include "..\..\core\system\input.h"
#include "..\..\core\system\video\video3d.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace UI;

//

CanvasInputContext::CanvasInputContext()
{
}

void CanvasInputContext::set_cursor(int _idx, Cursor const& _cursor)
{
	if (_idx < 0)
	{
		return;
	}
	cursors.set_size(max(_idx + 1, cursors.get_size()));
	cursors[_idx] = _cursor;
}

void CanvasInputContext::keep_only_cursor(int _idx)
{
	for_every(cursor, cursors)
	{
		if (for_everys_index(cursor) != _idx)
		{
			*cursor = Cursor();
		}
	}
}

//--

CanvasInputContext::Cursor::Cursor()
{
}

CanvasInputContext::Cursor CanvasInputContext::Cursor::use_mouse(Canvas const* _canvas)
{
	Cursor c;

	if (auto* input = ::System::Input::get())
	{
		Vector2 mouseAt = input->get_mouse_window_location();
		Vector2 mouseRel = input->get_mouse_relative_location();
		c.at = _canvas->real_to_logical_location(mouseAt);
		c.movedBy = _canvas->real_to_logical_vector(mouseRel);

		c.buttonFlags = 0;
		c.prevButtonFlags = 0;
		for_count(int, i, 3)
		{
			c.buttonFlags |= input->is_mouse_button_pressed(i) ? bit(i) : 0;
			c.prevButtonFlags |= input->was_mouse_button_pressed(i) ? bit(i) : 0;
		}
		
#ifdef AN_STANDARD_INPUT
		if (input->has_key_been_pressed(System::Key::MouseWheelUp)) c.scrollV += 1;
		if (input->has_key_been_pressed(System::Key::MouseWheelDown)) c.scrollV -= 1;
		if (input->has_key_been_pressed(System::Key::MouseWheelRight)) c.scrollH += 1;
		if (input->has_key_been_pressed(System::Key::MouseWheelLeft)) c.scrollH -= 1;
#endif
	
	}

	return c;
}
