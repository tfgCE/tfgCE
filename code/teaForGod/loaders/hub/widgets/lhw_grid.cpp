#include "lhw_grid.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\..\..\library\library.h"
#include "..\..\..\tutorials\tutorialSystem.h"

#include "..\..\..\..\framework\sound\soundSample.h"
#include "..\..\..\..\framework\video\font.h"

#include "..\..\..\..\core\system\core.h"
#include "..\..\..\..\core\system\video\video3dPrimitives.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Loader;

//

// references
DEFINE_STATIC_NAME_STR(loaderHubMove, TXT("loader hub move"));

// system tags
DEFINE_STATIC_NAME(lowGraphics);

//

HubWidgets::GridShape::GridShape()
{
	SET_EXTRA_DEBUG_INFO(shape, TXT("HubWidgets::GridShape.shape"));

	shape.set_size(MAX_SIZE * MAX_SIZE);
	for_every(s, shape)
	{
		*s = false;
	}
	calc_size();
}

HubWidgets::GridShape HubWidgets::GridShape::rect(VectorInt2 const& _size)
{
	GridShape gs;
	for_every(s, gs.shape)
	{
		*s = false;
	}
	for_count(int, y, _size.y)
	{
		for_count(int, x, _size.x)
		{
			gs.at(x, y) = true;
		}
	}
	gs.calc_size();
	return gs;
}

HubWidgets::GridShape& HubWidgets::GridShape::set_from(bool const* _leftBottom, VectorInt2 const& _arraySize)
{
	// clear
	shape.set_size(MAX_SIZE * MAX_SIZE);
	for_every(s, shape)
	{
		*s = false;
	}

	// fill
	for_count(int, y, _arraySize.y)
	{
		if (y < MAX_SIZE)
		{
			for_count(int, x, _arraySize.x)
			{
				if (x < MAX_SIZE)
				{
					at(x, y) = *(_leftBottom + y * _arraySize.x + x);
				}
			}
		}
	}

	calc_size();

	return *this;
}

void HubWidgets::GridShape::calc_size()
{
	size = VectorInt2(1, 1);
	for_count(int, y, MAX_SIZE) 
	{
		for_count(int, x, MAX_SIZE)
		{
			if (get(x, y))
			{
				size.x = max(size.x, x + 1);
				size.y = max(size.y, y + 1);
			}
		}
	}
}

VectorInt2 HubWidgets::GridShape::get_rotated_size(int _rotated) const
{
	VectorInt2 rSize = size;
	if (mod(_rotated, 2) == 1)
	{
		swap(rSize.x, rSize.y);
	}
	return rSize;
}


void HubWidgets::GridShape::rotate(int _rotation)
{
	_rotation = mod(_rotation, 4);
	while (_rotation)
	{
		GridShape prev = *this;
		for_count(int, y, MAX_SIZE)
		{
			for_count(int, x, MAX_SIZE)
			{
				at(x, y) = prev.get(size.x - 1 - y, x);
			}
		}
		--_rotation;
		calc_size();
	}

}

bool HubWidgets::GridShape::move_by(VectorInt2 const & _by)
{
	if (_by.x != 0)
	{
		int minX = MAX_SIZE;
		int maxX = 0;
		for_count(int, y, MAX_SIZE)
		{
			for_count(int, x, MAX_SIZE)
			{
				if (get(x, y))
				{
					maxX = max(x, maxX);
					minX = min(x, minX);
				}
			}
		}
		if (_by.x > 0 && maxX > MAX_SIZE - 1 - _by.x)
		{
			return false;
		}
		if (_by.x < 0 && minX > -_by.x)
		{
			return false;
		}
	}
	if (_by.y != 0)
	{
		int minY = MAX_SIZE;
		int maxY = 0;
		for_count(int, y, MAX_SIZE)
		{
			for_count(int, x, MAX_SIZE)
			{
				if (get(x, y))
				{
					maxY = max(y, maxY);
					minY = min(y, minY);
				}
			}
		}
		if (_by.y > 0 && maxY > MAX_SIZE - 1 - _by.y)
		{
			return false;
		}
		if (_by.y < 0 && minY > -_by.y)
		{
			return false;
		}
	}

	GridShape prev = *this;
	for_count(int, y, MAX_SIZE)
	{
		for_count(int, x, MAX_SIZE)
		{
			at(x, y) = prev.get(x - _by.x, y - _by.y);
		}
	}
	calc_size();

	return true;
}

bool HubWidgets::GridShape::can_add(GridShape const & _shape)
{
	for_count(int, y, MAX_SIZE)
	{
		for_count(int, x, MAX_SIZE)
		{
			if (get(x, y) && _shape.get(x, y))
			{
				return false;
			}
		}
	}
	return true;
}

bool HubWidgets::GridShape::add(GridShape const & _shape)
{
	if (!can_add(_shape))
	{
		return false;
	}
	for_count(int, y, MAX_SIZE)
	{
		for_count(int, x, MAX_SIZE)
		{
			at(x, y) |= _shape.get(x, y);
		}
	}
	calc_size();
	return true;
}

void HubWidgets::GridShape::log(LogInfoContext & _context) const
{
	for_count(int, y, size.y)
	{
		String line;
		for_count(int, x, size.x)
		{
			line += get(x, size.y - 1 - y) ? '#' : '.';
		}
		_context.log(line.to_char());
	}
}

//

void HubWidgets::GridDraggable::update_at(Range2 const& _gridWindow, Vector2 const & _gridScroll, Vector2 const& _elementSize)
{
	if (auto * d = draggable.get())
	{
		Vector2 gridShapeSize = gridShape.get_size().to_vector2();
		if (mod(gridPlacement.rotation, 2) == 1)
		{
			swap(gridShapeSize.x, gridShapeSize.y);
		}

		d->at.x.min = round(_gridWindow.x.min + _gridScroll.x + (float)gridPlacement.at.x * _elementSize.x);
		d->at.x.max = round(d->at.x.min + gridShapeSize.x * _elementSize.x - 1.0f);

		d->at.y.min = round(_gridWindow.y.min + _gridScroll.y + (float)gridPlacement.at.y * _elementSize.y);
		d->at.y.max = round(d->at.y.min + gridShapeSize.y * _elementSize.y - 1.0f);
	}
}

HubWidgets::GridShape HubWidgets::GridDraggable::get_shape_in_place() const
{
	GridShape result = gridShape;

	result.rotate(gridPlacement.rotation);
	result.move_by(gridPlacement.at);

	return result;
}

VectorInt2 HubWidgets::GridDraggable::get_rotated_size() const
{
	VectorInt2 gridShapeSize = gridShape.get_size();
	if (mod(gridPlacement.rotation, 2) == 1)
	{
		swap(gridShapeSize.x, gridShapeSize.y);
	}
	return gridShapeSize;
}

bool HubWidgets::GridDraggable::does_contain(Vector2 const& _at, Vector2 const& _elementSize) const
{
	if (draggable.get() && draggable->at.does_contain(_at))
	{
		VectorInt2 gIdx = get_shape_grid_loc(_at, _elementSize);
		return gridShape.get(gIdx);
	}
	return false;
}

Vector2 HubWidgets::GridDraggable::get_loc(Vector2 const& _at, Vector2 const& _elementSize) const
{
	Vector2 gat = _at;
	gat.x -= draggable->at.x.centre();
	gat.y -= draggable->at.y.centre();
	int rotateClockwise = -gridPlacement.rotation; // - because we rotate to shape's local
	rotateClockwise = mod(rotateClockwise, 4);
	while (rotateClockwise > 0)
	{
		--rotateClockwise;
		swap(gat.x, gat.y);
		gat.y *= -1.0f;
	}

	return gat;
}

VectorInt2 HubWidgets::GridDraggable::get_shape_grid_loc(Vector2 const& _at, Vector2 const& _elementSize) const
{
	Vector2 gat = _at;

	an_assert(draggable.get());

	gat.x -= draggable->at.x.min;
	gat.y -= draggable->at.y.min;

	gat.x = floor(gat.x / _elementSize.x);
	gat.y = floor(gat.y / _elementSize.y);

	VectorInt2 gIdx = (gat + Vector2(0.1f, 0.1f)).to_vector_int2_cells() + gridPlacement.at;
	return get_shape_grid_loc(gIdx);
}

VectorInt2 HubWidgets::GridDraggable::get_shape_grid_loc(VectorInt2 const& _absAt) const
{
	VectorInt2 gIdx = _absAt - gridPlacement.at;

	int rotation = gridPlacement.rotation; // - because we rotate to shape's local
	rotation = mod(rotation, 4);
	VectorInt2 shapeSize = gridShape.get_size(); // grid shape is as it is, not rotated!
	if (rotation == 1)
	{
		gIdx = VectorInt2(shapeSize.x - 1 - gIdx.y, gIdx.x);
	}
	if (rotation == 2)
	{
		gIdx = VectorInt2(shapeSize.x - 1 - gIdx.x, shapeSize.y - 1 - gIdx.y);
	}
	if (rotation == 3)
	{
		gIdx = VectorInt2(gIdx.y, shapeSize.y - 1 - gIdx.x);
	}

	return gIdx;
}

//

REGISTER_FOR_FAST_CAST(HubWidgets::Grid);

void HubWidgets::Grid::load_defaults()
{
	scroll.autoHideScroll = false;
	scroll.scrollType = InnerScroll::NoScroll;
}

void HubWidgets::Grid::load_assets()
{
	if (auto* library = Framework::Library::get_current())
	{
		moveSample.set(library->get_global_references().get<Framework::Sample>(NAME(loaderHubMove)));
	}
}

void HubWidgets::Grid::advance(Hub* _hub, HubScreen* _screen, float _deltaTime, bool _beyondModal)
{
	base::advance(_hub, _screen, _deltaTime, _beyondModal);

	float activePrev = active;

	activeTarget = 0.0f;
	for_count(int, hIdx, 2)
	{
		auto & hand = _hub->access_hand((::Hand::Type)hIdx);
		if (hand.overWidget == this)
		{
			activeTarget = hand.prevOverWidget == this ? 1.0f : 0.0f;
		}
	}
	active = blend_to_using_time(active, activeTarget, 0.1f, _deltaTime);

	if (activePrev != active && abs(active - activeTarget) > 0.005f)
	{
		mark_requires_rendering();
	}

	calculate_auto_values();

	for_every_ref(gd, elements)
	{
		if (auto * d = gd->draggable.get())
		{
			d->activeTarget = 0.0f;
			if (d->heldByHand.is_set())
			{
				d->activeTarget = 0.7f;
			}
			else if (activeTarget > 0.0f)
			{
				for_count(int, hIdx, 2)
				{
					auto& hand = _hub->access_hand((::Hand::Type)hIdx);
					if (hand.onScreenTarget == _screen)
					{
						if (gd->does_contain(hand.onScreenTargetAt, elementSize))
						{
							d->activeTarget = 1.0f;
						}
					}
				}
			}
			d->active = blend_to_using_time(d->active, d->activeTarget, 0.1f, _deltaTime);
		}
	}
}

void HubWidgets::Grid::render_to_display(Framework::Display* _display)
{
	float minAlpha = 0.0f;
	if (::System::Core::get_system_tags().get_tag(NAME(lowGraphics)))
	{
		minAlpha = 0.6f;
	}

	calculate_auto_values();

	if (scroll.scrollType != InnerScroll::NoScroll)
	{
		Colour border = Colour::lerp(active, HubColours::border()/*.with_alpha(max(minAlpha, 0.8f))*/, HubColours::text());
		Colour inside = Colour::lerp(active, HubColours::widget_background()/*.with_alpha(max(minAlpha, 0.3f))*/, HubColours::highlight()/*.with_alpha(max(minAlpha, 0.8f))*/);

		border = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(border, screen, this);
		inside = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(border, screen, this);

		if (specialHighlight)
		{
			border = HubColours::special_highlight().with_set_alpha(1.0f);
			inside = HubColours::special_highlight_background_for(inside);
		}

		scroll.render_to_display(_display, at, border, inside);

		scroll.push_clip_plane_stack();
	}

	for_every_ref(element, elements)
	{
		element->update_at(gridWindow, gridScroll, elementSize);
	}

	if (draw_whole_grid)
	{
		draw_whole_grid(_display, gridWindow);
	}

	if (draw_grid_element)
	{
		for_count(int, y, gridSize.y)
		{
			for_count(int, x, gridSize.x)
			{
				{
					Range2 elementAt = gridWindow;
					elementAt.x.min += (float)x * elementSize.x + gridScroll.x;
					elementAt.y.min += (float)y * elementSize.y + gridScroll.y;
					elementAt.x.max = elementAt.x.min + elementSize.x - 1.0f;
					elementAt.y.max = elementAt.y.min + elementSize.y - 1.0f;
					draw_grid_element(_display, VectorInt2(x, y), elementAt, is_available(VectorInt2(x, y)));
				}
			}
		}
	}

	// first draw those that are inside
	// then draw those that are hovering
	for_count(int, pass, 2)
	{
		for_every_ref(element, elements)
		{
			if (!element->draggable.get() || !element->draggable->is_visible_in(this))
			{
				continue;
			}
			if ((pass == 0 && element->draggable->hoveringOver != nullptr) ||
				(pass == 1 && element->draggable->hoveringOver != this))
			{
				continue;
			}
			Range2 const& elAt = element->draggable->at;
			if (elAt.intersects(at))
			{
				if (draw_element)
				{
					draw_element(_display, elAt, element);
				}
			}
		}
	}

	if (draw_whole_grid_post)
	{
		draw_whole_grid_post(_display, gridWindow);
	}

	if (scroll.scrollType != InnerScroll::NoScroll)
	{
		scroll.pop_clip_plane_stack();
	}

	base::render_to_display(_display);
}

void HubWidgets::Grid::calculate_auto_values()
{
	scroll.calculate_auto_space_available(at);

	if (!autoElementSizeBasedOnGridSize.is_zero())
	{
		elementSize.x = floor((scroll.spaceAvailable.x + 0.01f) / (float)autoElementSizeBasedOnGridSize.x);
		elementSize.y = floor((scroll.spaceAvailable.y + 0.01f) / (float)autoElementSizeBasedOnGridSize.y);
	}

	if (forcedGridSize.is_set())
	{
		gridSize = forcedGridSize.get();
		visibleGridSize = gridSize;
	}
	else
	{
		gridSize.x = (int)(floor(scroll.spaceAvailable.x / elementSize.x) + 0.01f);
		gridSize.y = (int)(floor(scroll.spaceAvailable.y / elementSize.y) + 0.01f);
		visibleGridSize = gridSize;

		VectorInt2 extra = VectorInt2::zero;
		if (draggable) // if not draggable, what's the point of having an extra line?
		{
			if (scroll.scrollType == InnerScroll::HorizontalScroll) { ++extra.x; }
			if (scroll.scrollType == InnerScroll::VerticalScroll) { ++extra.y; }
		}

		for_every_ref(e, elements)
		{
			if (e->draggable->hoveringOver)
			{
				continue;
			}
			VectorInt2 dist = e->gridPlacement.at + e->gridShape.get_rotated_size(e->gridPlacement.rotation) - VectorInt2::one;
			gridSize.x = max(gridSize.x, dist.x + 1 + extra.x);
			gridSize.y = max(gridSize.y, dist.y + 1 + extra.y);
		}
	}

	wholeGridSpace = Vector2(gridSize.x * elementSize.x,
							 gridSize.y * elementSize.y);

	scroll.calculate_other_auto_variables(at, wholeGridSpace);

	if (scroll.scrollType != InnerScroll::NoScroll)
	{
		gridWindow = scroll.viewWindow;
		gridScroll = -scroll.scroll;
	}
	else
	{
		gridWindow = at;
		gridScroll = Vector2::zero;

		// centre!
		gridWindow.x.min += floor(max(0.0f, (gridWindow.x.length() - wholeGridSpace.x) * 0.5f));
		gridWindow.y.min += floor(max(0.0f, (gridWindow.y.length() - wholeGridSpace.y) * 0.5f));

		// actual size
		gridWindow.x.max = gridWindow.x.min + wholeGridSpace.x - 1.0f;
		gridWindow.y.max = gridWindow.y.min + wholeGridSpace.y - 1.0f;
	}
}

bool HubWidgets::Grid::internal_on_hold(int _handIdx, bool _beingHeld, Vector2 const& _at)
{
	bool result;
	if (scroll.handle_internal_on_hold(_handIdx, false, _beingHeld, _at, at, elementSize, OUT_ result))
	{
		return result;
	}

	return base::internal_on_hold(_handIdx, _beingHeld, _at);
}

void HubWidgets::Grid::internal_on_release(int _handIdx, Vector2 const& _at)
{
	scroll.handle_internal_on_release(_handIdx, false);
}

bool HubWidgets::Grid::internal_on_hold_grip(int _handIdx, bool _beingHeld, Vector2 const& _at)
{
	bool result;
	if (scroll.handle_internal_on_hold(_handIdx, true, _beingHeld, _at, at, elementSize, OUT_ result))
	{
		return result;
	}

	return base::internal_on_hold_grip(_handIdx, _beingHeld, _at);
}

void HubWidgets::Grid::internal_on_release_grip(int _handIdx, Vector2 const& _at)
{
	scroll.handle_internal_on_release(_handIdx, true);
}

void HubWidgets::Grid::process_input(int _handIdx, Framework::GameInput const& _input, float _deltaTime)
{
	scroll.handle_process_input(_handIdx, _input, _deltaTime, elementSize, screen->pixelsPerAngle);
}

void HubWidgets::Grid::clear()
{
	mark_requires_rendering();
	elements.clear();
}

HubWidgets::GridDraggable* HubWidgets::Grid::add(IHubDraggableData* _data, GridShape const& _shape, GridPlacement const& _gridPlacement)
{
	mark_requires_rendering();
	RefCountObjectPtr<GridDraggable> gridDraggable;
	gridDraggable = new GridDraggable(new HubDraggable());
	HubDraggable* draggable = gridDraggable->draggable.get();
	draggable->from = this;
	draggable->data = _data;
	gridDraggable->gridShape = _shape;
	gridDraggable->gridPlacement = _gridPlacement;
	elements.push_back(gridDraggable);
	return gridDraggable.get();
}

HubWidgets::GridDraggable* HubWidgets::Grid::add_anywhere(IHubDraggableData* _data, GridShape const& _shape, bool _verticalFirst, bool _startFromTop)
{
	mark_requires_rendering();
	RefCountObjectPtr<GridDraggable> gridDraggable;
	gridDraggable = new GridDraggable(new HubDraggable());
	HubDraggable* draggable = gridDraggable->draggable.get();
	draggable->from = this;
	draggable->data = _data;
	gridDraggable->gridShape = _shape;
	put_anywhere(gridDraggable->draggable.get(), _verticalFirst, _startFromTop, _shape);
	return gridDraggable.get();
}

// this works in pair with initialUpAngle in draggable to always have the same rotation and rotate from there
struct InitialGrabLookup
{
	int rotation = 0;
	Vector2 offset = Vector2::zero;
	HubDraggable* forDraggable = nullptr;
};
InitialGrabLookup initialGrabLookup[2];

bool HubWidgets::Grid::get_to_drag(int _handIdx, Vector2 const & _at, OUT_ RefCountObjectPtr<HubDraggable> & _draggable, OUT_ RefCountObjectPtr<HubDraggable> & _placeHolder)
{
	for_every_ref(gelement, elements)
	{
		if (auto * element = gelement->draggable.get())
		{
			if (!element->is_visible_in(this)) continue;
			if (!TeaForGodEmperor::TutorialSystem::can_be_dragged(screen, this, element)) continue;
			if (can_drag && !can_drag(element)) continue;
			if (gelement->does_contain(_at, elementSize) &&
				!element->heldByHand.is_set())
			{
				if (!draggable)
				{
					hub->select(this, element);
					return false;
				}
				_draggable = element;
				initialGrabLookup[_handIdx].rotation = gelement->gridPlacement.rotation;
				initialGrabLookup[_handIdx].offset = -gelement->get_loc(_at, elementSize);
				initialGrabLookup[_handIdx].forDraggable = _draggable.get();
				if ((!can_drop_discard || !can_drop_discard(element)) ||
					(should_stay_on_drag && should_stay_on_drag(element)))
				{
					GridDraggable* placeHolder = add(element->data.get(), gelement->gridShape, gelement->gridPlacement);
					placeHolder->draggable->placeHolder = true;
					placeHolder->draggable->forceVisible = false;
					_placeHolder = placeHolder->draggable.get();
				}
				return true;
			}
		}
	}
	return false;
}

void HubWidgets::Grid::hover(int _handIdx, Vector2 const & _at, HubDraggable* _draggable)
{
	int atIdx = NONE;
	for_every_ref(element, elements)
	{
		if (_draggable == element->draggable.get())
		{
			atIdx = for_everys_index(element);
			break;
		}
	}

	bool justAdded = false;
	if (atIdx == NONE)
	{
		GridShape shape;
		if (get_shape_info_for)
		{
			shape = get_shape_info_for(_draggable);
		}
		else
		{
			shape = GridShape::rect(VectorInt2::one);
		}
		if (!shape.get_size().is_zero())
		{
			// add it
			GridDraggable* newDraggable = new GridDraggable(_draggable);
			newDraggable->gridShape = shape;
			if (initialGrabLookup[_handIdx].forDraggable == _draggable)
			{
				newDraggable->gridPlacement.rotation = initialGrabLookup[_handIdx].rotation;
			}
			newDraggable->upAngleRef = _draggable->initialUpAngle;
			elements.push_back(RefCountObjectPtr<GridDraggable>(newDraggable));
			atIdx = elements.get_size() - 1;
			justAdded = true;
		}
	}

	if (atIdx != NONE)
	{
		auto* element = elements[atIdx].get();
		{
			bool silentRotate = false;
			float cursorUpAngle = hub->get_hand(Hand::Type(_handIdx)).onScreenUpAngle;
			if (!element->draggable->hoveringOver)
			{
				element->upAngleRef = _draggable->initialUpAngle;
				silentRotate = true;
			}
			{
				while (true)
				{
					float diffAngle = Rotator3::normalise_axis(cursorUpAngle - element->upAngleRef);
					if (abs(diffAngle) > 50.0f)
					{
						if (allowRotation)
						{
							if (diffAngle > 0.0f)
							{
								element->upAngleRef += 90.0f;
								element->gridPlacement.rotation += 1;
							}
							else
							{
								element->upAngleRef -= 90.0f;
								element->gridPlacement.rotation -= 1;
							}
						}
						if (!silentRotate)
						{
							play(moveSample);
						}
					}
					else
					{
						break;
					}
				}
				element->upAngleRef = Rotator3::normalise_axis(element->upAngleRef);
				element->gridPlacement.rotation = mod(element->gridPlacement.rotation, 4);
			}
		}
		Vector2 sizeAtGrid = element->get_rotated_size().to_vector2();
		Vector2 leftBottomAt = _at
			+ initialGrabLookup[_handIdx].offset.rotated_by_angle(90.0f * (float)element->gridPlacement.rotation)
			- sizeAtGrid * 0.5f * elementSize // move to actual left bottom
			+ 0.5f * elementSize; // and now to centre of that piece
		VectorInt2 leftBottomLoc = get_loc(leftBottomAt);
		if (element->gridPlacement.at != leftBottomLoc || justAdded)
		{
			element->gridPlacement.at = leftBottomLoc;
		}
		element->validPlacement = false;
		element->outOfBounds = false;
		element->hoveringOverOtherElements = false;
		if (snapHoveringToInterior)
		{
			element->gridPlacement.at.x = clamp(element->gridPlacement.at.x, 0, gridSize.x - (int)sizeAtGrid.x);
			element->gridPlacement.at.y = clamp(element->gridPlacement.at.y, 0, gridSize.y - (int)sizeAtGrid.y);
		}
		if (justAdded || element->gridPlacement.at != element->gridPlacement.sampleAt)
		{
			element->gridPlacement.sampleAt = element->gridPlacement.at;
			play(moveSample);
		}
		if (element->gridPlacement.at.x >= 0 &&
			element->gridPlacement.at.y >= 0 &&
			element->gridPlacement.at.x <= gridSize.x - (int)sizeAtGrid.x &&
			element->gridPlacement.at.y <= gridSize.y - (int)sizeAtGrid.y)
		{
			// is within grid, check if overlaps
			GridShape shapeInPlace = element->get_shape_in_place();
			element->validPlacement = true;
			for_count(int, y, shapeInPlace.get_size().y)
			{
				if (element->validPlacement)
				{
					for_count(int, x, shapeInPlace.get_size().x)
					{
						if (shapeInPlace.get(x, y))
						{
							if (!is_available(VectorInt2(x, y)))
							{
								element->validPlacement = false;
								element->hoveringOverOtherElements = true;
								break;
							}
						}
					}
				}
			}
		}
		else
		{
			element->outOfBounds = true;
		}

		if (auto * d = element->draggable.get())
		{
			d->hoveringOver = this;
		}
	}
	else
	{
		remove_draggable(_draggable);
		play(moveSample);
	}
}

void HubWidgets::Grid::remove_draggable(HubDraggable* _draggable)
{
	_draggable->hoveringOver = nullptr;
	int atIdx = NONE;
	for_every_ref(element, elements)
	{
		if (_draggable == element->draggable.get())
		{
			atIdx = for_everys_index(element);
			break;
		}
	}
	if (atIdx != NONE)
	{
		elements.remove_at(atIdx);
	}
	mark_requires_rendering();
}

bool HubWidgets::Grid::can_drop_hovering(HubDraggable* _draggableDragged) const
{
	for_every_ref(element, elements)
	{
		if (_draggableDragged == element->draggable.get())
		{
			return element->validPlacement || (! element->outOfBounds && allowReplacing && (on_drop_discard || on_drop_replace));
		}
	}
	return false;
}

HubDraggable* HubWidgets::Grid::drop_dragged(int _handIdx, Vector2 const & _at, HubDraggable* _draggableDragged, HubDraggable* _draggableToPlace)
{
	HubDraggable* placed = nullptr;
	if (allowReplacing && (on_drop_discard || on_drop_replace))
	{
		GridDraggable* element = nullptr;
		for_every_ref(e, elements)
		{
			if (_draggableDragged == e->draggable.get())
			{
				element = e;
			}
		}
		
		GridShape shapeInPlace = element->get_shape_in_place();

		// check if we overlap and discard all overlapping
		for_count(int, y, shapeInPlace.get_size().y)
		{
			for_count(int, x, shapeInPlace.get_size().x)
			{
				if (shapeInPlace.get(x, y))
				{
					if (!is_available(VectorInt2(x, y)))
					{
						int idx = NONE;
						while (true)
						{
							idx = get_placed_element_idx_at(VectorInt2(x, y), idx);
							if (idx != NONE)
							{
								if (elements[idx] != element)
								{
									auto* d = elements[idx]->draggable.get();
									if (d->is_placed())
									{
										if (on_drop_replace)
										{
											on_drop_replace(d, element->draggable.get());
										}
										else
										{
											on_drag_begin(d);
											on_drop_discard(d);
										}
									}
									elements.remove_at(idx);
									--idx;
								}
							}
							else
							{
								break;
							}
						}
					}
				}
			}
		}
#ifdef AN_DEVELOPMENT_OR_PROFILER
		bool okToDrop = true;
		for_count(int, y, shapeInPlace.get_size().y)
		{
			for_count(int, x, shapeInPlace.get_size().x)
			{
				if (shapeInPlace.get(x, y))
				{
					if (!is_available(VectorInt2(x, y)))
					{
						okToDrop = false;
					}
				}
			}
		}
		an_assert(okToDrop);
#endif
	}
	// we're hovering here, just do a copy
	int atIdx = NONE;
	for_every_ref(element, elements)
	{
		if (_draggableDragged == element->draggable.get())
		{
			atIdx = for_everys_index(element);
			break;
		}
	}
	an_assert(atIdx != NONE);
	if (atIdx != NONE)
	{
		float wasActive = elements[atIdx]->draggable->active;
		RefCountObjectPtr<IHubDraggableData> keepData;
		keepData = _draggableToPlace? _draggableToPlace->data : _draggableDragged->data;
		// store as we're about to remove it
		GridShape gridShape = elements[atIdx]->gridShape;
		GridPlacement gridPlacement = elements[atIdx]->gridPlacement;
		elements.remove_at(atIdx);
		// now use stored values to add a new one
		GridDraggable* newDraggable = add(keepData.get(), gridShape, gridPlacement);
		newDraggable->draggable->active = wasActive;
		placed = newDraggable->draggable.get();
	}

	play(moveSample);

	return placed;
}

bool HubWidgets::Grid::is_something_hovering_over(Loader::HubWidgets::GridDraggable const* _element) const
{
	GridShape shapeInPlace = _element->get_shape_in_place();
	for_every_ref(element, elements)
	{
		if (element->hoveringOverOtherElements && element != _element)
		{
			GridShape hoveringShapeInPlace = element->get_shape_in_place();
			for_count(int, y, shapeInPlace.get_size().y)
			{
				for_count(int, x, shapeInPlace.get_size().x)
				{
					if (shapeInPlace.get(x, y) && hoveringShapeInPlace.get(x, y))
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool HubWidgets::Grid::is_within_grid(VectorInt2 const& _at) const
{
	return _at.x >= 0 && _at.y >= 0 && _at.x < gridSize.x && _at.y < gridSize.y;
}

int HubWidgets::Grid::get_placed_element_idx_at(VectorInt2 const& _at, int _afterIdx) const
{
	if (is_within_grid(_at))
	{
		for_every_ref(element, elements)
		{
			if (for_everys_index(element) <= _afterIdx)
			{
				continue;
			}
			if (element->draggable->is_placed()) 
			{
				VectorInt2 atLoc = element->get_shape_grid_loc(_at);
				if (element->gridShape.get(atLoc))
				{
					return for_everys_index(element);
				}
			}
		}
		return NONE;
	}
	return NONE;
}

bool HubWidgets::Grid::is_available(VectorInt2 const& _at) const
{
	if (is_within_grid(_at))
	{
		for_every_ref(element, elements)
		{
			if (element->draggable->is_placed())
			{
				VectorInt2 atLoc = element->get_shape_grid_loc(_at);
				if (element->gridShape.get(atLoc))
				{
					return false;
				}
			}
		}
		return true;
	}
	return false;
}

VectorInt2 HubWidgets::Grid::get_loc(Vector2 const& _at) const
{
	Vector2 gat = _at - gridScroll;
	gat.x -= gridWindow.x.min;
	gat.y -= gridWindow.y.min;

	gat.x = gat.x / elementSize.x;
	gat.y = gat.y / elementSize.y;

	VectorInt2 gIdx = gat.to_vector_int2_cells();

	return gIdx;
}

void HubWidgets::Grid::put_anywhere(HubDraggable* _draggable, bool _verticalFirst, bool _startFromTop, Optional<GridShape> const& _shape)
{
	if (get_shape_info_for || _shape.is_set())
	{
		GridShape gs;
		if (_shape.is_set())
		{
			gs = _shape.get();
		}
		else
		{
			an_assert(get_shape_info_for);
			gs = get_shape_info_for(_draggable);
		}
		if (_verticalFirst)
		{
			int x = 0;
			while (true)
			{
				for_count(int, yy, gridSize.y)
				{
					int y = _startFromTop ? gridSize.y - 1 - yy : yy;
					if (put_anywhere_sub_at(_draggable, gs, x, y))
					{
						return;
					}
				}
				++x;

				if (scroll.scrollType == InnerScroll::NoScroll && x == gridSize.x)
				{
					break;
				}
			}
		}
		else
		{
			int iy = 0;
			while (true)
			{
				int y = _startFromTop ? gridSize.y - 1 - iy : iy;

				for_count(int, x, gridSize.x)
				{
					if (put_anywhere_sub_at(_draggable, gs, x, y))
					{
						return;
					}
				}
				++iy;

				if (scroll.scrollType == InnerScroll::NoScroll && iy == gridSize.y)
				{
					break;
				}
			}
		}
	}
	else
	{
		error(TXT("provide a shape, directly or via method"));
	}
}

bool HubWidgets::Grid::put_anywhere_sub_at(HubDraggable * _draggable, GridShape const & gs, int x, int y)
{
	for_count(int, r, allowRotation? 4 : 1)
	{
		VectorInt2 sizeAtGrid = gs.get_size();
		if (mod(r, 2) == 1)
		{
			swap(sizeAtGrid.x, sizeAtGrid.y);
		}

		GridPlacement gp;
		gp.at.x = x;
		gp.at.y = y;
		gp.rotation = r;

		if (gp.at.x >= 0 &&
			gp.at.y >= 0 &&
			gp.at.x <= gridSize.x - sizeAtGrid.x &&
			gp.at.y <= gridSize.y - sizeAtGrid.y)
		{
			// is within grid, check if overlaps
			GridShape shapeInPlace = gs;

			shapeInPlace.rotate(gp.rotation);
			shapeInPlace.move_by(gp.at);

			bool isOk = true;
			for_count(int, y, shapeInPlace.get_size().y)
			{
				for_count(int, x, shapeInPlace.get_size().x)
				{
					if (shapeInPlace.get(x, y))
					{
						if (!is_available(VectorInt2(x, y)))
						{
							isOk = false;
							break;
						}
					}
				}
			}

			if (isOk)
			{
				RefCountObjectPtr<HubDraggable> dropAs;
				dropAs = _draggable;
				if (on_drop_change)
				{
					dropAs = on_drop_change(_draggable);
				}
				add(dropAs->data.get(), gs, gp);
				if (_draggable->from)
				{
					if (!_draggable->from->should_stay_on_drag ||
						!_draggable->from->should_stay_on_drag(_draggable))
					{
						_draggable->from->remove_draggable(_draggable);
					}
				}
				return true;
			}
		}
	}
	return false;
}

void HubWidgets::Grid::discard_all()
{
	mark_requires_rendering();
	if (on_drop_discard)
	{
		for_every_ref(element, elements)
		{
			if (element->draggable.get())
			{
				on_drop_discard(element->draggable.get());
			}
		}
	}
	elements.clear();
}

HubWidgets::GridDraggable* HubWidgets::Grid::get_at(Vector2 const& _at)
{
	for_every_ref(element, elements)
	{
		if (element->does_contain(_at, elementSize))
		{
			return element;
		}
	}
	return nullptr;
}

HubDraggable* HubWidgets::Grid::tut_get_draggable(TeaForGodEmperor::TutorialHubId const& _id) const
{
	if (!_id.is_set())
	{
		return nullptr;
	}

	for_every_ref(element, elements)
	{
		if (auto* d = element->draggable.get())
		{
			if (auto* hd = d->data.get())
			{
				if (hd->tutorialHubId == _id)
				{
					return d;
				}
			}
		}
	}

	return nullptr;
}

void HubWidgets::Grid::select_next(HubDraggable const* _from)
{
	for_every_ref(element, elements)
	{
		if (auto* d = element->draggable.get())
		{
			if (d == _from)
			{
				hub->select(this, elements[(for_everys_index(element) + 1) % elements.get_size()]->draggable.get());
				return;
			}
		}
	}

	if (!elements.is_empty())
	{
		hub->select(this, elements.get_first()->draggable.get());
	}
}

void HubWidgets::Grid::select_in_dir(HubDraggable const* _from, VectorInt2 const& _inDir, bool _keepStraightLine)
{
	GridDraggable* from = nullptr;
	Optional<VectorInt2> at;
	for_every_ref(element, elements)
	{
		if (auto* d = element->draggable.get())
		{
			if (d == _from)
			{
				from = element;
				at = element->gridPlacement.at;
			}
		}
	}

	if (!from)
	{
		HubWidgets::GridDraggable * best = nullptr;
		float bestDist = 0.0f;
		Vector2 dir = _inDir.to_vector2();
		for_every_ref(element, elements)
		{
			float dist = Vector2::dot(dir, element->gridPlacement.at.to_vector2());
			if (dist < bestDist || !best)
			{
				bestDist = dist;
				best = element;
			}
		}
		if (best)
		{
			hub->select(this, best->draggable.get());
			return;
		}
	}

	if (at.is_set() && from)
	{
		Vector2 atv = at.get().to_vector2();
		Vector2 size = from->gridShape.get_rotated_size(from->gridPlacement.rotation).to_vector2();
		Vector2 postMove = atv + _inDir.to_vector2() * (size * 0.5f + Vector2::half);

		VectorInt2 gPostMove = TypeConversions::Normal::f_i_cells(postMove + Vector2(0.5f));

		// find closest in that dir
		HubWidgets::GridDraggable const* closest = nullptr;
		float bestDist = 0.0f;
		for_every_ref(e, elements)
		{
			if (e->gridPlacement.at == at.get())
			{
				continue;
			}
			VectorInt2 gLoc = e->get_shape_grid_loc(gPostMove);
			if (e->gridShape.get(gLoc))
			{
				closest = e;
				bestDist = 0.0f;
				break;
			}
			Vector2 esize = e->gridShape.get_rotated_size(e->gridPlacement.rotation).to_vector2();
			Vector2 eat = e->gridPlacement.at.to_vector2() + (esize - Vector2::one) * 0.5f;
			Vector2 toEat = eat - postMove;
			if (Vector2::dot(eat - atv, _inDir.to_vector2()) > 0.0f)
			{
				float dist = (toEat).length();
				if (_keepStraightLine)
				{
					dist = Vector2::dot(_inDir.to_vector2(), toEat) + 10.0f * abs(Vector2::dot(_inDir.to_vector2().rotated_right(), toEat));
				}
				if (dist < bestDist || !closest)
				{
					closest = e;
					bestDist = dist;
				}
			}
		}

		if (closest)
		{
			hub->select(this, closest->draggable.get());
		}
	}
}

void HubWidgets::Grid::clean_up()
{
	base::clean_up();
	get_shape_info_for = nullptr;
	draw_element = nullptr;
	draw_whole_grid = nullptr;
	draw_grid_element = nullptr;
	draw_whole_grid_post = nullptr;
}

