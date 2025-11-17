#pragma once

#include "displayTypes.h"

#include "..\..\core\math\math.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\text\localisedString.h"

#include <functional>

namespace Framework
{
	class Display;
	class DisplayControl;
	struct DisplayRegion;

	typedef std::function< void(Display* _display, DisplayControl* _control, Optional<VectorInt2> _cursor) > ActivateDisplayControl;
	typedef std::function< void(Display* _display, DisplayControl* _control, DisplayControl* _prevControl, Optional<VectorInt2> _cursor, Optional<VectorInt2> _goingInDir) > SelectedDisplayControl;

	class DisplayControl
	: public RefCountObject
	{
		FAST_CAST_DECLARE(DisplayControl);
		FAST_CAST_END();

	public:
		DisplayControl* with_id(Name const & _id) { id = _id; return this; }

		DisplayControl* selectable() { isSelectable = true; isHoverable = true; return this; }
		DisplayControl* not_selectable(bool _butHoverable = false) { isSelectable = false; isHoverable = _butHoverable; return this; }

		DisplayControl* at(CharCoords const & _at) { atParam = _at; return this; }
		DisplayControl* at(CharCoord _x, CharCoord _y) { return at(CharCoords(_x, _y)); }

		DisplayControl* size(CharCoords const & _size) { sizeParam = _size; return this; }
		DisplayControl* size(CharCoord _x, CharCoord _y) { return size(CharCoords(_x, _y)); }
		DisplayControl* size(CharCoord _x) { return size(CharCoords(_x, 1)); }

		DisplayControl* in_region(Name const & _region) { inRegionName = _region; return this; }
		DisplayControl* in_region(DisplayRegion const * _region) { inRegion = _region; return this; }
		DisplayControl* in_region_from_top_left(Name const & _region, CharCoords const & _fromTopLeft) { inRegionName = _region; fromTopLeftParam = _fromTopLeft; return this; }
		DisplayControl* in_region_from_top_left(Name const & _region, CharCoord _x, CharCoord _y) { inRegionName = _region; fromTopLeftParam = CharCoords(_x, _y); return this; }
		DisplayControl* in_region_from_top_left(DisplayRegion const * _region, CharCoords const & _fromTopLeft) { inRegion = _region; fromTopLeftParam = _fromTopLeft; return this; }
		DisplayControl* in_region_from_top_left(DisplayRegion const * _region, CharCoord _x, CharCoord _y) { inRegion = _region; fromTopLeftParam = CharCoords(_x, _y); return this; }

		DisplayControl* h_align_in_region(DisplayHAlignment::Type _alignment) { hAlignmentInRegionParam = _alignment; return this; }
		DisplayControl* h_align_in_region_right() { return h_align_in_region(DisplayHAlignment::Right); }
		DisplayControl* h_align_in_region_centre() { return h_align_in_region(DisplayHAlignment::Centre); }
		DisplayControl* h_align_in_region_left() { return h_align_in_region(DisplayHAlignment::Left); }

		DisplayControl* h_align(DisplayHAlignment::Type _alignment) { hAlignmentParam = _alignment; return this; }
		DisplayControl* h_align_right() { return h_align(DisplayHAlignment::Right); }
		DisplayControl* h_align_centre() { return h_align(DisplayHAlignment::Centre); }
		DisplayControl* h_align_left() { return h_align(DisplayHAlignment::Left); }

		DisplayControl* set_on_activate(ActivateDisplayControl _on_activate_fn) { on_activate_fn = _on_activate_fn; return this; }
		DisplayControl* set_on_selected(SelectedDisplayControl _on_selected_fn) { on_selected_fn = _on_selected_fn; return this; }
		DisplayControl* set_on_activate_simple(std::function<void()> _on_activate_fn) { return set_on_activate([_on_activate_fn](Display* _display, DisplayControl* _control, Optional<VectorInt2> _cursor){_on_activate_fn(); }); }
		DisplayControl* set_on_selected_simple(std::function<void()> _on_selected_fn) { return set_on_selected([_on_selected_fn](Display* _display, DisplayControl* _control, DisplayControl* _prevControl, Optional<VectorInt2> _cursor, Optional<VectorInt2> _goingInDir){_on_selected_fn(); }); }

		DisplayControl* set_shortcut_input_button(Name const & _shortcutInputButton, int _shortcutInputButtonPriority = 0) { shortcutInputButton = _shortcutInputButton; shortcutInputButtonPriority = _shortcutInputButtonPriority; return this; }
		DisplayControl* set_shortcut_selects(bool _shortuctSelects) { shortuctSelects = _shortuctSelects; return this; }
		DisplayControl* set_continuous(bool _continuous = true) { isContinuous = _continuous; return this; }

		DisplayControl* will_draw_immediately() { drawImmediately = true; return this; }

	public:
		virtual void on_added_to(Display* _display);
		virtual void on_removed_from(Display* _display, bool _noRedrawNeeded = false);

		// select, deselect
		virtual void process_selected(Display* _display, DisplayControl* _prevControl, Optional<VectorInt2> _cursor = NP, Optional<VectorInt2> _goingInDir = NP, bool _silent = false);
		virtual void process_deselected(Display* _display);
		// activate (through selector or click)
		virtual void process_activate(Display* _display, Optional<VectorInt2> _cursor = NP);
		// hover
		virtual void process_hover(Display* _display, VectorInt2 const & _cursor);
		// drag
		virtual DisplayControlDrag::Type check_process_drag(Display* _display, VectorInt2 const & _cursor) { return DisplayControlDrag::NotAvailable; } // only informs whether it may drag, activate or select depending on time
		virtual bool process_drag_start(Display* _display, VectorInt2 const & _cursor) { return false; } // returns true if cursor was captured
		virtual bool process_drag_continue(Display* _display, VectorInt2 const & _cursor, VectorInt2 const & _cursorMovement, float _dragTime) { return false; } // returns true if cursor was captured
		virtual bool process_drag_end(Display* _display, VectorInt2 const & _cursor, float _dragTime, OUT_ VectorInt2 & _outCursor) { return false; } // returns true when outCursor has value assigned 
		// click (by default activates)
		virtual void process_early_click(Display* _display, VectorInt2 const & _cursor) { ; } // early click, before double click
		virtual void process_click(Display* _display, VectorInt2 const & _cursor) { process_activate(_display, _cursor); }
		// double click
		virtual bool check_process_double_click(Display* _display, VectorInt2 const & _cursor) { return false; } // check if this control may accept double click
		virtual void process_double_click(Display* _display, VectorInt2 const & _cursor) { ; }
		
		virtual DisplaySelectorDir::Type process_navigation(Display* _display, DisplaySelectorDir::Type _inDir); // if selected
		virtual bool process_navigation_global(Display* _display, DisplaySelectorDir::Type _inDir) { return false; } // global, if not selected, returns true if handled

		virtual void show(Display* _display, bool _show = true);
		virtual void hide(Display* _display);

		virtual void update(Display* _display);

		virtual void redraw(Display* _display, bool _clear = false);

		virtual Vector2 get_from_point_for_navigation(Display* _display, Vector2 const & _inDir) const { return get_sub_centre() + get_sub_half_size() * 0.99f * _inDir; }

	public:
		Display* get_inside_display() const { return insideDisplay; }
		Name const & get_id() const { return id; }
		bool is_visible() const { return isVisible && insideDisplay; }
		bool is_selected() const { return isSelected; }
		virtual bool is_selectable() const { return isSelectable && is_visible() && !is_locked(); }
		virtual bool is_active_for_user() const { return is_selectable(); }
		virtual bool is_hoverable() const { return isHoverable && is_visible() && !is_locked(); }
		bool should_shortcut_select() const { return shortuctSelects; }
		CharCoords const & get_at() const { return actualAt; }
		CharCoords const & get_size() const { return actualSize; }
		CharCoords const get_centre() const { return actualAt + actualSize * 0.5f; }
		CharCoords const & get_bottom_left_corner() const { return actualAt; }
		CharCoords const get_top_right_corner() const { return actualAt + actualSize - CharCoords::one; }
		SubCharCoords const get_sub_centre() const { return actualAt.to_vector2() + actualSize.to_vector2() * 0.5f; }
		SubCharCoords const get_sub_half_size() const { return actualSize.to_vector2() * 0.5f; }

		CharCoords const & get_at_param() const { an_assert(atParam.is_set());  return atParam.is_set() ? atParam.get() : CharCoords::zero; }

		Name const & get_shortcut_input_button() const { return shortcutInputButton; }
		int get_shortcut_input_button_priority() const { return shortcutInputButtonPriority; }
		bool is_continuous() const { return isContinuous; }

		ActivateDisplayControl get_on_activate() const { return on_activate_fn; }
		SelectedDisplayControl get_on_selected() const { return on_selected_fn; }

	protected: friend class Display;
		void set_controls_lock_level(int _controlsLockLevel) { controlsLockLevel = _controlsLockLevel; }
		int get_controls_lock_level() const { return controlsLockLevel; }
		bool is_locked() const { return controlsLockLevel != NONE; }

		void set_controls_stack_level(int _controlsStackLevel) { controlsStackLevel = _controlsStackLevel; }
		int get_controls_stack_level() const { return controlsStackLevel; }

	protected:
		Name id;
		Display* insideDisplay = nullptr;
		int controlsLockLevel = NONE;
		int controlsStackLevel = NONE;
		bool isVisible = true;
		bool isSelected = false;
		bool isSelectable = true;
		bool isHoverable = true;
		bool shortuctSelects = true;
		CharCoords actualAt = CharCoords::zero;
		CharCoords actualSize = CharCoords(8, 1);
		Optional<CharCoords> atParam;
		Optional<CharCoords> sizeParam;

		Optional<Name> inRegionName;
		DisplayRegion const * inRegion = nullptr;
		Optional<CharCoords> fromTopLeftParam;
		Optional<DisplayHAlignment::Type> hAlignmentInRegionParam; // within region
		Optional<DisplayHAlignment::Type> hAlignmentParam;
		Optional<CharCoord> hAlignmentAtLengthParam;

		ActivateDisplayControl on_activate_fn = nullptr;
		SelectedDisplayControl on_selected_fn = nullptr;

		Name shortcutInputButton;
		int shortcutInputButtonPriority = 0;
		bool isContinuous = false; // if continuous, user may hold button to make it active again and again

		bool drawImmediately = false;

	protected:
		virtual void recalculate(Display* _display);

		virtual CharCoords calculate_at(Display* _display) const;
		virtual CharCoords calculate_size(Display* _display) const;

		virtual void add_draw_commands_clear(Display* _display);
	};

};
