#include "lhws_innerScroll.h"

#include "..\loaderHubScreen.h"

#include "..\..\..\tutorials\tutorialSystem.h"

#include "..\..\..\..\framework\game\gameInput.h"

#include "..\..\..\..\core\system\core.h"
#include "..\..\..\..\core\system\video\video3dPrimitives.h"
#include "..\..\..\..\core\system\video\viewFrustum.h"
#include "..\..\..\..\core\system\video\videoClipPlaneStackImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Loader;

//

// input
DEFINE_STATIC_NAME(scroll);
DEFINE_STATIC_NAME(scrollUp);
DEFINE_STATIC_NAME(scrollDown);

// system tags
DEFINE_STATIC_NAME(lowGraphics);

//

HubWidgets::InnerScroll::InnerScroll()
{
	scrollWidth = max(HubScreen::s_fontSizeInPixels.x, HubScreen::s_fontSizeInPixels.y);
}

void HubWidgets::InnerScroll::make_sane()
{
	if (scrollType == HorizontalScroll)
	{
		scroll.x = scrollRange.clamp(scroll.x);
		scroll.y = 0.0f;
	}
	else if (scrollType == VerticalScroll)
	{
		scroll.x = 0.0f;
		scroll.y = scrollRange.clamp(scroll.y);
	}
	else
	{
		scroll = Vector2::zero;
	}
}

void HubWidgets::InnerScroll::calculate_auto_space_available(Range2 const & _at)
{
	spaceAvailable = _at.length();
	
	if (scrollType == HorizontalScroll)
	{
		spaceAvailable.y -= scrollWidth + scrollSpacing;
	}
	if (scrollType == VerticalScroll)
	{
		spaceAvailable.x -= scrollWidth + scrollSpacing;
	}
}

void HubWidgets::InnerScroll::calculate_other_auto_variables(Range2 const & _at, Vector2 const & _wholeThingToShowSize)
{
	if (scrollType == HorizontalScroll)
	{
		if (spaceAvailable.x < _wholeThingToShowSize.x)
		{
			scrollLength = spaceAvailable.x * (spaceAvailable.x / _wholeThingToShowSize.x);
			scrollLength = round(scrollLength);
		}
		else
		{
			scrollLength = spaceAvailable.x;
		}
		scrollRange = Range(0.0f, max(0.0f, (_wholeThingToShowSize.x - spaceAvailable.x) - 1.0f));
		scroll.x = scrollRange.clamp(scroll.x);
		scroll.y = 0.0f;

		scrollWindow = _at;
		scrollWindow.y.max = scrollWindow.y.min + (scrollWidth - 1.0f);
	}
	else if (scrollType == VerticalScroll)
	{
		if (spaceAvailable.y < _wholeThingToShowSize.y)
		{
			scrollLength = spaceAvailable.y * (spaceAvailable.y / _wholeThingToShowSize.y);
			scrollLength = round(scrollLength);
		}
		else
		{
			scrollLength = spaceAvailable.y;
		}
		scrollRange = Range(min(0.0f, -((_wholeThingToShowSize.y - spaceAvailable.y) - 1.0f)), 0.0f);
		scroll.x = 0.0f;
		scroll.y = scrollRange.clamp(scroll.y);

		scrollWindow = _at;
		scrollWindow.x.min = scrollWindow.x.max - (scrollWidth - 1.0f);
	}
	else
	{
		scrollLength = 0.0f;
		scrollRange = Range(0.0f);
		scroll = Vector2::zero;
		scrollWindow = Range2::empty;
	}

	scrollVisible = scrollType != NoScroll;
	scrollRequired = false;
	{
		if (scrollType == HorizontalScroll)
		{
			scrollRequired = _wholeThingToShowSize.x > _at.x.length();
		}
		if (scrollType == VerticalScroll)
		{
			scrollRequired = _wholeThingToShowSize.y > _at.y.length();
		}
		if (autoHideScroll)
		{
			scrollVisible &= scrollRequired;
		}
	}

	{
		viewWindow = _at;

		if (scrollVisible)
		{
			if (scrollType == HorizontalScroll)
			{
				viewWindow.y.min += scrollWidth;
			}
			else if (scrollType == VerticalScroll)
			{
				viewWindow.x.max -= scrollWidth;
			}
		}
	}

}

float HubWidgets::InnerScroll::calculate_scroll_at(Range2 const & _at) const
{
	if (scrollType == HorizontalScroll)
	{
		float scrollAtPt = scrollRange.get_pt_from_value(scroll.x);
		float scrollAt = (spaceAvailable.x - scrollLength) * scrollAtPt;
		scrollAt += _at.x.min;
		scrollAt = round(scrollAt);
		if (scrollAtPt == 1.0f)
		{
			scrollAt = _at.x.max - scrollLength;
		}
		return scrollAt;
	}
	else if (scrollType == VerticalScroll)
	{
		float scrollAtPt = scrollRange.get_pt_from_value(scroll.y);
		float scrollAt = (spaceAvailable.y - scrollLength) * scrollAtPt;
		scrollAt += _at.y.min;
		scrollAt = round(scrollAt);
		if (scrollAtPt == 1.0f)
		{
			scrollAt = _at.y.max - scrollLength;
		}
		return scrollAt;
	}
	return 0.0f;
}

void HubWidgets::InnerScroll::render_to_display(Framework::Display* _display, Range2 const & _at, Colour const & _border, Colour const & _inside)
{
	if (! scrollVisible)
	{
		return;
	}
	Colour border = _border;
	Colour inside = _inside;
	if (::System::Core::get_system_tags().get_tag(NAME(lowGraphics)))
	{
		border.a = 1.0f;
		inside.a = 1.0f;
	}
	if (scrollType == HorizontalScroll)
	{
		float scrollAt = calculate_scroll_at(_at);
		Range2 scrollRange(Range(scrollAt, scrollAt + scrollLength), Range(_at.y.min, _at.y.min + scrollWidth - 2.0f));
		Range scrollAxis(scrollRange.y);
		scrollAxis.min += round(scrollRange.y.length() * 0.3f);
		scrollAxis.max -= round(scrollRange.y.length() * 0.3f);
		::System::Video3DPrimitives::fill_rect_2d(border, Vector2(_at.x.min, scrollAxis.min),							Vector2(_at.x.max, scrollAxis.max),								false);
		::System::Video3DPrimitives::fill_rect_2d(inside, Vector2(_at.x.min + 2.0f, scrollAxis.min + 2.0f),				Vector2(_at.x.max - 2.0f, scrollAxis.max - 2.0f),				false);
		::System::Video3DPrimitives::fill_rect_2d(border, Vector2(scrollRange.x.min, scrollRange.y.min),				Vector2(scrollRange.x.max, scrollRange.y.max),					false);
		::System::Video3DPrimitives::fill_rect_2d(inside, Vector2(scrollRange.x.min + 2.0f, scrollRange.y.min + 2.0f),	Vector2(scrollRange.x.max - 2.0f, scrollRange.y.max - 2.0f),	false);
	}
	else if (scrollType == VerticalScroll)
	{
		//::System::Video3DPrimitives::fill_rect_2d(border, Vector2(at.x.min, at.y.min), Vector2(at.x.max - scrollWidth, at.y.max), false);
		//::System::Video3DPrimitives::fill_rect_2d(inside, Vector2(at.x.min + 2.0f, at.y.min + 2.0f), Vector2(at.x.max - scrollWidth - 2.0f, at.y.max - 2.0f), false);

		float scrollAt = calculate_scroll_at(_at);
		Range2 scrollRange(Range(_at.x.max - scrollWidth + 2.0f, _at.x.max), Range(scrollAt, scrollAt + scrollLength));
		Range scrollAxis(scrollRange.x);
		scrollAxis.min += round(scrollRange.x.length() * 0.3f);
		scrollAxis.max -= round(scrollRange.x.length() * 0.3f);
		::System::Video3DPrimitives::fill_rect_2d(border, Vector2(scrollAxis.min, _at.y.min),							Vector2(scrollAxis.max, _at.y.max),								false);
		::System::Video3DPrimitives::fill_rect_2d(inside, Vector2(scrollAxis.min + 2.0f, _at.y.min + 2.0f),				Vector2(scrollAxis.max - 2.0f, _at.y.max - 2.0f),				false);
		::System::Video3DPrimitives::fill_rect_2d(border, Vector2(scrollRange.x.min, scrollRange.y.min),				Vector2(scrollRange.x.max, scrollRange.y.max),					false);
		::System::Video3DPrimitives::fill_rect_2d(inside, Vector2(scrollRange.x.min + 2.0f, scrollRange.y.min + 2.0f),	Vector2(scrollRange.x.max - 2.0f, scrollRange.y.max - 2.0f),	false);
	}
	else
	{
		//::System::Video3DPrimitives::fill_rect_2d(border, Vector2(at.x.min, at.y.min),								Vector2(at.x.max, at.y.max),									false);
		//::System::Video3DPrimitives::fill_rect_2d(inside, Vector2(at.x.min + 2.0f, at.y.min + 2.0f),					Vector2(at.x.max - 2.0f, at.y.max - 2.0f),						false);
	}
}

void HubWidgets::InnerScroll::push_clip_plane_stack()
{
	if (scrollType == NoScroll || ! scrollVisible)
	{
		return;
	}

	::System::Video3D* v3d = ::System::Video3D::get();
	auto & clipPlaneStack = v3d->access_clip_plane_stack();

	{
		Range2 clipped = viewWindow;
		//clipped.x.min -= 1.0f; // clip plane included?
		//clipped.y.min -= 1.0f; // clip plane included?
		clipped.x.max += 1.0f; // because we want to be at the end of the pixel!
		clipped.y.max += 1.0f; // because we want to be at the end of the pixel!
		clipPlaneStack.push();
		PlaneSet clipPlanes;
		clipPlanes.add(Plane(Vector3::xAxis, Vector3(clipped.x.min, clipped.y.min, 0.0f)));
		clipPlanes.add(Plane(-Vector3::xAxis, Vector3(clipped.x.max, clipped.y.max, 0.0f)));
		clipPlanes.add(Plane(Vector3::yAxis, Vector3(clipped.x.min, clipped.y.min, 0.0f)));
		clipPlanes.add(Plane(-Vector3::yAxis, Vector3(clipped.x.max, clipped.y.max, 0.0f)));
		clipPlaneStack.set_current(clipPlanes);
	}
}

void HubWidgets::InnerScroll::pop_clip_plane_stack()
{
	if (scrollType == NoScroll || ! scrollVisible)
	{
		return;
	}

	::System::Video3D* v3d = ::System::Video3D::get();
	auto & clipPlaneStack = v3d->access_clip_plane_stack();

	clipPlaneStack.pop();
}

bool HubWidgets::InnerScroll::handle_internal_on_hold(int _handIdx, bool _gripped, bool _beingHeld, Vector2 const & _cursorAt, Range2 const & _at, Vector2 const & _singleAdvancement, OUT_ bool & _result)
{
	if (!scrollVisible)
	{
		return false;
	}

	auto& sHeld = _gripped ? scrollHeldGrip[_handIdx] : scrollHeld[_handIdx];

	if (!scrollWindow.is_empty() &&
		(scrollWindow.does_contain(_cursorAt) || sHeld.held ||
		 (_gripped && viewWindow.does_contain(_cursorAt))) &&
		scrollType != NoScroll)
	{
		if (!_beingHeld)
		{
			if (scrollType == HorizontalScroll)
			{
				// presses beyond
				float scrollAt = calculate_scroll_at(_at);
				if (_cursorAt.x < scrollAt || _cursorAt.x > scrollAt + scrollLength)
				{
					if (_cursorAt.x < scrollAt)
					{
						scroll.x -= _singleAdvancement.x;
					}
					else
					{
						scroll.x += _singleAdvancement.x;
					}
					scroll.x = scrollRange.clamp(scroll.x);
					_result = false;
					return true;
				}
			}
			else if (scrollType == VerticalScroll)
			{
				// presses beyond
				float scrollAt = calculate_scroll_at(_at);
				if (_cursorAt.y < scrollAt || _cursorAt.y > scrollAt + scrollLength)
				{
					if (_cursorAt.y < scrollAt)
					{
						scroll.y -= _singleAdvancement.y;
					}
					else
					{
						scroll.y += _singleAdvancement.y;
					}
					scroll.y = scrollRange.clamp(scroll.y);
					_result = false;
					return true;
				}
			}
			sHeld.held = true;
			sHeld.heldStartAt = _cursorAt;
			sHeld.heldStartScroll = scroll;
		}
		else
		{
			Vector2 difference = _cursorAt - sHeld.heldStartAt;
			if (scrollType == HorizontalScroll)
			{
				if (scrollLength < scrollWindow.x.length())
				{
					float by = (difference.x / (scrollWindow.x.length() - scrollLength)) * scrollRange.length();
					scroll.x = scrollRange.clamp(sHeld.heldStartScroll.x + by);
				}
				else
				{
					scroll.y = 0.0f;
				}
			}
			else if (scrollType == VerticalScroll)
			{
				if (scrollLength < scrollWindow.y.length())
				{
					float by = (difference.y / (scrollWindow.y.length() - scrollLength)) * scrollRange.length();
					scroll.y = scrollRange.clamp(sHeld.heldStartScroll.y + by);
				}
				else
				{
					scroll.y = 0.0f;
				}
			}
		}
		_result = true;
		return true;
	}
	return false;
}

void HubWidgets::InnerScroll::handle_internal_on_release(int _handIdx, bool _gripped)
{
	if (_gripped)
	{
		scrollHeldGrip[_handIdx].held = false;
	}
	else
	{
		scrollHeld[_handIdx].held = false;
	}
}

void HubWidgets::InnerScroll::handle_process_input(int _handIdx, Framework::GameInput const & _input, float _deltaTime, Vector2 const & _singleAdvancement, Vector2 const& _pixelsPerAngle, int _flags)
{
	if (!scrollVisible)
	{
		return;
	}

	if (scrollType == HorizontalScroll)
	{
		float scrollBy = 0.0f;
		if (!is_flag_set(_flags, NoScrollWithStick))
		{
			scrollBy += _input.get_stick(NAME(scroll)).x * _deltaTime * 50.0f * _pixelsPerAngle.x;
		}
		scrollBy += _input.get_mouse(NAME(scroll)).x;
		if (_input.has_button_been_pressed(NAME(scrollUp)))
		{
			scrollBy += _singleAdvancement.x;
		}
		if (_input.has_button_been_pressed(NAME(scrollDown)))
		{
			scrollBy -= _singleAdvancement.x;
		}
		scroll.x = scrollRange.clamp(scroll.x + scrollBy);
	}
	else if (scrollType == VerticalScroll)
	{
		float scrollBy = 0.0f;
		if (!is_flag_set(_flags, NoScrollWithStick))
		{
			scrollBy += _input.get_stick(NAME(scroll)).y * _deltaTime * 50.0f * _pixelsPerAngle.y;
		}
		scrollBy += _input.get_mouse(NAME(scroll)).y;
		if (_input.has_button_been_pressed(NAME(scrollUp)))
		{
			scrollBy += _singleAdvancement.y;
		}
		if (_input.has_button_been_pressed(NAME(scrollDown)))
		{
			scrollBy -= _singleAdvancement.y;
		}
		scroll.y = scrollRange.clamp(scroll.y + scrollBy);
	}
}

