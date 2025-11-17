#pragma once

#include "displayControl.h"

namespace Framework
{
	class DisplayTable;

	namespace DisplayScrollBarOrientation
	{
		enum Type
		{
			Vertical,
			Horizontal,
		};
	};

	/**
	 *	For now it is only char based
	 *	And only vertical
	 */
	struct DisplayScrollBar
	{
	public:
		void set_bar_at(RangeCharCoord2 const & _charCoords) { scrollBarAt = _charCoords; updateRequired = true; }

		void set_range(int _minValue, int _maxValue) { range = RangeInt(_minValue, _maxValue); updateRequired = true; }
		void set_range(int _maxValue) { set_range(0, _maxValue); updateRequired = true; }

		void set_displayed_count(int _displayedCount) { displayedCount = _displayedCount; updateRequired = true; }
		
		void set_at(int _atValue) { atIdx = _atValue; updateRequired = true; }
		int get_at() const { return atIdx; }

		void update(); // update all values, is it visible, etc.

	public:
		void be_always_visible(bool _alwaysVisible = true) { alwaysVisible = _alwaysVisible; }
		void be_visible_only_if_required() { alwaysVisible = false; }
		void be_hidden() { isHidden = true; }

		bool is_visible() const;

	public:
		void redraw(Display* _display, bool _parentSelected, bool _drawImmediately = false);

		void process_activate(Display* _display, VectorInt2 const & _cursor);

		bool is_cursor_inside(Display* _display, VectorInt2 const & _cursor) const;

		void process_click(Display* _display, VectorInt2 const & _cursor);

		DisplayControlDrag::Type check_process_drag(Display* _display, VectorInt2 const & _cursor);
		bool process_drag_start(Display* _display, VectorInt2 const & _cursor);
		bool process_drag_continue(Display* _display, VectorInt2 const & _cursor, VectorInt2 const & _cursorMovement, float _dragTime);
		bool process_drag_end(Display* _display, VectorInt2 const & _cursor, float _dragTime, OUT_ VectorInt2 & _outCursor);

		DisplaySelectorDir::Type process_navigation(Display* _display, DisplaySelectorDir::Type _inDir);

	private:
		DisplayScrollBarOrientation::Type orientation = DisplayScrollBarOrientation::Vertical;

		bool updateRequired = true;

		RangeCharCoord2 scrollBarAt;

		bool alwaysVisible = true;
		bool isHidden = false;

		CharCoord scrollBarSize = 0; // how much space visually does it take
		CACHED_ CharCoord scrollBarBoxSize = 0; // size of the box (chars)
		CACHED_ CharCoord scrollBarBoxAtRel = 0; // where at scrollBarSize is box displayed, relative to scrollbar
		CACHED_ CharCoord scrollBarBoxMovementSpace = 0; // how much space does have bar to move
		
		VectorInt2 dragStartedAtCursor;
		int dragStartedAtIdx;

		int displayedCount = 0; // how many values are displayed at one time

		RangeInt range = RangeInt(0, 0);
		int atIdx = 0;

		CACHED_ float scrollBarAtPt = 0.0f;

		void update_cached_values();

		inline void update_if_required() { if (updateRequired) update(); }

		inline void on_at_idx_update();
	};

};
