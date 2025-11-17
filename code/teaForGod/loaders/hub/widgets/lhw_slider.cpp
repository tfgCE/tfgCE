#include "lhw_slider.h"

#include "..\loaderHub.h"

#include "..\..\..\tutorials\tutorialSystem.h"

#include "..\..\..\..\framework\video\font.h"

#include "..\..\..\..\core\system\core.h"
#include "..\..\..\..\core\system\video\video3dPrimitives.h"

//

using namespace Loader;

//

// system tags
DEFINE_STATIC_NAME(lowGraphics);

//

REGISTER_FOR_FAST_CAST(HubWidgets::Slider);

void HubWidgets::Slider::advance(Hub* _hub, HubScreen* _screen, float _deltaTime, bool _beyondModal)
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
}

Range HubWidgets::Slider::calc_range() const
{
	bool horizontal = is_horizontal();

	float sliderWidth = slider_width();
	float halfSliderWidth = round(sliderWidth * 0.5f);

	return horizontal ? at.x.expanded_by(-halfSliderWidth) : at.y.expanded_by(-halfSliderWidth);
}

float HubWidgets::Slider::slider_width() const
{
	float xl = at.x.length();
	float yl = at.y.length();

	return round(min(xl, yl) / 2.0f);
}

void HubWidgets::Slider::render_to_display(Framework::Display* _display)
{
	float minAlpha = 0.0f;
	if (::System::Core::get_system_tags().get_tag(NAME(lowGraphics)))
	{
		minAlpha = 0.6f;
	}
	Colour border = Colour::lerp(active, HubColours::border()/*.with_alpha(max(minAlpha, 0.8f))*/, HubColours::text());
	Colour inside = Colour::lerp(active, HubColours::widget_background()/*.with_alpha(max(minAlpha, 0.3f))*/, HubColours::highlight()/*.with_alpha(max(minAlpha, 0.8f))*/);

	bool horizontal = is_horizontal();

	Range range = calc_range();
	float sliderWidth = slider_width();
	float halfSliderWidth = round(sliderWidth * 0.5f);

	::System::Video3DPrimitives::fill_rect_2d(border, at, false);
	::System::Video3DPrimitives::fill_rect_2d(inside, at.expanded_by(-Vector2(2.0f)), false);

	if (operateAsInt)
	{
		value = (float)valueInt;
		if (valueRangeIntDynamic)
		{
			valueRangeIntDynamic(valueRangeInt);
		}
		valueRange = Range((float)valueRangeInt.min, (float)valueRangeInt.max);
	}
	float pt = valueRange.get_pt_from_value(value);
	pt = clamp(pt, 0.0f, 1.0f);
	float actAt = range.get_at(pt);
	if (horizontal)
	{
		::System::Video3DPrimitives::fill_rect_2d(border, Vector2(actAt - halfSliderWidth, at.y.min), Vector2(actAt + halfSliderWidth, at.y.max), false);
	}
	else
	{
		::System::Video3DPrimitives::fill_rect_2d(border, Vector2(at.x.min, actAt - halfSliderWidth), Vector2(at.y.max, actAt + halfSliderWidth), false);
	}

	base::render_to_display(_display);
}

bool HubWidgets::Slider::internal_on_hold(int _handIdx, bool _beingHeld, Vector2 const & _at)
{
	mark_requires_rendering();

	Range range = calc_range();

	float pt = 0.5f;
	if (is_horizontal())
	{
		pt = range.get_pt_from_value(_at.x);
	}
	else
	{
		pt = range.get_pt_from_value(_at.y);
	}

	pt = clamp(pt, 0.0f, 1.0f);
	value = valueRange.get_at(pt);
	if (operateAsInt)
	{
		valueInt = (int)(round(value) + 0.1f);
		if (stepInt > 1)
		{
			int rest = mod(valueInt, stepInt);
			valueInt = valueInt - rest;
			if (rest >= stepInt / 2)
			{
				valueInt += stepInt;
			}
		}
	}
	else
	{
		if (step != 0.0f)
		{
			value = round_to(value, step);
		}
	}

	return true;
}
