#pragma once

#include "displayControl.h"
#include "displayScrollBar.h"

#include "..\..\core\memory\softPooledObject.h"

namespace Framework
{
	namespace DisplayTextDecoration
	{
		enum Type
		{
			None,
			Full
		};
	};

	class DisplayText
	: public DisplayControl
	, public SoftPooledObject<DisplayText>
	{
		FAST_CAST_DECLARE(DisplayText);
		FAST_CAST_BASE(DisplayControl);
		FAST_CAST_END();

		typedef DisplayControl base;
	public:
		DisplayScrollBar & access_vertical_scroll_bar() { return verticalScrollBar; }
		void scroll_to_top(Display* _display);

		void use_decoration(DisplayTextDecoration::Type _decoration) { decoration = _decoration; requiresRecalculation = true; }

		DisplayText* no_scroll(bool _noScroll = true) { noScroll = _noScroll; return this; }
		DisplayText* with_scroll(bool _withScroll = true) { noScroll = !_withScroll; invisibleScroll = false; return this; }
		DisplayText* with_invisible_scroll(bool _withScroll = true) { noScroll = !_withScroll; invisibleScroll = _withScroll; return this; }

		void keep_vertical_navigation_within(bool _keepVerticalNavigationWithin = true) { keepVerticalNavigationWithin = _keepVerticalNavigationWithin; }

		void always_allow_to_scroll(bool _alwaysAllowToScroll = true) { alwaysAllowToScroll = _alwaysAllowToScroll; }

		void be_full_screen_table_with_horizontal_controls(bool _fullScreenWithHorizontalControls = true);

		void selectable_only_if_theres_need_to_scroll(bool _isSelectableOnlyIfTheresNeedToScroll = true) { isSelectableOnlyIfTheresNeedToScroll = _isSelectableOnlyIfTheresNeedToScroll; }

	public:
		void set_text(String const & _text) { set_text(_text.to_char()); }
		void set_text(tchar const * _text) { text = _text; requiresRecalculation = true; }
		String const & get_text() const { return text; }

		void set_text_h_alignment(DisplayHAlignment::Type _hAlignment) { hAlignment = _hAlignment; }
		void set_text_v_alignment(DisplayVAlignment::Type _vAlignment) { vAlignment = _vAlignment; }

		int get_lines_occupied() const { return min(linesDisplayedCount, lines.get_size()); }

	public:
		void set_ink_colour(Colour const & _colour) { inkColour = _colour; }
		void set_ink_colour(Optional<Colour> const & _colour) { inkColour = _colour; }
		void clear_ink_colour() { inkColour.clear(); }

		void set_paper_colour(Colour const & _colour) { paperColour = _colour; }
		void set_paper_colour(Optional<Colour> const & _colour) { paperColour = _colour; }
		void clear_paper_colour() { paperColour.clear(); }

	public: // DisplayControl
		override_ bool is_selectable() const { return base::is_selectable() && (! isSelectableOnlyIfTheresNeedToScroll || lines.get_size() > linesDisplayedCount); }
		override_ bool is_active_for_user() const { return base::is_active_for_user() || ((alwaysAllowToScroll || fullScreenWithHorizontalControls) && is_visible() && !is_locked()); }

		override_ void redraw(Display* _display, bool _clear = false);

		override_ DisplayControlDrag::Type check_process_drag(Display* _display, VectorInt2 const & _cursor);
		override_ bool process_drag_start(Display* _display, VectorInt2 const & _cursor);
		override_ bool process_drag_continue(Display* _display, VectorInt2 const & _cursor, VectorInt2 const & _cursorMovement, float _dragTime);
		override_ bool process_drag_end(Display* _display, VectorInt2 const & _cursor, float _dragTime, OUT_ VectorInt2 & _outCursor);
		override_ void process_early_click(Display* _display, VectorInt2 const & _cursor);
		override_ void process_click(Display* _display, VectorInt2 const & _cursor);
		override_ bool check_process_double_click(Display* _display, VectorInt2 const & _cursor);
		override_ void process_double_click(Display* _display, VectorInt2 const & _cursor);

		override_ DisplaySelectorDir::Type process_navigation(Display* _display, DisplaySelectorDir::Type _inDir);
		override_ bool process_navigation_global(Display* _display, DisplaySelectorDir::Type _inDir);

	protected: // DisplayControl
		override_ void recalculate(Display* _display);

	protected: // RefCountObject
		override_ void destroy_ref_count_object() { release(); }

	protected: // SoftPooledObject
		override_ void on_release() { *this = DisplayText(); }

	private:
		bool noScroll = false;
		bool invisibleScroll = false;
		bool alwaysAllowToScroll = false; // will always allow to scroll, will accept global navigation
		bool fullScreenWithHorizontalControls = false; // if true, table will eat up/down controls at global level taking them as own/inside up/down - this may make moving through UI a bit easier
		bool keepVerticalNavigationWithin = false; // if true, pressing up or down when at top or bottom won't exit control
		bool isSelectableOnlyIfTheresNeedToScroll = false;

		DisplayTextDecoration::Type decoration = DisplayTextDecoration::None;

		DisplayHAlignment::Type hAlignment = DisplayHAlignment::Left;
		DisplayVAlignment::Type vAlignment = DisplayVAlignment::Top;

		String text;
		CACHED_ Array<String> lines; // text broken into lines

		Optional<Colour> inkColour;
		Optional<Colour> paperColour;

		int lineTopIdx = 0;
		CACHED_ int linesDisplayedCount = 0;
		DisplayScrollBar verticalScrollBar;
		bool draggingVerticalScrollBar;

		bool requiresRecalculation = false;

		void add_draw_commands(Display* _display);
		void add_draw_commands_clear(Display* _display);

		void make_sure_everything_is_calculated(Display* _display);

		void update_scroll_bars();
		void update_from_scroll_bars(Display* _display);
		void calculate_lines_at_and_size(OUT_ CharCoords & _linesAt, OUT_ CharCoords & _linesSize) const;
	};

};

DECLARE_REGISTERED_TYPE(RefCountObjectPtr<Framework::DisplayText>);

TYPE_AS_CHAR(Framework::DisplayText);
