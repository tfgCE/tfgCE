#include "displayTable.h"

#include "display.h"
#include "displayDrawCommands.h"

#include "..\..\core\containers\arrayStack.h"

using namespace Framework;

//

DEFINE_STATIC_NAME(cursor);
DEFINE_STATIC_NAME_STR(cursorMove, TXT("cursor move"));
DEFINE_STATIC_NAME_STR(softCursor, TXT("soft cursor"));

//

void DisplayTable::Row::set(Name const & _id, String const & _value)
{
	for_every(d, data)
	{
		if (d->get()->id == _id)
		{
			d->get()->value = _value;
			return;
		}
	}

	Data* d = new Data(_id, _value);
	data.push_back(RefCountObjectPtr<Data>(d));
}

String const & DisplayTable::Row::get(Name const & _id) const
{
	for_every(d, data)
	{
		if (d->get()->id == _id)
		{
			return d->get()->value;
		}
	}
	return String::empty();
}

CharCoord DisplayTable::Row::calc_content_width() const
{
	CharCoord width = 0;
	for_every_ref(d, data)
	{
		width += d->value.get_length();
	}
	return width;
}

//

REGISTER_FOR_FAST_CAST(DisplayTable);

void DisplayTable::redraw(Display* _display, bool _clear)
{
	if (insideDisplay == _display && ! is_locked())
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

void DisplayTable::have_selected_row_visible()
{
	rowTopIdx = clamp(rowTopIdx, rowSelectedIdx - rowsDisplayedCount + 1, rowSelectedIdx);
	rowTopIdx = clamp(rowTopIdx, 0, max(0, rows.get_size() - rowsDisplayedCount));
}

bool DisplayTable::process_navigation_global(Display* _display, DisplaySelectorDir::Type _inDir)
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

DisplaySelectorDir::Type DisplayTable::process_navigation(Display* _display, DisplaySelectorDir::Type _inDir)
{
	DisplaySelectorDir::Type inDir = base::process_navigation(_display, _inDir);

	// we don't want to process navigation with scrollbar, row selecting is enough
	//_inDir = verticalScrollBar.process_navigation(_display, _inDir);
	//update_from_scroll_bars(_display);

	int rowSelectedIdxChange = 0;
	if (inDir == DisplaySelectorDir::Up)
	{
		if (rowSelectedIdx > 0)
		{
			rowSelectedIdxChange = -1;
			inDir = DisplaySelectorDir::None; // remain inside
		}
	}
	if (inDir == DisplaySelectorDir::Down)
	{
		if (rowSelectedIdx < rows.get_size() - 1)
		{
			rowSelectedIdxChange = 1;
			inDir = DisplaySelectorDir::None; // remain inside
		}
	}
	if (inDir == DisplaySelectorDir::PageUp)
	{
		rowSelectedIdxChange = max(-rowSelectedIdx, -rowsDisplayedCount);
		inDir = DisplaySelectorDir::None; // remain inside
	}
	if (inDir == DisplaySelectorDir::PageDown)
	{
		rowSelectedIdxChange = min(rows.get_size()-1 - rowSelectedIdx, rowsDisplayedCount);
		inDir = DisplaySelectorDir::None; // remain inside
	}
	if (inDir == DisplaySelectorDir::Home)
	{
		rowSelectedIdxChange = -rowSelectedIdx;
		inDir = DisplaySelectorDir::None; // remain inside
	}
	if (inDir == DisplaySelectorDir::End)
	{
		rowSelectedIdxChange = rows.get_size()-1 - rowSelectedIdx;
		inDir = DisplaySelectorDir::None; // remain inside
	}

	if (rowSelectedIdxChange != 0)
	{
		select_row(_display, rowSelectedIdx + rowSelectedIdxChange, true);
	}
	if (inDir == DisplaySelectorDir::Left)
	{
		inDir = DisplaySelectorDir::Prev;
	}
	if (inDir == DisplaySelectorDir::Right)
	{
		inDir = DisplaySelectorDir::Next;
	}

	int rowTopIdxChange = 0;
	if (inDir == DisplaySelectorDir::ScrollUp)
	{
		if (rowTopIdx > 0)
		{
			rowTopIdxChange = -1;
			inDir = DisplaySelectorDir::None; // remain inside
		}
	}
	if (inDir == DisplaySelectorDir::ScrollDown)
	{
		if (rowTopIdx < rows.get_size() - rowsDisplayedCount)
		{
			rowTopIdxChange = 1;
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

	if (rowTopIdxChange != 0)
	{
		rowTopIdx += rowTopIdxChange;
		rowTopIdx = clamp(rowTopIdx, 0, max(0, rows.get_size() - rowsDisplayedCount));
		if (is_visible())
		{
			redraw(_display);
			_display->draw_all_commands_immediately();

			_display->play_sound(NAME(cursorMove));
		}
	}

	return inDir;
}

void DisplayTable::add_draw_commands(Display* _display)
{
	an_assert(insideDisplay == _display);

	if (pendingSelectIdx != NONE)
	{
		if (pendingSelectIdx != rowSelectedIdx)
		{
			int idx = pendingSelectIdx;
			bool shouldBeVisible = pendingHasToBeVisible;
			pendingSelectIdx = NONE;
			select_row(_display, idx, shouldBeVisible);
			return; // return as select_row will redraw
		}
		else
		{
			pendingSelectIdx = NONE;
		}
	}

	make_sure_everything_is_calculated(_display);

	// clear
	_display->add((new DisplayDrawCommands::ClearRegion())->rect(actualAt, actualSize)->use_default_colour_pair()->immediate_draw(drawImmediately));

	// header
	CharCoord headerCaptionAt = actualAt.y + actualSize.y - 1;
	CharCoord rowsTopAt;
	CharCoord rowsBottomAt;
	if (decoration != DisplayTableDecoration::None)
	{
		headerCaptionAt -= 1;
	}
	calculate_rows_top_and_bottom(rowsTopAt, rowsBottomAt);

	tchar lt = Display::get_frame_lt();
	tchar lc = Display::get_frame_lc();
	tchar lb = Display::get_frame_lb();
	tchar ct = Display::get_frame_ct();
	tchar cc = Display::get_frame_cc();
	tchar cb = Display::get_frame_cb();
	tchar rt = Display::get_frame_rt();
	tchar rc = Display::get_frame_rc();
	tchar rb = Display::get_frame_rb();
	tchar h = Display::get_frame_h();
	tchar v = Display::get_frame_v();

	bool firstColumn = true;
	for_every(column, columns)
	{
		Column * col = column->get();
		if (col->isHidden)
		{
			continue;
		}
		bool lastColumn = false;
		for_every_reverse(columnRev, columns)
		{
			if (columnRev->get()->isHidden)
			{
				continue;
			}
			if (columnRev->get() == column->get())
			{
				lastColumn = true;
			}
			break;
		}
		if (!noHeader)
		{
			DisplayDrawCommands::TextAt* columnCaption = new DisplayDrawCommands::TextAt();
			columnCaption->at(actualAt.x + col->actualAt, headerCaptionAt);
			columnCaption->length(col->actualWidth);
			columnCaption->text(col->caption.to_char());
			columnCaption->h_align(col->headerHAlignment);
			_display->add(columnCaption->immediate_draw(drawImmediately));
		}
		if ((decoration != DisplayTableDecoration::None && !noHeader) ||
			decoration == DisplayTableDecoration::Full)
		{
			Array<tchar> decorationString;
			decorationString.set_size(col->actualWidth + (firstColumn ? 2 : 1) + 1);
			for_every(d, decorationString)
			{
				*d = h;
			}
			decorationString.get_last() = 0;

			// top

			if (firstColumn)
			{
				decorationString.get_first() = lt;
			}
			decorationString[decorationString.get_size() - 2] = lastColumn ? rt : ct;

			DisplayDrawCommands::TextAt* topDecoration = new DisplayDrawCommands::TextAt();
			topDecoration->at(actualAt.x + col->actualAt + (firstColumn ? -1 : 0), headerCaptionAt + 1);
			topDecoration->length(col->actualWidth + (firstColumn ? 2 : 1));
			topDecoration->text(decorationString.get_data());
			_display->add(topDecoration->immediate_draw(drawImmediately));

			// middle
			if (!noHeader)
			{
				if (decoration == DisplayTableDecoration::Full)
				{
					if (firstColumn)
					{
						decorationString.get_first() = lc;
					}
					decorationString[decorationString.get_size() - 2] = lastColumn ? rc : cc;
				}
				else
				{
					if (firstColumn)
					{
						decorationString.get_first() = lb;
					}
					decorationString[decorationString.get_size() - 2] = lastColumn ? rb : cb;
				}

				DisplayDrawCommands::TextAt* bottomDecoration = new DisplayDrawCommands::TextAt();
				bottomDecoration->at(actualAt.x + col->actualAt + (firstColumn ? -1 : 0), headerCaptionAt - 1);
				bottomDecoration->length(col->actualWidth + (firstColumn ? 2 : 1));
				bottomDecoration->text(decorationString.get_data());
				_display->add(bottomDecoration->immediate_draw(drawImmediately));
			}

			if (decoration == DisplayTableDecoration::Full)
			{
				if (firstColumn)
				{
					decorationString.get_first() = lb;
				}
				decorationString[decorationString.get_size() - 2] = lastColumn ? rb : cb;

				DisplayDrawCommands::TextAt* bottomDecoration = new DisplayDrawCommands::TextAt();
				bottomDecoration->at(actualAt.x + col->actualAt + (firstColumn ? -1 : 0), rowsBottomAt - 1);
				bottomDecoration->length(col->actualWidth + (firstColumn ? 2 : 1));
				bottomDecoration->text(decorationString.get_data());
				_display->add(bottomDecoration->immediate_draw(drawImmediately));
			}

			tchar vString[] = { v, 0 };
			if (!noHeader)
			{
				if (lastColumn)
				{
					DisplayDrawCommands::TextAt* sideDecoration = new DisplayDrawCommands::TextAt();
					sideDecoration->at(actualAt.x + col->actualAt + col->actualWidth, headerCaptionAt);
					sideDecoration->text(vString);
					_display->add(sideDecoration->immediate_draw(drawImmediately));
				}
				DisplayDrawCommands::TextAt* sideDecoration = new DisplayDrawCommands::TextAt();
				sideDecoration->at(actualAt.x + col->actualAt - 1, headerCaptionAt);
				sideDecoration->text(vString);
				_display->add(sideDecoration->immediate_draw(drawImmediately));
			}

			if (decoration == DisplayTableDecoration::Full)
			{
				for (int y = rowsBottomAt; y <= rowsTopAt; ++y)
				{
					if (lastColumn)
					{
						DisplayDrawCommands::TextAt* sideDecoration = new DisplayDrawCommands::TextAt();
						sideDecoration->at(actualAt.x + col->actualAt + col->actualWidth, y);
						sideDecoration->text(vString);
						_display->add(sideDecoration->immediate_draw(drawImmediately));
					}
					DisplayDrawCommands::TextAt* sideDecoration = new DisplayDrawCommands::TextAt();
					sideDecoration->at(actualAt.x + col->actualAt - 1, y);
					sideDecoration->text(vString);
					_display->add(sideDecoration->immediate_draw(drawImmediately));
				}
			}
		}
		firstColumn = false;
	}

	for (int y = rowsTopAt; y >= rowsBottomAt; --y)
	{
		int idx = rowsTopAt - y;
		idx += rowTopIdx;
		Row const * row = nullptr;
		if (rows.is_index_valid(idx))
		{
			row = rows[idx].get();
		}
		bool softSelectedRow = idx == rowSelectedIdx;
		bool selectedRow = softSelectedRow && (is_selected() || fullScreenWithHorizontalControls);
		for_every(column, columns)
		{
			Column * col = column->get();
			if (col->isHidden)
			{
				continue;
			}
			DisplayDrawCommands::TextAt* rowData = new DisplayDrawCommands::TextAt();
			rowData->at(actualAt.x + col->actualAt, y);
			rowData->length(col->actualWidth);
			rowData->h_align(col->contentHAlignment);
			if (selectedRow)
			{
				rowData->use_colour_pair(NAME(cursor));
			}
			else if (softSelectedRow)
			{
				rowData->use_colour_pair(NAME(softCursor));
			}
			if (row)
			{
				for_every(data, row->data)
				{
					if (data->get()->id == col->id)
					{
						String const & value = data->get()->value;
						if (!col->fillWithOnOverload.is_empty() &&
							value.get_length() > col->actualWidth)
						{
							String filled;
							while (filled.get_length() < col->actualWidth)
							{
								filled += col->fillWithOnOverload;
							}
							if (softSelectedRow && _display->get_setup().useUpperCaseForSoftSelection)
							{
								rowData->text(filled.to_upper());
							}
							else
							{
								rowData->text(filled);
							}
						}
						else
						{
							if (softSelectedRow && _display->get_setup().useUpperCaseForSoftSelection)
							{
								rowData->text(value.to_upper());
							}
							else
							{
								rowData->text(value);
							}
						}
						break;
					}
				}
			}
			_display->add(rowData->immediate_draw(drawImmediately));
		}
	}

	update_scroll_bars();
	verticalScrollBar.redraw(_display, is_selected() || fullScreenWithHorizontalControls, drawImmediately);
}

void DisplayTable::calculate_rows_top_and_bottom(OUT_ CharCoord & _rowsTopAt, OUT_ CharCoord & _rowsBottomAt) const
{
	_rowsTopAt = actualAt.y + actualSize.y - 1;
	if (!noHeader)
	{
		_rowsTopAt -= 1;
	}
	_rowsBottomAt = actualAt.y;
	if (decoration != DisplayTableDecoration::None)
	{
		if (noHeader)
		{
			_rowsTopAt -= 1;
		}
		else
		{
			_rowsTopAt -= 2;
		}
	}
	if (decoration == DisplayTableDecoration::Full)
	{
		_rowsBottomAt += 1;
	}
}

void DisplayTable::update_scroll_bars()
{
	CharCoord rowsTopAt;
	CharCoord rowsBottomAt;
	calculate_rows_top_and_bottom(rowsTopAt, rowsBottomAt);

	verticalScrollBar.set_range(rows.get_size() - 1);
	verticalScrollBar.set_displayed_count(rowsDisplayedCount);
	verticalScrollBar.set_at(rowTopIdx);
	verticalScrollBar.set_bar_at(RangeCharCoord2(RangeCharCoord(get_top_right_corner().x, get_top_right_corner().x), RangeCharCoord(rowsBottomAt, rowsTopAt)));
	
	verticalScrollBar.update();
}

void DisplayTable::update_from_scroll_bars(Display* _display)
{
	if (rowTopIdx != verticalScrollBar.get_at())
	{
		rowTopIdx = verticalScrollBar.get_at();
		if (is_visible())
		{
			redraw(_display);
			_display->draw_all_commands_immediately();

			_display->play_sound(NAME(cursorMove));
		}
	}
}

void DisplayTable::make_sure_everything_is_calculated(Display* _display)
{
	if (requiresRecalculation)
	{
		recalculate(_display);
	}
}

void DisplayTable::recalculate(Display* _display)
{
	base::recalculate(_display);

	CharCoord availableSize = actualSize.x;

	if (decoration != DisplayTableDecoration::None)
	{
		availableSize -= 2; // sides
		int visibleColumns = 0;
		for_every(column, columns)
		{
			Column * col = column->get();
			if (col->isHidden)
			{
				continue;
			}
			++visibleColumns;
		}
		availableSize -= max(0, visibleColumns - 1); // between
	}

	update_scroll_bars();

	// calculate how many rows do we display
	rowsDisplayedCount = actualSize.y;
	if (!noHeader)
	{
		rowsDisplayedCount -= 1; // one for header
	}
	if (decoration != DisplayTableDecoration::None)
	{
		rowsDisplayedCount -= 2; // two lines - above and below header
	}
	if (decoration == DisplayTableDecoration::Full)
	{
		rowsDisplayedCount -= 1; // one line at the bottom of table
	}
	verticalScrollBar.set_range(rows.get_size() - 1); // required to tell whether scroll bar should be visible

	if (verticalScrollBar.is_visible())
	{
		availableSize -= 1; // for scrollbar
	}

	// first mark all leftovers or not and calculate total pt
	float allPt = 0.0f;
	int leftoverCount = 0;
	for_every(column, columns)
	{
		Column * col = column->get();
		if (col->isHidden)
		{
			continue;
		}
		col->leftover = col->widthRange.max < col->widthRange.min;
		if (col->leftover)
		{
			++leftoverCount;
		}
		if (col->widthPt.is_set())
		{
			allPt += col->widthPt.get();
		}
	}

	CharCoord widthUsed = 0;
	// calculate all that have range set, ignore pt-only for time being (unless it is with range)
	// consider all-pt right now but still limit to provided range
	{
		float currentAllPt = allPt;
		for_every(column, columns)
		{
			Column * col = column->get();
			if (col->isHidden)
			{
				continue;
			}
			col->actualWidth = 0;
			if (!col->leftover)
			{
				if (col->widthPt.is_set())
				{
					// check comment above
					float usePt = col->widthPt.get();
					col->actualWidth = (CharCoord)((float)(availableSize - widthUsed) * usePt * (currentAllPt != 0.0f ? 1.0f / currentAllPt : 0.0f));
					currentAllPt -= usePt;
				}
				col->actualWidth = col->widthRange.clamp(col->actualWidth);
				col->actualWidth = max(1, col->actualWidth);
			}
			widthUsed += col->actualWidth;
		}
	}

	// if we already exceed available size, limit all, leave space for rest - at least one character
	if (widthUsed > availableSize - leftoverCount)
	{
		// limit all of them
		bool ignoreLimits = false;
		while (widthUsed > availableSize - leftoverCount)
		{
			CharCoord prevWidthUsed = widthUsed;
			widthUsed = 0;
			for_every(column, columns)
			{
				Column * col = column->get();
				if (col->isHidden)
				{
					continue;
				}
				if (!col->leftover)
				{
					-- col->actualWidth;
					if (!ignoreLimits && col->widthRange.max >= col->widthRange.min)
					{
						col->actualWidth = col->widthRange.clamp(col->actualWidth);
					}
					col->actualWidth = max(1, col->actualWidth);
				}
				widthUsed += col->actualWidth;
			}
			if (prevWidthUsed == widthUsed)
			{
				if (ignoreLimits)
				{
					break;
				}
				ignoreLimits = true;
			}
		}
	}

	// now leftovers
	if (leftoverCount)
	{
		// calculate rest-pt (sum)
		float restPt = 0.0f;
		float fallbackPt = 1.0f / (float)leftoverCount;
		for_every(column, columns)
		{
			Column * col = column->get();
			if (col->isHidden)
			{
				continue;
			}
			if (col->leftover)
			{
				if (col->widthPt.is_set() && col->widthPt.get() != 0.0f)
				{
					restPt += col->widthPt.get();
				}
				else
				{
					restPt += fallbackPt;
				}
			}
		}

		// calculate how much do we take space (of what is left)
		CharCoord widthLeft = availableSize - widthUsed;
		CharCoord widthLeftUsed = 0;
		{
			float currentRestPt = restPt;
			for_every(column, columns)
			{
				Column * col = column->get();
				if (col->isHidden)
				{
					continue;
				}
				if (col->leftover)
				{
					if (leftoverCount > 1)
					{
						float usePt = fallbackPt;
						if (col->widthPt.is_set() && col->widthPt.get() != 0.0f)
						{
							usePt = col->widthPt.get();
						}
						col->actualWidth = (CharCoord)((float)(widthLeft - widthLeftUsed) * usePt * (currentRestPt != 0.0f ? 1.0f / currentRestPt : 0.0f));
						currentRestPt -= usePt;
					}
					else
					{
						// last one
						col->actualWidth = widthLeft - widthLeftUsed;
					}
					col->actualWidth = max(1, col->actualWidth);
					widthLeftUsed += col->actualWidth;
					--leftoverCount;
				}
			}
		}

		// if the exceed, try to fit
		while (widthLeftUsed > widthLeft && widthLeft >= 0)
		{
			widthLeftUsed = 0;
			for_every(column, columns)
			{
				Column * col = column->get();
				if (col->isHidden)
				{
					continue;
				}
				if (col->leftover)
				{
					col->actualWidth = max(1, col->actualWidth - 1);
				}
				widthLeftUsed += col->actualWidth;
			}
		}
	}

	// calculate ats
	CharCoord actualAt = 0;
	for_every(column, columns)
	{
		Column * col = column->get();
		if (col->isHidden)
		{
			continue;
		}
		if (decoration != DisplayTableDecoration::None)
		{
			actualAt += 1;
		}
		col->actualAt = actualAt;
		actualAt += col->actualWidth;
	}
	
	requiresRecalculation = false;
}

void DisplayTable::clear()
{
	clear_rows();
	clear_columns();
}

void DisplayTable::clear_columns()
{
	columns.clear();

	requiresRecalculation = true;
}

DisplayTable::Column & DisplayTable::insert_column(int _atIdx, Name const & _id, String const & _caption)
{
	Column* column = new Column(_id, _caption);
	columns.insert_at(_atIdx, RefCountObjectPtr<Column>(column));

	requiresRecalculation = true;

	return *column;
}

DisplayTable::Column & DisplayTable::add_column(Name const & _id, String const & _caption)
{
	Column* column = new Column(_id, _caption);
	columns.push_back(RefCountObjectPtr<Column>(column));

	requiresRecalculation = true;

	return *column;
}

DisplayTable::Column * DisplayTable::access_column(Name const & _id)
{
	requiresRecalculation = true;

	for_every(column, columns)
	{
		if (column->get()->id == _id)
		{
			return column->get();
		}
	}

	return nullptr;
}

void DisplayTable::clear_rows()
{
	rows.clear();

	requiresRecalculation = true;
}

DisplayTable::Row & DisplayTable::add_row()
{
	Row* row = new Row();
	rows.push_back(RefCountObjectPtr<Row>(row));

	requiresRecalculation = true;

	return *row;
}

DisplayControlDrag::Type DisplayTable::check_process_drag(Display* _display, VectorInt2 const & _cursor)
{
	if (verticalScrollBar.is_cursor_inside(_display, _cursor))
	{
		return verticalScrollBar.check_process_drag(_display, _cursor);
	}
	else
	{
		return base::check_process_drag(_display, _cursor);
	}
}

bool DisplayTable::process_drag_start(Display* _display, VectorInt2 const & _cursor)
{
	draggingVerticalScrollBar = false;
	if (verticalScrollBar.is_cursor_inside(_display, _cursor))
	{
		draggingVerticalScrollBar = true;
		return verticalScrollBar.process_drag_start(_display, _cursor);
	}
	else
	{
		return base::process_drag_start(_display, _cursor);
	}
}

bool DisplayTable::process_drag_continue(Display* _display, VectorInt2 const & _cursor, VectorInt2 const & _cursorMovement, float _dragTime)
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

bool DisplayTable::process_drag_end(Display* _display, VectorInt2 const & _cursor, float _dragTime, OUT_ VectorInt2 & _outCursor)
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

void DisplayTable::process_hover(Display* _display, VectorInt2 const & _cursor)
{
	if (simpleHoverAndClickTable)
	{
		if (verticalScrollBar.is_cursor_inside(_display, _cursor))
		{
			return;
		}
		else
		{
			int selectIdx = find_row(_display, _cursor);

			if (selectIdx != NONE)
			{
				select_row(_display, selectIdx, true);
			}
		}
	}
}

void DisplayTable::process_early_click(Display* _display, VectorInt2 const & _cursor)
{
	if (verticalScrollBar.is_cursor_inside(_display, _cursor))
	{
		// nothing for early click
		an_assert(false, TXT("we should not end here!"));
		return;
	}
	else
	{
		int selectIdx = find_row(_display, _cursor);

		if (selectIdx != NONE)
		{
			select_row(_display, selectIdx, true);
		}

		if (simpleHoverAndClickTable)
		{
			process_double_click(_display, _cursor);
		}
	}
}

void DisplayTable::process_click(Display* _display, VectorInt2 const & _cursor)
{
	if (verticalScrollBar.is_cursor_inside(_display, _cursor))
	{
		verticalScrollBar.process_click(_display, _cursor);
		update_from_scroll_bars(_display);
	}
	else
	{
		int selectIdx = find_row(_display, _cursor);

		if (selectIdx != NONE)
		{
			select_row(_display, selectIdx, true);
		}
	}
}

bool DisplayTable::check_process_double_click(Display* _display, VectorInt2 const & _cursor)
{
	if (verticalScrollBar.is_cursor_inside(_display, _cursor))
	{
		// we want immediate click
		return false;
	}
	else
	{
		return true;
	}
}

void DisplayTable::process_double_click(Display* _display, VectorInt2 const & _cursor)
{
	if (verticalScrollBar.is_cursor_inside(_display, _cursor))
	{
		return;
	}
	else
	{
		int selectIdx = find_row(_display, _cursor);

		if (selectIdx != NONE)
		{
			select_row(_display, selectIdx, true);
		}

		if (selectIdx != NONE)
		{
			process_activate(_display, _cursor);
		}
	}
}

int DisplayTable::find_row(Display* _display, VectorInt2 const & _cursor) const
{
	CharCoord rowsTopAt;
	CharCoord rowsBottomAt;
	calculate_rows_top_and_bottom(rowsTopAt, rowsBottomAt);
	for (int y = rowsTopAt; y >= rowsBottomAt; --y)
	{
		if (_cursor.y >= y * _display->get_char_size().y &&
			_cursor.y < (y + 1) * _display->get_char_size().y)
		{
			int idx = rowsTopAt - y;
			idx += rowTopIdx;
			if (rows.is_index_valid(idx))
			{
				return idx;
			}
		}
	}

	return NONE;
}

void DisplayTable::act_as_just_selected_row(Display* _display)
{
	if (on_selected_row_fn)
	{
		on_selected_row_fn(_display, this, rowSelectedIdx);
	}
}

void DisplayTable::select_row(Display* _display, int _idx, bool _haveItVisible)
{
	if (rows.is_empty())
	{
		_idx = 0;
	}
	else
	{
		_idx = clamp(_idx, 0, rows.get_size() - 1);
	}

	if (!insideDisplay)
	{
		pendingSelectIdx = _idx;
		pendingHasToBeVisible = _haveItVisible;
		return;
	}

	if (rowSelectedIdx != _idx)
	{
		rowSelectedIdx = _idx;
		if (_haveItVisible)
		{
			have_selected_row_visible();
		}

		if (is_visible())
		{
			redraw(_display);
			_display->draw_all_commands_immediately();

			_display->play_sound(NAME(cursorMove));
		}

		act_as_just_selected_row(_display);
	}
}

void DisplayTable::be_full_screen_table_with_horizontal_controls(bool _fullScreenWithHorizontalControls)
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

DisplayTable::Row * DisplayTable::get_selected_row()
{
	return rows.is_index_valid(rowSelectedIdx) ? rows[rowSelectedIdx].get() : nullptr;
}

void DisplayTable::select_row_by_custom_data(Display* _display, void * _data, bool _hasToBeVisible, bool _selectFirstIfNothingFound)
{
	for_every(row, rows)
	{
		if (row->get()->customData == _data)
		{
			select_row(_display, for_everys_index(row), _hasToBeVisible);
			return;
		}
	}
	if (_selectFirstIfNothingFound)
	{
		select_row(_display, 0, _hasToBeVisible);
	}
}

Vector2 DisplayTable::get_from_point_for_navigation(Display* _display, Vector2 const & _inDir) const
{
	Vector2 point = base::get_from_point_for_navigation(_display, _inDir);

	if (abs(_inDir.x) > abs(_inDir.y))
	{
		CharCoord rowsTopAt;
		CharCoord rowsBottomAt;
		calculate_rows_top_and_bottom(rowsTopAt, rowsBottomAt);
		int y = rowsTopAt - (rowSelectedIdx - rowTopIdx);
		y = clamp(y, rowsBottomAt, rowsTopAt);
		point.y = (float)(y * _display->get_char_size().y + _display->get_char_size().y / 2);
	}

	return point;
}

struct SortDisplayTableRow
{
	RefCountObjectPtr<DisplayTable::Row> row;
	DisplayTable::SortOrder const * so;

	static int compare(void const * _a, void const * _b)
	{
		SortDisplayTableRow const * a = plain_cast<SortDisplayTableRow>(_a);
		SortDisplayTableRow const * b = plain_cast<SortDisplayTableRow>(_b);
		pointerint diff = 0;
		for_every(order, a->so->orders)
		{
			diff = String::diff_icase(a->row->get(order->column), b->row->get(order->column)) * (order->reverse ? -1 : 1);
			if (diff != 0)
			{
				break;
			}
		}
		if (diff == 0)
		{
			diff = (pointerint)a->row->get_custom_data() - (pointerint)b->row->get_custom_data();
		}
		return (int)diff;
	}
};

void DisplayTable::sort_by_column(Name const & _column, bool _reverse)
{
	sort_by_column(SortOrder().by(_column, _reverse));
}

void DisplayTable::sort_by_column(SortOrder const & _so)
{
	ARRAY_STACK(SortDisplayTableRow, toSort, rows.get_size());
	for_every(row, rows)
	{
		SortDisplayTableRow r;
		r.row = *row;
		r.so = &_so;
		toSort.push_back(r);
	}

	sort(toSort);

	rows.clear();
	for_every(row, toSort)
	{
		rows.push_back(row->row);
	}
}

void DisplayTable::process_selected(Display* _display, DisplayControl* _prevControl, Optional<VectorInt2> _cursor, Optional<VectorInt2> _goingInDir, bool _silent)
{
	base::process_selected(_display, _prevControl, _cursor, _goingInDir, _silent);

	if (isSelected && selectRowOnVerticalNavigation && _goingInDir.is_set() && _goingInDir.get().y != 0 && ! rows.is_empty())
	{
		if (_goingInDir.get().y > 0)
		{
			select_row(_display, rows.get_size() - 1);
		}
		else
		{
			select_row(_display, 0);
		}
	}
}

DisplayTableStoredState DisplayTable::store_state()
{
	DisplayTableStoredState state;
	state.rowTopIdx = rowTopIdx;
	state.rowSelectedIdx = rowSelectedIdx;
	state.rowSelectedCustomData = nullptr;
	if (auto * row = get_selected_row())
	{
		state.rowSelectedCustomData = row->get_custom_data();
	}
	return state;
}

void DisplayTable::restore_state(DisplayTableStoredState const & _state)
{
	rowTopIdx = _state.rowTopIdx;
	rowSelectedIdx = clamp(_state.rowSelectedIdx, 0, rows.get_size() - 1);
	if (_state.rowSelectedCustomData)
	{
		for_every_ref(row, rows)
		{
			if (row->get_custom_data() == _state.rowSelectedCustomData)
			{
				rowSelectedIdx = for_everys_index(row);
				break;
			}
		}
	}
}
