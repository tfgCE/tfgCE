#include "displayText.h"

#include "display.h"
#include "displayDrawCommands.h"

#include "..\..\core\containers\arrayStack.h"

using namespace Framework;

//

DEFINE_STATIC_NAME_STR(cursorMove, TXT("cursor move"));

//

REGISTER_FOR_FAST_CAST(DisplayText);

void DisplayText::redraw(Display* _display, bool _clear)
{
	if (insideDisplay == _display && !is_locked())
	{
		if (isVisible && !_clear)
		{
			add_draw_commands(_display);
		}
		else
		{
			add_draw_commands_clear(_display);
		}
	}
}

bool DisplayText::process_navigation_global(Display* _display, DisplaySelectorDir::Type _inDir)
{
	if (alwaysAllowToScroll)
	{
		if (_inDir == DisplaySelectorDir::ScrollDown ||
			_inDir == DisplaySelectorDir::ScrollUp)
		{
			process_navigation(_display, _inDir);
			return true;
		}
	}
	if (fullScreenWithHorizontalControls)
	{
		an_assert(! isSelectable);
		if (_inDir == DisplaySelectorDir::Up ||
			_inDir == DisplaySelectorDir::Down ||
			_inDir == DisplaySelectorDir::PageDown ||
			_inDir == DisplaySelectorDir::PageUp ||
			_inDir == DisplaySelectorDir::Home ||
			_inDir == DisplaySelectorDir::End)
		{
			// we can't be selected, right? do navigation here then!
			process_navigation(_display, _inDir);
			return true;
		}
	}
	return false;
}

DisplaySelectorDir::Type DisplayText::process_navigation(Display* _display, DisplaySelectorDir::Type _inDir)
{
	DisplaySelectorDir::Type inDir = base::process_navigation(_display, _inDir);

	// do navigation through scroll bar, handle just up and down here
	if (!noScroll)
	{
		_inDir = verticalScrollBar.process_navigation(_display, _inDir);
	}
	update_from_scroll_bars(_display);

	int lineTopIdxChange = 0;
	if (inDir == DisplaySelectorDir::Up)
	{
		if (lineTopIdx > 0)
		{
			lineTopIdxChange = -1;
			inDir = DisplaySelectorDir::None; // remain inside
		}
	}
	if (inDir == DisplaySelectorDir::Down)
	{
		if (lineTopIdx < lines.get_size() - linesDisplayedCount)
		{
			lineTopIdxChange = 1;
			inDir = DisplaySelectorDir::None; // remain inside
		}
	}

	if (keepVerticalNavigationWithin)
	{
		if (inDir == DisplaySelectorDir::Up ||
			inDir == DisplaySelectorDir::Down ||
			inDir == DisplaySelectorDir::ScrollUp ||
			inDir == DisplaySelectorDir::ScrollDown)
		{
			inDir = DisplaySelectorDir::None; // remain inside
		}
	}

	if (lineTopIdxChange != 0)
	{
		lineTopIdx += lineTopIdxChange;
		lineTopIdx = clamp(lineTopIdx, 0, max(0, lines.get_size() - linesDisplayedCount));
		if (is_visible())
		{
			redraw(_display);
			_display->draw_all_commands_immediately();

			_display->play_sound(NAME(cursorMove));
		}
	}

	return inDir;
}

void DisplayText::add_draw_commands(Display* _display)
{
	make_sure_everything_is_calculated(_display);

	// clear
	_display->add((new DisplayDrawCommands::ClearRegion())->rect(actualAt, actualSize)->use_default_colour_pair()->immediate_draw(drawImmediately));

	// sizes
	CharCoords linesAt;
	CharCoords linesSize;
	calculate_lines_at_and_size(linesAt, linesSize);

	int width = actualSize.x;
	if (!noScroll && !invisibleScroll)
	{
		width -= 1; // scrollbar
	}

	if (decoration != DisplayTextDecoration::None)
	{
			tchar lt = Display::get_frame_lt();
		//	tchar lc = Display::get_frame_lc();
			tchar lb = Display::get_frame_lb();
		//	tchar ct = Display::get_frame_ct();
		//	tchar cc = Display::get_frame_cc();
		//	tchar cb = Display::get_frame_cb();
			tchar rt = Display::get_frame_rt();
		//	tchar rc = Display::get_frame_rc();
			tchar rb = Display::get_frame_rb();
			tchar h = Display::get_frame_h();
			tchar v = Display::get_frame_v();

		Array<tchar> decorationString;
		decorationString.set_size(width);
		for_every(d, decorationString)
		{
			*d = h;
		}

		decorationString[0] = lt;
		decorationString[width - 1] = rt;
		{
			DisplayDrawCommands::TextAt* topDecoration = new DisplayDrawCommands::TextAt();
			topDecoration->at(actualAt.x, actualAt.y + actualSize.y - 1);
			topDecoration->length(width);
			topDecoration->text(decorationString.get_data());
			topDecoration->ink(inkColour);
			topDecoration->paper(paperColour);
			_display->add(topDecoration->immediate_draw(drawImmediately));
		}

		decorationString[0] = lb;
		decorationString[width - 1] = rb;
		{
			DisplayDrawCommands::TextAt* bottomDecoration = new DisplayDrawCommands::TextAt();
			bottomDecoration->at(actualAt.x, actualAt.y);
			bottomDecoration->length(width);
			bottomDecoration->text(decorationString.get_data());
			bottomDecoration->ink(inkColour);
			bottomDecoration->paper(paperColour);
			_display->add(bottomDecoration->immediate_draw(drawImmediately));
		}

		decorationString.set_size(1);
		decorationString[0] = v;
		for_range(CharCoord, y, actualAt.y + 1, actualAt.y + actualSize.y - 1 - 1)
		{
			{ // left
				DisplayDrawCommands::TextAt* sideDecoration = new DisplayDrawCommands::TextAt();
				sideDecoration->at(actualAt.x, y);
				sideDecoration->length(1);
				sideDecoration->text(decorationString.get_data());
				sideDecoration->ink(inkColour);
				sideDecoration->paper(paperColour);
				_display->add(sideDecoration->immediate_draw(drawImmediately));
			}
			{ // right
				DisplayDrawCommands::TextAt* sideDecoration = new DisplayDrawCommands::TextAt();
				sideDecoration->at(actualAt.x + width - 1, y);
				sideDecoration->length(1);
				sideDecoration->text(decorationString.get_data());
				sideDecoration->ink(inkColour);
				sideDecoration->paper(paperColour);
				_display->add(sideDecoration->immediate_draw(drawImmediately));
			}
		}
	}

	int offsetForVAlignmentIdx = 0;
	if (lines.get_size() < linesDisplayedCount)
	{
		if (vAlignment == DisplayVAlignment::Centre)
		{
			offsetForVAlignmentIdx = -(linesDisplayedCount - lines.get_size()) / 2;
		}
		else if (vAlignment == DisplayVAlignment::Bottom)
		{
			offsetForVAlignmentIdx = -(linesDisplayedCount - lines.get_size());
		}
	}
	int linesTopAt = linesAt.y + linesSize.y - 1;
	for (int y = linesTopAt; y >= linesAt.y; --y)
	{
		int idx = linesTopAt - y;
		idx += lineTopIdx;
		idx += offsetForVAlignmentIdx;
		String const * line = nullptr;
		if (lines.is_index_valid(idx))
		{
			line = &lines[idx];
		}

		DisplayDrawCommands::TextAt* lineData = new DisplayDrawCommands::TextAt();
		lineData->at(linesAt.x, y);
		lineData->length(linesSize.x);
		lineData->h_align(hAlignment);
		if (invisibleScroll)
		{
			// for invisible scroll we need some indicators that there is more text available up or down
			if ((y == linesTopAt && idx > 0) ||
				(y == linesAt.y && idx < lines.get_size() - 1))
			{
				lineData->text(linesSize.x > 8? TXT(". . .") : TXT("..."));
				lineData->h_align_centre();
				line = nullptr;
			}
		}
		if (line)
		{
			lineData->text(*line);
		}
		lineData->ink(inkColour);
		lineData->paper(paperColour);
		_display->add(lineData->immediate_draw(drawImmediately));
	}

	update_scroll_bars();
	if (!noScroll && !invisibleScroll && verticalScrollBar.is_visible())
	{
		verticalScrollBar.redraw(_display, is_selected() || fullScreenWithHorizontalControls, drawImmediately);
	}
}

void DisplayText::calculate_lines_at_and_size(OUT_ CharCoords & _linesAt, OUT_ CharCoords & _linesSize) const
{
	_linesAt = actualAt;
	_linesSize = actualSize;
	if (!noScroll && !invisibleScroll && verticalScrollBar.is_visible())
	{
		_linesSize.x -= 1; // scrollbar
	}
	if (decoration == DisplayTextDecoration::Full)
	{
		_linesAt.x += 1;
		_linesAt.y += 1;
		_linesSize.x -= 2;
		_linesSize.y -= 2;
	}
}

void DisplayText::update_scroll_bars()
{
	if (noScroll)
	{
		return;
	}

	CharCoords linesAt;
	CharCoords linesSize;
	calculate_lines_at_and_size(linesAt, linesSize);

	verticalScrollBar.set_range(lines.get_size() - 1);
	verticalScrollBar.set_displayed_count(linesDisplayedCount);
	verticalScrollBar.set_at(lineTopIdx);
	verticalScrollBar.set_bar_at(RangeCharCoord2(RangeCharCoord(get_top_right_corner().x, get_top_right_corner().x), RangeCharCoord(linesAt.y, linesAt.y + linesSize.y - 1)));

	verticalScrollBar.update();
}

void DisplayText::update_from_scroll_bars(Display* _display)
{
	if (lineTopIdx != verticalScrollBar.get_at())
	{
		lineTopIdx = verticalScrollBar.get_at();
		if (is_visible() && _display)
		{
			redraw(_display);
			_display->draw_all_commands_immediately();

			_display->play_sound(NAME(cursorMove));
		}
	}
}

void DisplayText::add_draw_commands_clear(Display* _display)
{
	make_sure_everything_is_calculated(_display);

	_display->add((new DisplayDrawCommands::ClearRegion())->rect(actualAt, actualSize)->use_default_colour_pair()->immediate_draw(drawImmediately));
}

void DisplayText::make_sure_everything_is_calculated(Display* _display)
{
	if (requiresRecalculation)
	{
		recalculate(_display);
	}
}

void DisplayText::recalculate(Display* _display)
{
	base::recalculate(_display);

	CharCoords linesAt;
	CharCoords linesSize;
	calculate_lines_at_and_size(linesAt, linesSize);
	
	linesDisplayedCount = linesSize.y;

	lines.clear();
	int lineStartedAt = 0;
	int lastLineBreakerAt = NONE;
	ARRAY_STACK(tchar, line, linesSize.x + 1);
	line.clear();
	bool lineProcessed = false;
	for_count(int, idx, text.get_length())
	{
		tchar ch = text[idx];
		if (ch == '\r' || ch == '\t')
		{
			// skip
			continue;
		}
		if (ch == 32)
		{
			lastLineBreakerAt = idx;
		}
		bool newLine = false;
		if (ch == '~' || ch == '\n')
		{
			newLine = true;
		}
		else
		{
			line.push_back(ch);
		}
		if (line.get_size() > linesSize.x)
		{
			newLine = true;
			if (lastLineBreakerAt == NONE)
			{
				if (ch != 32 && ch != '~' && ch != '\n')
				{
					line.set_size(line.get_size() - 1);
					--idx; // will redo char
				}
			}
			else
			{
				line.set_size(lastLineBreakerAt - lineStartedAt);
				idx = lastLineBreakerAt;
			}
		}
		lineProcessed = true;
		if (newLine)
		{
			lastLineBreakerAt = NONE;
			if (!noScroll || lines.get_size() < linesSize.y)
			{
				line.push_back(0);
				lines.push_back(String(line.get_data()));
			}
			line.clear();
			lineProcessed = false;
			lineStartedAt = idx + 1;
		}
	}
	if (lineProcessed)
	{
		if (!noScroll || lines.get_size() < linesSize.y)
		{
			line.push_back(0);
			lines.push_back(String(line.get_data()));
		}
	}

	requiresRecalculation = false;

	// update scroll bar in case we might be outside of our range and read new top idx
	update_scroll_bars();
	update_from_scroll_bars(nullptr);
}

DisplayControlDrag::Type DisplayText::check_process_drag(Display* _display, VectorInt2 const & _cursor)
{
	if (!noScroll && !invisibleScroll && verticalScrollBar.is_cursor_inside(_display, _cursor))
	{
		return verticalScrollBar.check_process_drag(_display, _cursor);
	}
	else
	{
		return base::check_process_drag(_display, _cursor);
	}
}

bool DisplayText::process_drag_start(Display* _display, VectorInt2 const & _cursor)
{
	draggingVerticalScrollBar = false;
	if (!noScroll && !invisibleScroll && verticalScrollBar.is_cursor_inside(_display, _cursor))
	{
		draggingVerticalScrollBar = true;
		return verticalScrollBar.process_drag_start(_display, _cursor);
	}
	else
	{
		return base::process_drag_start(_display, _cursor);
	}
}

bool DisplayText::process_drag_continue(Display* _display, VectorInt2 const & _cursor, VectorInt2 const & _cursorMovement, float _dragTime)
{
	if (draggingVerticalScrollBar)
	{
		bool result = verticalScrollBar.process_drag_continue(_display, _cursor, _cursorMovement, _dragTime);
		update_from_scroll_bars(_display);
		return result;
	}
	else
	{
		return base::process_drag_continue(_display, _cursor, _cursorMovement, _dragTime);
	}
}

bool DisplayText::process_drag_end(Display* _display, VectorInt2 const & _cursor, float _dragTime, OUT_ VectorInt2 & _outCursor)
{
	if (draggingVerticalScrollBar)
	{
		draggingVerticalScrollBar = false;
		return verticalScrollBar.process_drag_end(_display, _cursor, _dragTime, OUT_ _outCursor);
	}
	else
	{
		return base::process_drag_end(_display, _cursor, _dragTime, OUT_ _outCursor);
	}
}

void DisplayText::process_early_click(Display* _display, VectorInt2 const & _cursor)
{
	if (!noScroll && !invisibleScroll && verticalScrollBar.is_cursor_inside(_display, _cursor))
	{
		// nothing for early click
		an_assert(false, TXT("we should not end here!"));
		return;
	}
}

void DisplayText::process_click(Display* _display, VectorInt2 const & _cursor)
{
	if (!noScroll && !invisibleScroll && verticalScrollBar.is_cursor_inside(_display, _cursor))
	{
		verticalScrollBar.process_click(_display, _cursor);
		update_from_scroll_bars(_display);
	}
}

bool DisplayText::check_process_double_click(Display* _display, VectorInt2 const & _cursor)
{
	if (!noScroll && !invisibleScroll && verticalScrollBar.is_cursor_inside(_display, _cursor))
	{
		// we want immediate click
		return false;
	}
	else
	{
		return true;
	}
}

void DisplayText::process_double_click(Display* _display, VectorInt2 const & _cursor)
{
	if (!noScroll && !invisibleScroll && verticalScrollBar.is_cursor_inside(_display, _cursor))
	{
		return;
	}
}

void DisplayText::be_full_screen_table_with_horizontal_controls(bool _fullScreenWithHorizontalControls)
{
	fullScreenWithHorizontalControls = _fullScreenWithHorizontalControls;

	if (fullScreenWithHorizontalControls)
	{
		alwaysAllowToScroll = true;
		not_selectable(true);
	}
	else
	{
		alwaysAllowToScroll = false;
		selectable();
	}
}

void DisplayText::scroll_to_top(Display* _display)
{
	if (lineTopIdx == 0)
	{
		return;
	}
	lineTopIdx = 0;
	if (is_visible() && _display)
	{
		redraw(_display);
		_display->draw_all_commands_immediately();

		_display->play_sound(NAME(cursorMove));
	}
}