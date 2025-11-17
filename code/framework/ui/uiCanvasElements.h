#pragma once

#include "..\..\core\containers\array.h"
#include "..\..\core\math\math.h"

namespace Framework
{
	namespace UI
	{
		class Canvas;
		interface_class ICanvasElement;
		struct CanvasInputContext;
		struct CanvasRenderContext;
		struct CanvasUpdateWorktable;

		struct CanvasElements
		{
			CanvasElements(ICanvasElement* _isPartOfElement);
			CanvasElements(Canvas* _isMainCanvas);
			~CanvasElements();

			void reset();
			void clear();
			bool is_empty() const { return elements.is_empty(); }

			void update_for_canvas(Canvas const* _canvas) const;

			void render(CanvasRenderContext& _context) const;
			void add(ICanvasElement* _element);
			void remove(ICanvasElement* _element);
			void to_top(ICanvasElement* _element);
			void remove_last();

			ICanvasElement* find_by_id(Name const& _id, Optional<int> const& _additionalId = NP) const;

			ICanvasElement* get_in_element() const;
			Canvas* get_in_canvas() const;

			void offset_placement_by(Vector2 const& _offset);

		public:
			Concurrency::SpinLock& access_lock() { return accessLock; }

			Array<ICanvasElement*> & access_elements() { an_assert(accessLock.is_locked_on_this_thread()); return elements; }
			Array<ICanvasElement*> const& get_elements() const { an_assert(accessLock.is_locked_on_this_thread()); return elements; }

		public:
			bool process_shortcuts(CanvasInputContext const& _cic, CanvasUpdateWorktable & _cuw) const;
			bool process_controls(CanvasInputContext const& _cic, CanvasUpdateWorktable & _cuw);
			void update_hover_on(CanvasInputContext const& _cic, CanvasUpdateWorktable & _cuw) const;

		public:
			Vector2 const& get_global_offset() const { return globalOffset; }
			void set_global_offset(Vector2 const& _offset) const { globalOffset = _offset; }

		private:
			mutable Concurrency::SpinLock accessLock;

			ICanvasElement* isPartOfElement = nullptr;
			Canvas* isMainCanvas = nullptr;

			Array<ICanvasElement*> elements;

			mutable Vector2 globalOffset = Vector2::zero;
		};
	};
};
