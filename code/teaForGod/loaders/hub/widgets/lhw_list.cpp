#include "lhw_list.h"

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

REGISTER_FOR_FAST_CAST(HubWidgets::List);

HubWidgets::List::List()
{
	load_defaults();
	load_assets();
}

HubWidgets::List::List(Name const& _id, Range2 const& _at)
: base(_id, _at)
{
	load_defaults();
	load_assets();
}

void HubWidgets::List::load_defaults()
{
	scroll.autoHideScroll = false;
	scroll.scrollType = InnerScroll::VerticalScroll;
}

void HubWidgets::List::load_assets()
{
	if (auto* library = Framework::Library::get_current())
	{
		moveSample.set(library->get_global_references().get<Framework::Sample>(NAME(loaderHubMove)));
	}
}

void HubWidgets::List::advance(Hub* _hub, HubScreen* _screen, float _deltaTime, bool _beyondModal)
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

	for_every_ref(d, elements)
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
				if (scroll.scrollHeld[hIdx].held || scroll.scrollHeldGrip[hIdx].held) continue;
				auto & hand = _hub->access_hand((::Hand::Type)hIdx);
				if (hand.onScreenTarget == _screen &&
					scroll.viewWindow.does_contain(hand.onScreenTargetAt))
				{
					if (d->at.does_contain(hand.onScreenTargetAt))
					{
						d->activeTarget = 1.0f;
					}
				}
			}
		}
		d->active = blend_to_using_time(d->active, d->activeTarget, 0.1f, _deltaTime);
	}
}

void HubWidgets::List::scroll_to_centre(HubDraggable* _draggable)
{
	calculate_auto_values();

	for_every_ref(d, elements)
	{
		if (d == _draggable)
		{
			scroll.scroll += Vector2(d->at.x.centre() - at.x.centre(), d->at.y.centre() - at.y.centre());
			scroll.make_sane();
			break;
		}
	}
}

void HubWidgets::List::scroll_to_make_visible(HubDraggable* _draggable)
{
	calculate_auto_values();

	for_every_ref(d, elements)
	{
		if (d == _draggable)
		{
			scroll.scroll.x += max(0.0f, d->at.x.max - at.x.max);
			scroll.scroll.x += min(0.0f, d->at.x.min - at.x.min);
			scroll.scroll.y += max(0.0f, d->at.y.max - at.y.max);
			scroll.scroll.y += min(0.0f, d->at.y.min - at.y.min);
			scroll.make_sane();
			break;
		}
	}

	mark_requires_rendering();
}

HubDraggable* HubWidgets::List::change_selection_by(int _by)
{
	for_every_ref(element, elements)
	{
		if (hub->is_selected(this, element))
		{
			int newIdx = for_everys_index(element) + _by;
			newIdx = clamp(newIdx, 0, elements.get_size() - 1);
			auto* newSelected = elements[newIdx].get();
			hub->select(this, newSelected);
			mark_requires_rendering();
			return newSelected;
		}
	}

	if (!elements.is_empty())
	{
		hub->select(this, elements.get_first().get());
		mark_requires_rendering();
		return change_selection_by(_by);
	}
	return nullptr;
}

void HubWidgets::List::render_to_display(Framework::Display* _display)
{
	float minAlpha = 0.0f;
	if (::System::Core::get_system_tags().get_tag(NAME(lowGraphics)))
	{
		minAlpha = 0.6f;
	}
	Colour border = Colour::lerp(active, HubColours::border()/*.with_alpha(max(minAlpha, 0.8f))*/, HubColours::text());
	Colour inside = Colour::lerp(active, HubColours::widget_background()/*.with_alpha(max(minAlpha, 0.3f))*/, HubColours::highlight()/*.with_alpha(max(minAlpha, 0.8f))*/);

	border = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(border, screen, this);
	inside = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(inside, screen, this);

	if (specialHighlight)
	{
		border = HubColours::special_highlight().with_set_alpha(1.0f);
		inside = HubColours::special_highlight_background_for(inside);
	}

	calculate_auto_values();

	scroll.render_to_display(_display, at, border, inside);

	scroll.push_clip_plane_stack();

	for_every_ref(element, elements)
	{
		if (!is_visible(element))
		{
			continue;
		}
		Range2 const & elAt = element->at;
		if (elAt.intersects(at))
		{
			if (drawElementBorder || ! draw_element)
			{
				bool asSelected = drawSelectedColours && hub->is_selected(this, element);
					
				draw_element_border(_display, elAt, element, asSelected);
			}
			if (draw_element)
			{
				draw_element(_display, elAt, element);
			}
		}
	}

	scroll.pop_clip_plane_stack();

	base::render_to_display(_display);
}

void HubWidgets::List::draw_element_border(Framework::Display* _display, Range2 const& _at, HubDraggable const* _element, bool _asSelected)
{
	Colour border = Colour::lerp(_element->active, (_asSelected ? HubColours::selected_highlighted() : HubColours::border())/*.with_alpha(max(minAlpha, 0.8f))*/, _asSelected ? HubColours::selected_highlighted() : HubColours::text());
	Colour inside = Colour::lerp(_element->active, HubColours::widget_background()/*.with_alpha(max(minAlpha, _asSelected? 0.35f : 0.25f))*/, HubColours::highlight()/*.with_alpha(max(minAlpha, _asSelected ? 0.85f : 0.75f))*/);

	border = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(border, screen, this);
	inside = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(inside, screen, this);

	::System::Video3DPrimitives::fill_rect_2d(border, _at, false);
	::System::Video3DPrimitives::fill_rect_2d(inside, _at.expanded_by(-Vector2(2.0f)), false);
}

void HubWidgets::List::calculate_auto_values()
{
	scroll.calculate_auto_space_available(at);

	if (horizontal)
	{
		gridSize.y = (int)(floor(scroll.spaceAvailable.y / elementSize.y) + 0.01f);
		if (!gridMode)
		{
			elementSize.y = scroll.spaceAvailable.y;
			gridSize.y = 1;
		}
		gridSize.x = (int)(floor(scroll.spaceAvailable.x / elementSize.x) + 0.01f);
	}
	else
	{
		gridSize.x = (int)(floor(scroll.spaceAvailable.x / elementSize.x) + 0.01f);
		if (!gridMode)
		{
			elementSize.x = scroll.spaceAvailable.x;
			gridSize.x = 1;
		}
		gridSize.y = (int)(floor(scroll.spaceAvailable.y / elementSize.y) + 0.01f);
	}

	if (elementSpacing.is_set())
	{
		actualElementSpacing = elementSpacing.get();
	}
	else
	{
		if (horizontal || scroll.scrollType == InnerScroll::HorizontalScroll)
		{
			if (gridSize.y > 1)
			{
				actualElementSpacing.y = (scroll.spaceAvailable.y - (float)gridSize.y * elementSize.y);
				actualElementSpacing.y /= (float)gridSize.y;
				actualElementSpacing.x = actualElementSpacing.y;
			}
			else
			{
				actualElementSpacing.y = 0.0f;
				actualElementSpacing.x = max(2.0f, round(elementSize.x * 0.2f));
			}
		}
		else
		{
			if (gridSize.x > 1)
			{
				actualElementSpacing.x = (scroll.spaceAvailable.x - (float)gridSize.x * elementSize.x);
				actualElementSpacing.x /= (float)gridSize.x;
				actualElementSpacing.y = actualElementSpacing.x;
			}
			else
			{
				actualElementSpacing.x = 0.0f;
				actualElementSpacing.y = max(2.0f, round(elementSize.y * 0.2f));
			}
		}
	}

	{
		int elementsCount = 0;
		for_every_ref(element, elements)
		{
			if (is_visible(element))
			{
				++elementsCount;
			}
		}
		if (horizontal || scroll.scrollType == InnerScroll::HorizontalScroll)
		{
			wholeGridSize.y = min(gridSize.y, elementsCount);
			wholeGridSize.x = elementsCount != 0 ? (elementsCount + gridSize.y - 1) / gridSize.y : 0; // to round it nicely to contain all
		}
		else
		{
			wholeGridSize.x = min(gridSize.x, elementsCount);
			wholeGridSize.y = elementsCount != 0? (elementsCount + gridSize.x - 1) / gridSize.x : 0; // to round it nicely to contain all
		}
	}
	wholeGridSpace = Vector2((elementSize.x + actualElementSpacing.x) * (float)wholeGridSize.x - actualElementSpacing.x,
							 (elementSize.y + actualElementSpacing.y) * (float)wholeGridSize.y - actualElementSpacing.y);
	wholeGridSpace.x = max(wholeGridSpace.x, scroll.spaceAvailable.x);
	wholeGridSpace.y = max(wholeGridSpace.y, scroll.spaceAvailable.y);

	scroll.calculate_other_auto_variables(at, wholeGridSpace);

	VectorInt2 gridIdx = VectorInt2::zero;
	{
		// left top
		for_every_ref(element, elements)
		{
			if (!is_visible(element))
			{
				continue;
			}
			Vector2 elAtLT(at.x.min, at.y.max);
			elAtLT.x += (elementSize.x + actualElementSpacing.x) * (float)(gridIdx.x);
			elAtLT.y -= (elementSize.y + actualElementSpacing.y) * (float)(gridIdx.y);
			elAtLT -= scroll.scroll;
			elAtLT.x = round(elAtLT.x);
			elAtLT.y = round(elAtLT.y);

			element->at = Range2(Range(elAtLT.x, elAtLT.x + elementSize.x - 1.0f),
				Range(elAtLT.y - elementSize.y + 1.0f, elAtLT.y));

			if (horizontal || scroll.scrollType == InnerScroll::HorizontalScroll)
			{
				++gridIdx.y;
				if (gridIdx.y == gridSize.y)
				{
					gridIdx.y = 0;
					++gridIdx.x;
				}
			}
			else
			{
				++gridIdx.x;
				if (gridIdx.x == gridSize.x)
				{
					gridIdx.x = 0;
					++gridIdx.y;
				}
			}
		}
	}
}

bool HubWidgets::List::internal_on_hold(int _handIdx, bool _beingHeld, Vector2 const & _at)
{
	bool result;
	if (scroll.handle_internal_on_hold(_handIdx, false, _beingHeld, _at, at, elementSize + actualElementSpacing, OUT_ result))
	{
		return result;
	}

	return base::internal_on_hold(_handIdx, _beingHeld, _at);
}

void HubWidgets::List::internal_on_release(int _handIdx, Vector2 const & _at)
{
	scroll.handle_internal_on_release(_handIdx, false);
}

bool HubWidgets::List::internal_on_hold_grip(int _handIdx, bool _beingHeld, Vector2 const & _at)
{
	bool result;
	if (scroll.handle_internal_on_hold(_handIdx, true, _beingHeld, _at, at, elementSize + actualElementSpacing, OUT_ result))
	{
		return result;
	}

	return base::internal_on_hold_grip(_handIdx, _beingHeld, _at);
}

void HubWidgets::List::internal_on_release_grip(int _handIdx, Vector2 const & _at)
{
	scroll.handle_internal_on_release(_handIdx, true);
}

void HubWidgets::List::process_input(int _handIdx, Framework::GameInput const & _input, float _deltaTime)
{
	scroll.handle_process_input(_handIdx, _input, _deltaTime, elementSize + actualElementSpacing, screen->pixelsPerAngle, allowScrollWithStick? 0 : InnerScroll::NoScrollWithStick);
}

HubDraggable* HubWidgets::List::get_at(Vector2 const& _at)
{
	for_every_ref(element, elements)
	{
		if (!is_visible(element)) continue;
		if (element->at.does_contain(_at) &&
			!element->heldByHand.is_set())
		{
			return element;
		}
	}
	return nullptr;
}

void HubWidgets::List::clear()
{
	elements.clear();
}

HubDraggable* HubWidgets::List::add(IHubDraggableData* _data, Optional<int> const & _at)
{
	RefCountObjectPtr<HubDraggable> draggable;
	draggable = new HubDraggable();
	draggable->from = this;
	draggable->data = _data;
	if (_at.is_set() && elements.is_index_valid(_at.get()))
	{
		elements.insert_at(_at.get(), draggable);
	}
	else
	{
		elements.push_back(draggable);
	}
	return draggable.get();
}

bool HubWidgets::List::select_instead_of_drag(int _handIdx, Vector2 const & _at)
{
	for_every_ref(element, elements)
	{
		if (!is_visible(element)) continue;
		//if (!TeaForGodEmperor::TutorialSystem::can_be_dragged(screen, this, element)) continue;
		if (element->at.does_contain(_at))
		{
			hub->select(this, element);
			return true;
		}
	}
	return false;
}

bool HubWidgets::List::get_to_drag(int _handIdx, Vector2 const & _at, OUT_ RefCountObjectPtr<HubDraggable> & _draggable, OUT_ RefCountObjectPtr<HubDraggable> & _placeHolder)
{
	for_every_ref(element, elements)
	{
		if (!is_visible(element)) continue;
		if (! TeaForGodEmperor::TutorialSystem::can_be_dragged(screen, this, element)) continue;
		if (element->at.does_contain(_at) &&
			! element->heldByHand.is_set())
		{
			if (!draggable)
			{
				hub->select(this, element, true); // pretend we were clicked here
				return false;
			}
			_draggable = element;
			if ((!can_drop_discard || !can_drop_discard(element)) ||
				(should_stay_on_drag && should_stay_on_drag(element)))
			{
				HubDraggable* placeHolder = add(element->data.get(), for_everys_index(element));
				placeHolder->placeHolder = true;
				placeHolder->forceVisible = false;
				_placeHolder = placeHolder;
			}
			return true;
		}
	}
	return false;
}

void HubWidgets::List::hover(int _handIdx, Vector2 const & _at, HubDraggable* _draggable)
{
	int atIdx = NONE;
	for_every_ref(element, elements)
	{
		if (_draggable == element)
		{
			atIdx = for_everys_index(element);
			break;
		}
	}

	// find new index!
	int newIdx = NONE;
	VectorInt2 gridIdx = VectorInt2::zero;
	{
		// left top
		int idx = 0;
		while (true)
		{
			if (elements.is_index_valid(idx) &&
				(!is_visible(elements[idx].get())))
			{
				++idx;
				continue;
			}
			Vector2 elAtLT(at.x.min, at.y.max);
			elAtLT.x += (elementSize.x + actualElementSpacing.x) * (float)(gridIdx.x);
			elAtLT.y -= (elementSize.y + actualElementSpacing.y) * (float)(gridIdx.y);
			elAtLT -= scroll.scroll;
			elAtLT.x = round(elAtLT.x);
			elAtLT.y = round(elAtLT.y);

			Range2 place = Range2(Range(elAtLT.x, elAtLT.x + elementSize.x - 1.0f),
								  Range(elAtLT.y - elementSize.y + 1.0f, elAtLT.y));
			place.expand_by(actualElementSpacing);

			if (place.does_contain(_at))
			{
				newIdx = idx;
			}

			++idx;
			if (horizontal || scroll.scrollType == InnerScroll::HorizontalScroll)
			{
				++gridIdx.y;
				if (gridIdx.y == gridSize.y)
				{
					gridIdx.y = 0;
					++gridIdx.x;
					if (!elements.is_index_valid(idx) && gridIdx.x > gridSize.x)
					{
						break;
					}
				}
			}
			else
			{
				++gridIdx.x;
				if (gridIdx.x == gridSize.x)
				{
					gridIdx.x = 0;
					++gridIdx.y;
					if (!elements.is_index_valid(idx) && gridIdx.y > gridSize.y)
					{
						break;
					}
				}
			}
		}
	}

	if (newIdx != NONE)
	{
		newIdx = clamp(newIdx, 0, elements.get_size()
								+ (atIdx != NONE? -1 /* we are on the list, can't expand it*/ : 0));
		_draggable->hoveringOver = this;
		if (newIdx != atIdx)
		{
			RefCountObjectPtr<HubDraggable> keep;
			keep = _draggable;
			if (atIdx != NONE)
			{
				elements.remove_at(atIdx);
			}
			if (newIdx < elements.get_size())
			{
				elements.insert_at(newIdx, keep);
			}
			else
			{
				elements.push_back(keep);
			}
			play(moveSample);
		}
	}
	else
	{
		remove_draggable(_draggable);
		play(moveSample);
	}
}

void HubWidgets::List::remove_draggable(HubDraggable* _draggable)
{
	_draggable->hoveringOver = nullptr;
	int atIdx = NONE;
	for_every_ref(element, elements)
	{
		if (_draggable == element)
		{
			atIdx = for_everys_index(element);
			break;
		}
	}
	if (atIdx != NONE)
	{
		elements.remove_at(atIdx);
	}
}

bool HubWidgets::List::can_drop_hovering(HubDraggable* _draggableDragged) const
{
	for_every_ref(element, elements)
	{
		if (_draggableDragged == element)
		{
			return true;
		}
	}
	return false;
}

HubDraggable* HubWidgets::List::drop_dragged(int _handIdx, Vector2 const & _at, HubDraggable* _draggableDragged, HubDraggable* _draggableToPlace)
{
	HubDraggable* placed = nullptr;
	// we're hovering here, just do a copy
	int atIdx = NONE;
	for_every_ref(element, elements)
	{
		if (_draggableDragged == element)
		{
			atIdx = for_everys_index(element);
			break;
		}
	}
	an_assert(atIdx != NONE); // this is the same as can_drop_hovering
	if (atIdx != NONE)
	{
		float wasActive = elements[atIdx]->active;
		RefCountObjectPtr<IHubDraggableData> keepData;
		keepData = _draggableToPlace? _draggableToPlace->data : _draggableDragged->data;
		elements.remove_at(atIdx);
		HubDraggable* newDraggable = add(keepData.get(), atIdx);
		newDraggable->active = wasActive;
		placed = newDraggable;
	}

	play(moveSample);

	return placed;
}

bool HubWidgets::List::is_visible(HubDraggable const* _element)
{
	return _element->is_visible_in(this)&&
		  (! should_be_visible || should_be_visible(_element));
}

int HubWidgets::List::find_element_idx(HubDraggable* _draggable) const
{
	for_every_ref(e, elements)
	{
		if (e == _draggable)
		{
			return for_everys_index(e);
		}
	}
	return NONE;
}

void HubWidgets::List::clean_up()
{
	base::clean_up();
	draw_element = nullptr;
	should_be_visible = nullptr;
}

