#pragma once

#include "lhws_innerScroll.h"
#include "..\loaderHubWidget.h"
#include "..\..\..\..\core\types\hand.h"

namespace Loader
{
	struct HubScreen;
	class Hub;

	namespace HubWidgets
	{
		/*
		 *	List is, well, a list that sometimes may be a grid (all elements have the same size!).
		 *	It can also function in a list mode (one element per row).
		 *
		 *	Possible actions
		 */
		struct List
		: public HubWidget
		{
			FAST_CAST_DECLARE(List);
			FAST_CAST_BASE(HubWidget);
			FAST_CAST_END();

			typedef HubWidget base;
		public:
			void draw_element_border(Framework::Display* _display, Range2 const& _at, HubDraggable const* _element, bool _asSelected);

		public:
			float active = 0.0f;
			float activeTarget = 0.0f;
			
			bool horizontal = false;
			bool gridMode = false;
			bool draggable = false;

			std::function<void(Framework::Display* _display, Range2 const & _at, HubDraggable const * _element)> draw_element = nullptr;
			std::function<bool(HubDraggable const * _element)> should_be_visible = nullptr;

			bool drawElementBorder = true;
			bool drawSelectedColours = true;
			Vector2 elementSize = Vector2(16.0f, 16.0f);
			Optional<Vector2> elementSpacing; // if automatic, will try to make it even to fit elements and a scroll
			Array<RefCountObjectPtr<HubDraggable>> elements;

			AUTO_ Range2 gridWindow; // where grid/elements are on screen
			AUTO_ VectorInt2 gridSize; // this is the size that fits, it can be used either way
			AUTO_ Vector2 actualElementSpacing;
			AUTO_ VectorInt2 wholeGridSize; // space occupied by whole grid
			AUTO_ Vector2 wholeGridSpace; // space occupied by whole grid

			InnerScroll scroll;
			bool allowScrollWithStick = true;

			List();
			List(Name const& _id, Range2 const& _at);

			void be_draggable() { draggable = true; }
			void be_grid() { gridMode = true; }

			void clear();
			HubDraggable* add(IHubDraggableData* _data, Optional<int> const & _at = NP);

			HubDraggable* get_at(Vector2 const& _at);
			int find_element_idx(HubDraggable* _draggable) const;

			void scroll_to_centre(HubDraggable* _draggable);
			void scroll_to_make_visible(HubDraggable* _draggable);

			HubDraggable* change_selection_by(int _by);

		public: // HubWidget
			implement_ void advance(Hub* _hub, HubScreen* _screen, float _deltaTime, bool _beyondModal); // sets in hub active widget

			implement_ void render_to_display(Framework::Display* _display);

			implement_ bool internal_on_hold(int _handIdx, bool _beingHeld, Vector2 const & _at);
			implement_ void internal_on_release(int _handIdx, Vector2 const & _at);

			implement_ bool internal_on_hold_grip(int _handIdx, bool _beingHeld, Vector2 const & _at);
			implement_ void internal_on_release_grip(int _handIdx, Vector2 const & _at);

			implement_ void process_input(int _handIdx, Framework::GameInput const & _input, float _deltaTime);

			// drag'n'drop
			implement_ bool select_instead_of_drag(int _handIdx, Vector2 const& _at);
			implement_ bool get_to_drag(int _handIdx, Vector2 const & _at, OUT_ RefCountObjectPtr<HubDraggable> & _draggable, OUT_ RefCountObjectPtr<HubDraggable> & _placeHolder);
			implement_ void hover(int _handIdx, Vector2 const & _at, HubDraggable* _draggable);
			implement_ void remove_draggable(HubDraggable* _draggable);
			implement_ bool can_drop_hovering(HubDraggable* _draggableDragged) const;
			implement_ HubDraggable* drop_dragged(int _handIdx, Vector2 const & _at, HubDraggable* _draggableDragged, HubDraggable* _draggableToPlace);

			implement_ void clean_up();

		protected:
			Framework::UsedLibraryStored<Framework::Sample> moveSample; // will create on fly using reference "loader hub scene mesh generator"

			void load_defaults();
			void load_assets();

			void calculate_auto_values();

			bool is_visible(HubDraggable const * _element);
		};
	};
};
