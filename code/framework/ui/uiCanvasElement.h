#pragma once

#include "..\..\core\fastCast.h"
#include "..\..\core\globalDefinitions.h"
#include "..\..\core\math\math.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\core\types\optional.h"
#include "..\..\core\system\input.h"

namespace Framework
{
	namespace UI
	{
		interface_class ICanvasElement;
		class Canvas;
		struct CanvasElements;
		struct CanvasInputContext;
		struct CanvasRenderContext;
		struct CanvasUpdateWorktable;;
		struct CanvasWindow;

		struct ShortcutParams
		{
			bool ctrl = false;
			bool alt = false;
			bool shift = false;
			bool handleHoldAsPresses = false; // so you don't have to press multiple times, can't work with "act on release"
			bool actOnRelease = false;
			bool notToBeUsed = false; // if set, will only display info but will be ignored when gathered
			ICanvasElement const* cursorHasToBeIn = nullptr; // just ref
			ShortcutParams& with_ctrl() { ctrl = true; return *this; }
			ShortcutParams& with_alt() { alt = true; return *this; }
			ShortcutParams& with_shift() { shift = true; return *this; }
			ShortcutParams& handle_hold_as_presses() { handleHoldAsPresses = true; actOnRelease = false; return *this; }
			ShortcutParams& act_on_release() { actOnRelease = true; handleHoldAsPresses = false; return *this; }
			ShortcutParams& not_to_be_used() { notToBeUsed = true; return *this; }
			ShortcutParams& cursor_has_to_be_in(ICanvasElement* _in) { cursorHasToBeIn = _in; return *this; }
		};

		struct ActContext
		{
			Canvas* canvas = nullptr;
			ICanvasElement* element = nullptr;
			Optional<Vector2> at; // global
			Optional<Vector2> relToCentre;
		};
		typedef std::function<void(ActContext const& _context)> ActFunc;

		interface_class ICanvasElement
		: public SafeObject<ICanvasElement>
		{
			FAST_CAST_DECLARE(ICanvasElement);
			FAST_CAST_END();

		public:
			ICanvasElement();
			virtual ~ICanvasElement();

		public:
			ICanvasElement* set_id(Name const& _id, int _additionalId = 0) { id = _id; additionalId = _additionalId; return this; }
			Name const& get_id() const { return id; }
			int get_additional_id() const { return additionalId; }

		public:
			ICanvasElement* clear_shortcuts();
			ICanvasElement* add_shortcut(System::Key::Type _key, Optional<ShortcutParams> const & _params, std::function<void(ActContext const&)> _perform = nullptr, String const & _info = String::empty());

		public:
			Canvas* get_in_canvas() const;
			CanvasWindow* get_in_window() const;
			bool is_part_of(ICanvasElement const * _e) const;

			void to_top(); // only in current space
			virtual void window_to_top(); // in every space
			void remove();

			void fit_into_canvas(Canvas const* _canvas);

		protected: // derived classes should call it, no one else
			void on_get_element();
			void on_release_element();

		public:
			virtual void release_element() { delete this; }

			virtual void offset_placement_by(Vector2 const & _offset) = 0;
			virtual Range2 get_placement(Canvas const* _canvas) const = 0;
			Vector2 const& get_global_offset() const;

			virtual void update_for_canvas(Canvas const * _canvas) {} // pre rendering etc
			virtual void render(CanvasRenderContext & _context) const = 0;
			virtual void execute_shortcut(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw) const;
			virtual bool process_shortcuts(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw) const;
			virtual bool process_controls(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw); // scrolling etc
			virtual void update_hover_on(CanvasInputContext const & _cic, CanvasUpdateWorktable& _cuw) const {}
			
			virtual ICanvasElement* find_by_id(Name const& _id, Optional<int> const& _additionalId = NP);
			
		public:
			struct ClickContext
			{
				ClickContext(Canvas* _canvas) : canvas(_canvas) {}

				// in
				Canvas * const canvas = nullptr;
				Optional<Vector2> at;
			};
			virtual void on_click(REF_ ClickContext & _context) {}
			virtual bool on_double_click(REF_ ClickContext& _context) { return false; } // return true if handled

		public:
			struct PressContext
			{
				PressContext(Canvas* _canvas) : canvas(_canvas) {}

				// in
				Canvas * const canvas = nullptr;
				Optional<Vector2> at;
			};
			virtual void on_press(REF_ PressContext & _context) {}

		public:
			struct HoldContext
			{
				HoldContext(Canvas* _canvas) : canvas(_canvas) {}

				// in
				Canvas * const canvas = nullptr;
				Optional<Vector2> at;
				Vector2 movedBy = Vector2::zero;

				// in/out
				bool isHolding = false;
				int holdingCustomId = 0;
			};
			virtual void on_hold(REF_ HoldContext & _context) {}

		protected:
			CanvasElements & access_in_elements() { an_assert(inElements); return *inElements; }
			CanvasElements const& get_in_elements() const { an_assert(inElements); return *inElements; }

		protected:
			friend class Canvas;
			friend struct CanvasElements;

			Name id;
			int additionalId = 0;

			CanvasElements* inElements = nullptr;

			struct Shortcut
			{
				System::Key::Type key =
#ifdef AN_STANDARD_INPUT
					System::Key::SPECIAL;
#else
					System::Key::None;
#endif
				ShortcutParams params;
				std::function<void(ActContext const &)> perform;
				String info;
			};
			ArrayStatic<Shortcut, 8> shortcuts;

		protected:
			virtual void prepare_context(ActContext& _context, Canvas* _canvas, Optional<Vector2> const& _actAt) const;
		};
	};
};
