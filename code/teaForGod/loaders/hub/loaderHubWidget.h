#pragma once

#include "loaderHubDraggableData.h"
#include "loaderHubEnums.h"

#include "..\..\..\core\fastCast.h"
#include "..\..\..\core\types\hand.h"
#include "..\..\..\framework\display\display.h"

struct Vector2;
struct VectorInt2;
struct Colour;

namespace System
{
	class Video3D;
};

namespace Framework
{
	class Display;
	class GameInput;
};

namespace TeaForGodEmperor
{
	class Game;
};

namespace Loader
{
	struct HubScreen;
	struct HubWidget;
	class Hub;

	/**
	 *	This struct allows to point at actual data.
	 *	It may exist in at least two different widgets (one source, where it still is during dragging and the target one)
	 */
	struct HubDraggable
	: public RefCountObject
	{
		HubWidget* from = nullptr;
		HubWidget* hoveringOver = nullptr;
		Optional<Hand::Type> heldByHand;
		RefCountObjectPtr<IHubDraggableData> data;
		float initialUpAngle = 0.0f;

		bool forceVisible = false;
		bool placeHolder = false; // it is literally a place holder, holds place when we dragged something out of widget and we couldn't drop it. gets back here

		virtual ~HubDraggable() {}

		bool is_placed() const { return !placeHolder && !heldByHand.is_set() && ! hoveringOver; } // to distinguish whether this one is placed in a widget or is just hovering there, is placeholder 
		bool is_visible_in(HubWidget* _in) const { return !placeHolder || forceVisible; }

		bool can_be_stored() const { return placeHolder && (!heldByHand.is_set() && !hoveringOver); }

		// this is when it is in widget
		Range2 at; // on screen
		float active = 0.0f;
		float activeTarget = 0.0f;
	};

	struct HubWidget
	: public RefCountObject
	{
		FAST_CAST_DECLARE(HubWidget);
		FAST_CAST_END();

	public:
		// these are weak pointers!
		Hub* hub = nullptr;
		HubScreen* screen = nullptr;

		Name id;
		
		Optional<int> customData;

		Range2 at;

		bool ignoreInput = false;

		bool specialHighlight = false;

		std::function<void(HubWidget* _widget, Vector2 const & _at, HubInputFlags::Flags _flags)> on_click = nullptr;
		std::function<void(HubWidget* _widget, Vector2 const & _at, HubInputFlags::Flags _flags)> on_double_click = nullptr;
		std::function<void(HubWidget* _widget, Vector2 const & _at, float _holdTime, float _prevHoldTime)> on_hold = nullptr;
		std::function<void(HubWidget* _widget, Vector2 const & _at, float _holdTime, float _prevHoldTime)> on_hold_grip = nullptr;
		std::function<void(HubWidget* _widget, Vector2 const & _at)> on_release = nullptr;

		std::function<void(HubDraggable const *, bool _clicked)> on_select = nullptr; // if object is selected (clicked, etc)
		std::function<bool(HubDraggable const *, Vector2 const & _at)> can_hover = nullptr; // returns true if it is a valid drop/hover
		std::function<bool(HubDraggable const *, Vector2 const & _at)> can_drop = nullptr; // returns true if it is a valid drop
		std::function<bool(HubDraggable const *)> can_drop_discard = nullptr; // returns true if can discard this element when dropping outside (default false), if you change it to true you may want to implement on_drop_discard
		std::function<bool(HubDraggable const *)> should_stay_on_drag = nullptr; // returns true if should stay (if we drop it somewhere else in the same list, we will remove the one that remained) (default false)
		std::function<void(HubDraggable*)> on_drag_begin = nullptr; // when starts dragging (called when we already removed, added place holder etc)
		std::function<void(HubDraggable*)> on_drag_drop = nullptr; // when about to drop (before on_drop_change)
		std::function<HubDraggable* (HubDraggable*)> on_drop_change = nullptr; // when dropping, this function allows to change draggable object - create a new one, that we will actually drop
		std::function<void(HubDraggable* _toBeRemoved, HubDraggable* _toBePlaced)> on_drop_replace = nullptr; // if defined, is used instead of on_drag_begin+on_drop_discard
		std::function<void(HubDraggable*)> on_drag_dropped = nullptr; // post drop
		std::function<void(HubDraggable*)> on_drop_discard = nullptr; // if object is discarded on dropping outside, we may want to perform an extra thing to handle that (this is done )
		std::function<void()> on_drag_done = nullptr; // when done dragging (called when we already removed, discarded etc, everything is arranged, called for both where we were hovering and from where the draggable was dragged)

		HubWidget() {}
		HubWidget(Name const & _id, Range2 const & _at) : id(_id), at(_at) {}
		virtual ~HubWidget() {}

		virtual void clean_up();

		HubWidget* with_id(Name const& _id) { id = _id; return this; }

	public:
		virtual bool does_allow_to_process_on_click(HubScreen* _screen, int _handIdx, Vector2 const& _at, HubInputFlags::Flags _flags) { return true; } // some widgets may block processing normal clicks
		virtual void internal_on_click(HubScreen* _screen, int _handIdx, Vector2 const & _at, HubInputFlags::Flags _flags) { } // if there is a special thing that may happen on a click
		virtual bool internal_on_hold(int _handIdx, bool _beingHeld, Vector2 const & _at) { return on_hold != nullptr; } // returns true if actually holds, if not being held, means: pressed
		virtual void internal_on_release(int _handIdx, Vector2 const & _at) {}
		virtual bool internal_on_hold_grip(int _handIdx, bool _beingHeld, Vector2 const & _at) { return on_hold_grip != nullptr; } // returns true if actually holds, if not being held, means: pressed
		virtual void internal_on_release_grip(int _handIdx, Vector2 const & _at) {}
		virtual void process_input(int _handIdx, Framework::GameInput const & _input, float _deltaTime) {}

		// drag'n'drop
		virtual bool select_instead_of_drag(int _handIdx, Vector2 const & _at) { return false; } // instead of drag (click!)
		virtual bool get_to_drag(int _handIdx, Vector2 const & _at, OUT_ RefCountObjectPtr<HubDraggable> & _draggable, OUT_ RefCountObjectPtr<HubDraggable> & _placeHolder) { return false; } // get draggable to drag - this is when starting dragging, placeholder may be empty, we won't put that thing back
		virtual void hover(int _handIdx, Vector2 const & _at, HubDraggable* _draggable) {}
		virtual void remove_draggable(HubDraggable* _draggable) {} // more of a utility
		virtual bool can_drop_hovering(HubDraggable* _draggableDragged) const { return true; }
		virtual HubDraggable* drop_dragged(int _handIdx, Vector2 const & _at, HubDraggable* _draggableDragged, HubDraggable* _draggableToPlace) { return nullptr; } // we may need to replace draggable as sometimes when we drop, we put a different draggable (check on_drop_change)

		virtual HubDraggable* tut_get_draggable(TeaForGodEmperor::TutorialHubId const& _id) const { todo_important(TXT("implement")); return nullptr; }

	public:
		virtual void advance(Hub* _hub, HubScreen* _screen, float _deltaTime, bool _beyondModal);

		bool does_required_rendering() const { return requiresRender || alwaysRequiresRender; }
		void mark_requires_rendering() { requiresRender = true; }
		void always_requires_rendering(bool _alwaysRequiresRender = true) { alwaysRequiresRender = _alwaysRequiresRender; }
		void mark_no_longer_requires_rendering() { requiresRender = false; } // should be called before rendering as sometimes widgets may require to rerender on their own

		virtual void render_to_display(Framework::Display* _display) {}

		void play(Framework::UsedLibraryStored<Framework::Sample> const & _sample) const;

	private:
		Optional<int> selected;

		bool requiresRender = true;
		bool alwaysRequiresRender = false;
	};
};
