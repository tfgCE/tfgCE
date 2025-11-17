#include "displayScrollBar.h"

#include "display.h"
#include "displayDrawCommands.h"

using namespace Framework;

//

DEFINE_STATIC_NAME(cursor);

//

bool DisplayScrollBar::is_visible() const
{
	int const count = range.length();
	return !isHidden && (alwaysVisible || displayedCount < count);
}

void DisplayScrollBar::update()
{
	on_at_idx_update(); // to make sure atIdx is sane

	int const count = range.length();
	float scrollBarBoxPtSize = (float)displayedCount / (float)count;
	int possibleScrollbarMaxPos = max(0, count - displayedCount);
	scrollBarAtPt = possibleScrollbarMaxPos != 0? (float)atIdx / (float)possibleScrollbarMaxPos : 0.0f;

	scrollBarBoxSize = max(1, possibleScrollbarMaxPos != 0 ? min(displayedCount < count ? displayedCount - 1 : displayedCount, (int)((float)displayedCount * scrollBarBoxPtSize)) : displayedCount);
	scrollBarBoxMovementSpace = displayedCount - scrollBarBoxSize;
	scrollBarBoxAtRel = (int)((float)scrollBarBoxMovementSpace * scrollBarAtPt);

	updateRequired = false;
}

void DisplayScrollBar::redraw(Display* _display, bool _parentSelected, bool _drawImmediately)
{
	if (is_visible())
	{
		if (orientation == DisplayScrollBarOrientation::Vertical)
		{
			an_assert(! updateRequired);

			int x = scrollBarAt.x.min;
			for (int y = scrollBarAt.y.max; y >= scrollBarAt.y.min; --y)
			{
				int fromTop = scrollBarAt.y.max - y;
				bool inBox = fromTop >= scrollBarBoxAtRel && fromTop < scrollBarBoxAtRel + scrollBarBoxSize;
				DisplayDrawCommands::TextAt* scrollBar = new DisplayDrawCommands::TextAt();
				scrollBar->at(x, y);
				scrollBar->length(scrollBarAt.x.length());
				if (inBox)
				{
					if (_parentSelected)
					{
						scrollBar->use_colour_pair(NAME(cursor));
					}
					else
					{
						static String v = String::empty() + 0x2502;
						scrollBar->text(v);
					}
				}
				_display->add(scrollBar->immediate_draw(_drawImmediately));
			}
		}
		else
		{
			todo_important(TXT("implement_ other orientations"));
		}
	}
}

bool DisplayScrollBar::is_cursor_inside(Display* _display, VectorInt2 const & _cursor) const
{
	an_assert(!updateRequired);
	if (_display->get_use_coordinates() == DisplayCoordinates::Char)
	{
		return _cursor.x >= scrollBarAt.x.min * _display->get_char_size().x &&
			   _cursor.x < (scrollBarAt.x.max + 1) * _display->get_char_size().x &&
			   _cursor.y >= scrollBarAt.y.min * _display->get_char_size().y &&
			   _cursor.y < (scrollBarAt.y.max + 1) * _display->get_char_size().y;
	}
	else
	{
		return _cursor.x >= scrollBarAt.x.min &&
			   _cursor.x <= scrollBarAt.x.max &&
			   _cursor.y >= scrollBarAt.y.min &&
			   _cursor.y <= scrollBarAt.y.max;
	}
}

void DisplayScrollBar::process_click(Display* _display, VectorInt2 const & _cursor)
{
	if (orientation == DisplayScrollBarOrientation::Vertical)
	{
		Vector2 multiply = _display->get_use_coordinates() == DisplayCoordinates::Char ? _display->get_char_size() : Vector2::one;
		if (_cursor.y > (scrollBarAt.y.max + 1 - scrollBarBoxAtRel) * multiply.y)
		{
			atIdx = atIdx - displayedCount;
			on_at_idx_update();
		}
		else if (_cursor.y < (scrollBarAt.y.max + 1 - scrollBarBoxAtRel - scrollBarBoxSize) * multiply.y)
		{
			atIdx = atIdx + displayedCount;
			on_at_idx_update();
		}
	}
	else
	{
		todo_important(TXT("implement_ other orientations"));
	}
}

DisplayControlDrag::Type DisplayScrollBar::check_process_drag(Display* _display, VectorInt2 const & _cursor)
{
	if (orientation == DisplayScrollBarOrientation::Vertical)
	{
		Vector2 multiply = _display->get_use_coordinates() == DisplayCoordinates::Char ? _display->get_char_size() : Vector2::one;
		if (_cursor.y >= (scrollBarAt.y.max + 1 - scrollBarBoxAtRel - scrollBarBoxSize) * multiply.y &&
			_cursor.y < (scrollBarAt.y.max + 1 - scrollBarBoxAtRel) * multiply.y)
		{
			return DisplayControlDrag::DragImmediately;
		}
	}
	else
	{
		todo_important(TXT("implement_ other orientations"));
	}
	return DisplayControlDrag::NotAvailable;
}

bool DisplayScrollBar::process_drag_start(Display* _display, VectorInt2 const & _cursor)
{
	dragStartedAtCursor = _cursor;
	dragStartedAtIdx = atIdx;
	return false;
}

void DisplayScrollBar::on_at_idx_update()
{
	atIdx = clamp(atIdx, 0, max(0, range.length() - displayedCount));
}

bool DisplayScrollBar::process_drag_continue(Display* _display, VectorInt2 const & _cursor, VectorInt2 const & _cursorMovement, float _dragTime)
{
	if (orientation == DisplayScrollBarOrientation::Vertical)
	{
		Vector2 multiply = _display->get_use_coordinates() == DisplayCoordinates::Char ? _display->get_char_size() : Vector2::one;
		int moved = _cursor.y - dragStartedAtCursor.y;
		int movementSpace = scrollBarBoxMovementSpace * (int)multiply.y;
		if (movementSpace != 0)
		{
			atIdx = dragStartedAtIdx - (max(0, range.length() - displayedCount) * moved) / (movementSpace);
			on_at_idx_update();
		}
		else
		{
			return false;
		}
	}
	else
	{
		todo_important(TXT("implement_ other orientations"));
	}
	return false;
}

bool DisplayScrollBar::process_drag_end(Display* _display, VectorInt2 const & _cursor, float _dragTime, OUT_ VectorInt2 & _outCursor)
{
	return false;
}

DisplaySelectorDir::Type DisplayScrollBar::process_navigation(Display* _display, DisplaySelectorDir::Type _inDir)
{
	if (_inDir == DisplaySelectorDir::ScrollUp)
	{
		atIdx -= 1;
		on_at_idx_update();
	}
	else if (_inDir == DisplaySelectorDir::ScrollDown)
	{
		atIdx += 1;
		on_at_idx_update();
	}
	else if (_inDir == DisplaySelectorDir::PageUp)
	{
		atIdx -= displayedCount;
		on_at_idx_update();
	}
	else if (_inDir == DisplaySelectorDir::PageDown)
	{
		atIdx += displayedCount;
		on_at_idx_update();
	}
	return _inDir;
}
