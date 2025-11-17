#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\memory\softPooledObject.h"
#include "..\..\core\types\colour.h"

#include "uiCanvasElement.h"
#include "uiCanvasElements.h"

namespace Framework
{
	class Font;

	namespace UI
	{
		struct CanvasWindow
		: public SoftPooledObject<CanvasWindow>
		, public ICanvasElement
		{
			FAST_CAST_DECLARE(CanvasWindow);
			FAST_CAST_BASE(ICanvasElement);
			FAST_CAST_END();

			typedef ICanvasElement base;
		public:
			typedef std::function<void(CanvasWindow*)> FeedbackFunc;

			static CanvasWindow* get();
			
			CanvasWindow();

			CanvasWindow* set_at(Vector2 const& _leftBottom);
			CanvasWindow* set_size(Vector2 const& _size);
			CanvasWindow* set_inner_size(Vector2 const& _size);
			CanvasWindow* set_title(String const& _title) { title = _title; if (!title.is_empty()) movable = true; return this; }
			CanvasWindow* set_title(tchar const * _title) { title = _title; if (!title.is_empty()) movable = true; return this; }
			CanvasWindow* set_closable(bool _closable = true) { closable = _closable; return this; }
			CanvasWindow* set_movable(bool _movable = true) { movable = _movable; return this; }
			CanvasWindow* set_modal(bool _modal = true) { modal = _modal; return this; }
			CanvasWindow* set_use_scroll_vertical(bool _inUse = true) { scrollVertical.inUse = _inUse; return this; }
			CanvasWindow* set_use_scroll_horizontal(bool _inUse = true) { scrollHorizontal.inUse = _inUse; return this; }

			CanvasWindow* set_on_moved(FeedbackFunc _func) { onMoved = _func; return this; }

			CanvasWindow* set_at_pt(Canvas const* _canvas, Vector2 const& _atPt);
			CanvasWindow* set_size_to_fit_all(Canvas const* _canvas, Vector2 const& _margin);
			CanvasWindow* set_centre_at(Canvas const* _canvas, Vector2 const& _centre);
			CanvasWindow* set_align_vertically(Canvas const* _canvas, ICanvasElement const * _rel, Vector2 const & _pt); // pt.x 0 align left, 1 align right, py.y 0 will be above, 1 will be below
			CanvasWindow* set_align_horizontally(Canvas const* _canvas, ICanvasElement const * _rel, Vector2 const & _pt); // pt.x 0 will be on right, 1 will be on left, py.y 0 align bottom, 1 align top

		public:
			Vector2 get_at() const { return leftBottom; }
			Vector2 get_size(Canvas const* _canvas) const;

			Range2 get_inner_placement(Canvas const* _canvas) const;

			bool is_modal() const { return modal; }

		public:
			CanvasElements& access_elements() { return elements; }

			void add(ICanvasElement* _element);

			void place_content_vertically(Canvas const* _canvas, Optional<Vector2> const& _spacing = NP, Optional<bool> const & _fromTop = NP);
			void place_content_horizontally(Canvas const* _canvas, Optional<Vector2> const& _spacing = NP);
	
			void place_content_on_grid(Canvas const* _canvas, Optional<VectorInt2> const & _grid = NP, Optional<Vector2> const& _size = NP, Optional<Vector2> const& _spacing = NP, Optional<bool> const& _fromTop = NP, Optional<bool> const& _keepSizeAsIs = NP); // if any value of _grid is 0, will be calculated

		public:
			void scroll_by(Canvas const* _canvas, Vector2 const& _by);
			void scroll_to_show_element(Canvas const* _canvas, ICanvasElement const * _element);

		public: // SoftPooledObject<Canvas>
			override_ void on_get();
			override_ void on_release();

		public: // ICanvasElement
			implement_ void window_to_top(); // in every space (only if has title)

			implement_ void release_element() { on_release_element(); release(); }

			implement_ void offset_placement_by(Vector2 const& _offset);
			implement_ Range2 get_placement(Canvas const* _canvas) const;

			implement_ void update_for_canvas(Canvas const* _canvas);
			implement_ void render(CanvasRenderContext & _context) const;
			implement_ bool process_shortcuts(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw) const;
			implement_ bool process_controls(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw);
			implement_ void update_hover_on(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw) const;

			implement_ ICanvasElement* find_by_id(Name const& _id, Optional<int> const& _additionalId = NP);

			implement_ void on_hold(REF_ HoldContext& _context);

		private:
			Vector2 leftBottom;
			Optional<Vector2> size;
			Optional<Vector2> innerSize;
			String title;

			FeedbackFunc onMoved;

			CanvasElements elements;

			bool closable = false;
			bool movable = false;
			bool modal = false;

			struct Scroll
			{
				bool inUse = false;
				float scrollableLength = 0.0f;
				float nonScrollableLength = 0.0f; // scrollable + non scrollable = all elements
				float at = 0.0f;
				Range2 scrollArea = Range2::empty; // global
			};
			Scroll scrollVertical;
			Scroll scrollHorizontal;

			Vector2 calculate_size(Canvas const* _canvas) const;

			void update_scrolls(Canvas const* _canvas);
		};
	};
};

TYPE_AS_CHAR(Framework::UI::CanvasWindow);
