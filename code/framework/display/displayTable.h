#pragma once

#include "displayControl.h"
#include "displayScrollBar.h"
#include "displayTableStoredState.h"

namespace Framework
{
	class DisplayTable;

	namespace DisplayTableDecoration
	{
		enum Type
		{
			None,
			HeaderOnly,
			Full
		};
	};

	typedef std::function< void(Display* _display, DisplayTable* _table, int _idx) > DisplayTableSelectedRowChanged;

	class DisplayTable
	: public DisplayControl
	{
		FAST_CAST_DECLARE(DisplayTable);
		FAST_CAST_BASE(DisplayControl);
		FAST_CAST_END();

		typedef DisplayControl base;
	public:
		struct SortOrder
		{
			struct Order
			{
				Name column;
				bool reverse = true;
				Order() {}
				Order(Name const & _column, bool _reverse) : column(_column), reverse(_reverse) {}
			};
			ArrayStatic<Order, 10> orders;
			SortOrder() { SET_EXTRA_DEBUG_INFO(orders, TXT("DisplayTable::SortOrder.orders")); }
			SortOrder & by(Name const & _column, bool _reverse = false) { orders.push_back(Order(_column, _reverse)); return *this; }
		};
		struct Column
		: public RefCountObject
		{
			// header columns, describes how content is presented
		public:
			Column() {}
			Column(Name const & _id, String const & _caption) : id(_id), caption(_caption) {}

			Column & hidden(bool _hidden = true) { isHidden = _hidden; return *this; }
			Column & width(CharCoord const & _width) { widthRange = RangeCharCoord(_width); return *this; }
			Column & width_range(CharCoord const & _range) { widthRange = RangeCharCoord(_range); return *this; }
			Column & width_range(CharCoord const & _min, CharCoord const & _max) { widthRange = RangeCharCoord(_min, _max); return *this; }
			Column & width_range(RangeCharCoord const & _range) { widthRange = _range; return *this; }
			Column & width_pt(float _pt) { widthPt = _pt; return *this; }
			Column & header_alignment(DisplayHAlignment::Type _headerHAlignment) { headerHAlignment = _headerHAlignment; return *this; }
			Column & content_alignment(DisplayHAlignment::Type _contentHAlignment) { contentHAlignment = _contentHAlignment; return *this; }
			Column & fill_with_on_overload(String const & _fillWithOnOverload = String(TXT("**********"))) { fillWithOnOverload = _fillWithOnOverload; return *this; }

		private:
			bool isHidden = false; 
			Name id;
			String caption;
			DisplayHAlignment::Type headerHAlignment = DisplayHAlignment::Centre;
			DisplayHAlignment::Type contentHAlignment = DisplayHAlignment::Left;
			String fillWithOnOverload;

			RangeCharCoord widthRange = RangeCharCoord::empty;
			Optional<float> widthPt;
			
			CharCoord actualAt;
			CharCoord actualWidth;
			bool leftover = false; // leftover for width calculation

			friend class DisplayTable;
		};

		struct Row
		: public RefCountObject
		{
			// data row
		public:
			void set(Name const & _id, String const & _value);
			void set_custom_data(void * _customData) { customData = _customData; }
			void * get_custom_data() const { return customData; }

			String const & get(Name const & _id) const;

			CharCoord calc_content_width() const;

		private:
			struct Data
			: public RefCountObject
			{
				Name id;
				String value;

				Data() {}
				Data(Name const & _id, String const & _value) : id(_id), value(_value) {}
			};

			Array<RefCountObjectPtr<Data>> data;
			void * customData = nullptr;

			friend class DisplayTable;
		};

	public:
		DisplayControl* set_on_selected_row(DisplayTableSelectedRowChanged _on_selected_row_fn) { on_selected_row_fn = _on_selected_row_fn; return this; }

	public:
		DisplayScrollBar & access_vertical_scroll_bar() { return verticalScrollBar; }
		
		void use_decoration(DisplayTableDecoration::Type _decoration) { decoration = _decoration; requiresRecalculation = true; }
		void no_header(bool _noHeader = true) { noHeader = _noHeader; requiresRecalculation = true; }

		void keep_vertical_navigation_within(bool _keepVerticalNavigationWithin = true) { keepVerticalNavigationWithin = _keepVerticalNavigationWithin; }
		void always_allow_to_scroll(bool _alwaysAllowToScroll = true) { alwaysAllowToScroll = _alwaysAllowToScroll; }
		void be_full_screen_table_with_horizontal_controls(bool _fullScreenWithHorizontalControls = true);
		void select_row_on_vertical_navigation(bool _selectRowOnVerticalNavigation = true) { selectRowOnVerticalNavigation = _selectRowOnVerticalNavigation; }
		void be_simple_hover_and_click_table(bool _simpleHoverAndClickTable = true) { simpleHoverAndClickTable = _simpleHoverAndClickTable; }

		void sort_by_column(Name const & _column, bool _reverse = false);
		void sort_by_column(SortOrder const & _so);

	public:
		void clear(); // clears everything

		void clear_columns();
		Column & insert_column(int _atIdx, Name const & _id, String const & _caption = String::empty());
		Column & add_column(Name const & _id, String const & _caption = String::empty());
		Column * access_column(Name const & _id);
		void hide_column(Name const & _id) { if (auto * col = access_column(_id)) col->hidden(true); requiresRecalculation = true; }
		void show_column(Name const & _id) { if (auto * col = access_column(_id)) col->hidden(false); requiresRecalculation = true; }
		bool is_column_visible(Name const & _id) { if (auto * col = access_column(_id)) return ! col->isHidden; else return false; }

		void clear_rows();
		Row & add_row();

		void change_selected_row(Display* _display, int _by);
		void select_row_by_custom_data(Display* _display, void * _data, bool _hasToBeVisible = true, bool _selectFirstIfNothingFound = true);
		void select_row(Display* _display, int _idx, bool _haveItVisible = true);
		void act_as_just_selected_row(Display* _display);
		Row * get_selected_row();
		int get_selected_row_index() const { return rowSelectedIdx; }
		Array<RefCountObjectPtr<Row>> const & get_rows() const { return rows; }
		bool is_empty() const { return rows.is_empty(); }

		int get_top_visible_row_index() const { return rowTopIdx; }
		void set_top_visible_row_index(int _rowTopIdx) { rowTopIdx = _rowTopIdx; }

		DisplayTableStoredState store_state();
		void restore_state(DisplayTableStoredState const & _state);

	public: // DisplayControl
		override_ void redraw(Display* _display, bool _clear = false);

		override_ void process_selected(Display* _display, DisplayControl* _prevControl, Optional<VectorInt2> _cursor, Optional<VectorInt2> _goingInDir, bool _silent);
		override_ void process_hover(Display* _display, VectorInt2 const & _cursor);
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

		override_ Vector2 get_from_point_for_navigation(Display* _display, Vector2 const & _inDir) const;

		override_ bool is_active_for_user() const { return base::is_active_for_user() || (fullScreenWithHorizontalControls && is_visible() && !is_locked()); }

	protected: // DisplayControl
		override_ void recalculate(Display* _display);

	private:
		bool alwaysAllowToScroll = false;
		bool fullScreenWithHorizontalControls = false; // if true, table will eat up/down controls at global level taking them as own/inside up/down - this may make moving through UI a bit easier
		bool keepVerticalNavigationWithin = false; // if true, pressing up or down when at top or bottom won't exit control
		bool selectRowOnVerticalNavigation = false; // if true, when entering table with keyboard/gamepad will select last or first row=
		bool simpleHoverAndClickTable = false; // when hovering, will auto select, when clicking, will behave like it was activated/double clicked

		DisplayTableSelectedRowChanged on_selected_row_fn = nullptr;

		DisplayTableDecoration::Type decoration = DisplayTableDecoration::None;
		bool noHeader = false;

		Array<RefCountObjectPtr<Column>> columns;
		Array<RefCountObjectPtr<Row>> rows;
		int rowSelectedIdx = 0;
		int rowTopIdx = 0;
		CACHED_ int rowsDisplayedCount = 0;
		DisplayScrollBar verticalScrollBar;
		bool draggingVerticalScrollBar;

		int pendingSelectIdx = NONE; // special functionality to select when was not added to the display
		bool pendingHasToBeVisible = false;

		bool requiresRecalculation = false;

		void add_draw_commands(Display* _display);

		void make_sure_everything_is_calculated(Display* _display);

		void have_selected_row_visible();

		void update_scroll_bars();
		void update_from_scroll_bars(Display* _display);
		void calculate_rows_top_and_bottom(OUT_ CharCoord & _rowsTopAt, OUT_ CharCoord & _rowsBottomAt) const;

		int find_row(Display* _display, VectorInt2 const & _cursor) const;
	};

};
