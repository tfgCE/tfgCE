#pragma once

#include "lhws_innerScroll.h"
#include "..\loaderHubWidget.h"
#include "..\..\..\..\core\types\hand.h"

#define GRID_SHAPE_ONE HubWidgets::GridShape::rect(VectorInt2(1, 1))

namespace Loader
{
	struct HubScreen;
	class Hub;

	namespace HubWidgets
	{
		struct GridPlacement
		{
			VectorInt2 at = VectorInt2::zero; // left bottom
			VectorInt2 sampleAt = VectorInt2::zero; // left bottom - where sample was played
			int rotation = 0; // clockwise

			GridPlacement() {}
			GridPlacement(VectorInt2 const& _at, int _rotation) : at(_at), rotation(_rotation) {}
		};

		struct GridShape
		{
			static const int MAX_SIZE = 50;
			GridShape();

			static GridShape rect(VectorInt2 const& _size);

			bool& at(VectorInt2 const& _at) { return at(_at.x, _at.y); }
			bool& at(int _x, int _y) { an_assert(_x >= 0 && _x < MAX_SIZE && _y >= 0 && _y < MAX_SIZE); return shape[_y * MAX_SIZE + _x]; }

			bool get(VectorInt2 const& _at) const { return get(_at.x, _at.y); }
			bool get(int _x, int _y) const { return (_x >= 0 && _x < MAX_SIZE && _y >= 0 && _y < MAX_SIZE) ? shape[_y * MAX_SIZE + _x] : false; }

			void calc_size();
			VectorInt2 get_rotated_size(int _rotated) const;
			VectorInt2 const& get_size() const { return size; }

			void rotate(int _rotation);
			bool move_by(VectorInt2 const& _by);
			bool can_add(GridShape const& _shape);
			bool add(GridShape const& _shape);

			GridShape & set_from(bool const * _leftBottom, VectorInt2 const & _arraySize); // by rows

			void log(LogInfoContext& _context) const;

		private:
			AUTO_ VectorInt2 size;
			ArrayStatic<bool, MAX_SIZE* MAX_SIZE> shape;
		};

		struct GridDraggable
		: public RefCountObject
		{
			RefCountObjectPtr<HubDraggable> draggable;

			// gridShape is the shape it has before movement and rotation
			GridShape gridShape;
			GridPlacement gridPlacement;
			float upAngleRef = 0.0; // if we move to far, we switch angle and rotate

			bool validPlacement = true;
			bool outOfBounds = false;
			bool hoveringOverOtherElements = false;

			GridDraggable(HubDraggable* _draggable = nullptr) : draggable(_draggable) {}

			void update_at(Range2 const& _gridWindow, Vector2 const& _gridScroll, Vector2 const& _elementSize);
			bool does_contain(Vector2 const& _at, Vector2 const& _elementSize) const;
			Vector2 get_loc(Vector2 const& _at, Vector2 const& _elementSize) const;
			VectorInt2 get_shape_grid_loc(Vector2 const& _at, Vector2 const& _elementSize) const;
			VectorInt2 get_shape_grid_loc(VectorInt2 const& _absAt) const;

			VectorInt2 get_rotated_size() const;
			GridShape get_shape_in_place() const;
		};

		/*
		 *	Grid elements may have different size
		 *	0,0 is at left bottom
		 */
		struct Grid
		: public HubWidget
		{
			FAST_CAST_DECLARE(Grid);
			FAST_CAST_BASE(HubWidget);
			FAST_CAST_END();

			typedef HubWidget base;
		public:
			float active = 0.0f;
			float activeTarget = 0.0f;

			bool draggable = false;
			bool allowRotation = false;
			bool snapHoveringToInterior = false;
			bool allowReplacing = false; // if we want to place where something else is, that something else is going to be replaced

			std::function<GridShape(HubDraggable const* _draggable)> get_shape_info_for = nullptr; // if null, is 1,1
			std::function<bool(HubDraggable const* _draggable)> can_drag = nullptr; // if null, can drag always
			std::function<void(Framework::Display * _display, Range2 const& _at, GridDraggable const* _element)> draw_element = nullptr;
			std::function<void(Framework::Display* _display, Range2 const& _at)> draw_whole_grid = nullptr;
			std::function<void(Framework::Display* _display, VectorInt2 const & _gridCoord, Range2 const& _at, bool _available)> draw_grid_element = nullptr;
			std::function<void(Framework::Display* _display, Range2 const& _at)> draw_whole_grid_post = nullptr;

			Optional<VectorInt2> forcedGridSize; // if forced, will auto centre (no it won't, auto centre works in general)
			Vector2 elementSize = Vector2(16.0f, 16.0f);
			VectorInt2 autoElementSizeBasedOnGridSize = VectorInt2::zero; // will be used to calculate element size basing on this grid size / may differ from forced grid size
			Array<RefCountObjectPtr<GridDraggable>> elements;

			AUTO_ Range2 gridWindow; // where grid/elements are on screen
			AUTO_ Vector2 gridScroll; // apply scroll
			AUTO_ VectorInt2 gridSize; // this is the size that fits, it can be uled either way
			AUTO_ VectorInt2 visibleGridSize; // this is the visible part
			AUTO_ Vector2 wholeGridSpace; // space occupied by whole grid

			InnerScroll scroll;

			Grid() { load_defaults(); load_assets(); }
			Grid(Name const & _id, Range2 const & _at) : base(_id, _at) { load_defaults(); load_assets(); }

			void put_anywhere(HubDraggable* _draggable, bool _verticalFirst = false, bool _startFromTop = false, Optional<GridShape> const & _shape = NP);

			void clear(); // removes all draggables from the grid, but does nothing else
			void discard_all(); // calls on_drop_discard
			GridDraggable* add(IHubDraggableData* _data, GridShape const & _shape, GridPlacement const & _gridPlacement); // shape should be as is in zero rotation
			GridDraggable* add_anywhere(IHubDraggableData* _data, GridShape const& _shape, bool _verticalFirst = false, bool _startFromTop = false);

			GridDraggable* get_at(Vector2 const & _at); // without scroll

			void select_in_dir(HubDraggable const* _from, VectorInt2 const& _inDir, bool _keepStraightLine = false);
			void select_next(HubDraggable const* _from);

			bool is_something_hovering_over(Loader::HubWidgets::GridDraggable const* _element) const;

			int get_placed_element_idx_at(VectorInt2 const& _at, int _afterIdx = NONE) const;
			bool is_available(VectorInt2 const& _at) const;
			bool is_within_grid(VectorInt2 const& _at) const; // within grid

			void be_draggable() { draggable = true; }
			void allow_rotation() { allowRotation = true; }
			void snap_hovering_to_interior() { snapHoveringToInterior = true; }

			void calculate_auto_values();

		public: // HubWidget
			implement_ void advance(Hub* _hub, HubScreen* _screen, float _deltaTime, bool _beyondModal); // sets in hub active widget

			implement_ void render_to_display(Framework::Display* _display);

			implement_ bool internal_on_hold(int _handIdx, bool _beingHeld, Vector2 const & _at);
			implement_ void internal_on_release(int _handIdx, Vector2 const & _at);
			implement_ bool internal_on_hold_grip(int _handIdx, bool _beingHeld, Vector2 const& _at);
			implement_ void internal_on_release_grip(int _handIdx, Vector2 const& _at);
			implement_ void process_input(int _handIdx, Framework::GameInput const& _input, float _deltaTime);

			// drag'n'drop
			implement_ bool get_to_drag(int _handIdx, Vector2 const & _at, OUT_ RefCountObjectPtr<HubDraggable> & _draggable, OUT_ RefCountObjectPtr<HubDraggable> & _placeHolder);
			implement_ void hover(int _handIdx, Vector2 const & _at, HubDraggable* _draggable);
			implement_ void remove_draggable(HubDraggable* _draggable);
			implement_ bool can_drop_hovering(HubDraggable* _draggableDragged) const;
			implement_ HubDraggable* drop_dragged(int _handIdx, Vector2 const & _at, HubDraggable* _draggableDragged, HubDraggable* _draggableToPlace);

			implement_ HubDraggable* tut_get_draggable(TeaForGodEmperor::TutorialHubId const& _id) const;

			implement_ void clean_up();

		protected:
			Framework::UsedLibraryStored<Framework::Sample> moveSample; // will create on fly using reference "loader hub scene mesh generator"

			void load_defaults();
			void load_assets();

			VectorInt2 get_loc(Vector2 const& _at) const; // without scroll

		private:
			bool put_anywhere_sub_at(HubDraggable* _draggable, GridShape const& gs, int x, int y);
		};
	};
};
