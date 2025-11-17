#include "lhw_button.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"
#include "..\..\..\tutorials\tutorialSystem.h"

#include "..\..\..\..\framework\video\font.h"
#include "..\..\..\..\framework\video\texturePart.h"

#include "..\..\..\..\core\system\core.h"
#include "..\..\..\..\core\system\video\video3dPrimitives.h"
#include "..\..\..\..\core\vr\iVR.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Loader;

//

// system tags
DEFINE_STATIC_NAME(lowGraphics);

//

REGISTER_FOR_FAST_CAST(HubWidgets::Button);

void HubWidgets::Button::advance(Hub* _hub, HubScreen* _screen, float _deltaTime, bool _beyondModal)
{
	base::advance(_hub, _screen, _deltaTime, _beyondModal);

	if (firstAdvance)
	{
		active = activeTarget;
		highlightCurrent = highlighted ? 1.0f : 0.0f;
		enabledState = enabled? 1.0f : 0.0f;
	}

	float activePrev = active;
	float highlightPrev = highlightCurrent;
	float enabledStatePrev = enabledState;

	activeTarget = 0.0f;
	for_count(int, hIdx, 2)
	{
		auto & hand = _hub->access_hand((::Hand::Type)hIdx);
		if (hand.overWidget == this)
		{
			activeTarget = hand.prevOverWidget == this ? 1.0f : 0.0f;
		}
	}
	active = blend_to_using_time(active, activeTarget, firstAdvance? 0.0f : 0.1f, _deltaTime);

	highlightCurrent = blend_to_using_time(highlightCurrent, highlighted? 1.0f : 0.0f, firstAdvance ? 0.0f : 0.1f, _deltaTime);

	enabledState = blend_to_using_time(enabledState, enabled? 1.0f : 0.0f, firstAdvance ? 0.0f : 0.1f, _deltaTime);

	firstAdvance = false;

	if ((activePrev != active && abs(active - activeTarget) > 0.005f) ||
		(highlightPrev != highlightCurrent && abs(highlightCurrent - (highlighted? 1.0f : 0.0f)) > 0.005f) ||
		(enabledState != enabledStatePrev && abs(enabledState - (enabled? 1.0f : 0.0f)) > 0.005f))
	{
		mark_requires_rendering();
	}

	bool activateOnTriggerHoldNow = activateOnTriggerHold;
	if (activateOnTriggerHoldNow || activateOnHold)
	{
		bool activationIsHeld = false;
		if (enabled)
		{
			if (activateOnTriggerHoldNow && !_beyondModal && _screen->is_active())
			{
				if (auto* vr = VR::IVR::get())
				{
					if (!triggerWasReleasedAtLeastOnce)
					{
						// we require both triggers to be released before we allow holding any of them
						triggerWasReleasedAtLeastOnce = true;
						if (! _hub->is_trigger_held_for_button_hold())
						{
							triggerWasReleasedAtLeastOnce = false;
						}
					}
					else
					{
						if (_hub->is_trigger_held_for_button_hold())
						{
							activationIsHeld = true;
						}
					}
				}
			}
			if (activateOnHold && !_beyondModal && _screen->is_active())
			{
				for_count(int, hIdx, Hand::MAX)
				{
					Hand::Type hand = (Hand::Type)hIdx;
					activationIsHeld |= (_hub->get_hand(hand).heldWidget == this);
				}
			}
		}

		float prevActivationHeld = activationHeld;

		if (!activationIsHeld)
		{
			if (activationHeld == 1.0f)
			{
				// reset, was just activated
				activationHeld = 0.0f;
			}
			if (activateAlwaysRestart)
			{
				// always reset to start
				activationHeld = 0.0f;
			}
		}

		activationHeld += (activationIsHeld ? 1.0f : -1.0f) * _deltaTime * (activateTimeLength != 0.0f? 1.0f / activateTimeLength : 1.0f);
		activationHeld = clamp(activationHeld, 0.0f, 1.0f);

		if (activationHeld != prevActivationHeld)
		{
			if (text.is_empty() && locStrId.is_valid())
			{
				// move to text
				text = LOC_STR(locStrId);
				locStrId = Name::invalid();
			}
			int len = text.get_length();
			int upTo = (int)((float)(len + 1) * activationHeld);
			for_count(int, i, len)
			{
				auto& ch = text.access_data()[i];
				ch = i < upTo ? String::to_upper(ch) : String::to_lower(ch);
			}
			if (activateOnTriggerHoldNow)
			{
				highlighted = activationHeld;
				highlightCurrent = highlightCurrent + activationHeld * (1.0f - highlightCurrent);
			}
			mark_requires_rendering();
		}

		if (activationHeld >= 1.0f &&
			prevActivationHeld < 1.0f)
		{
			_hub->force_click(this);
		}
	}
}

void HubWidgets::Button::render_to_display(Framework::Display* _display)
{
	// rendering may happen before update?
	if (firstAdvance)
	{
		active = activeTarget;
		highlightCurrent = highlighted ? 1.0f : 0.0f;
		enabledState = enabled ? 1.0f : 0.0f;
	}

	if (! appearAsText)
	{
		float minAlpha = 0.0f;
		if (::System::Core::get_system_tags().get_tag(NAME(lowGraphics)))
		{
			minAlpha = 0.6f;
		}
		Optional<Colour> useColour;
		if (useColourForBorder) useColour = colour;
		Colour const insideNormal = HubColours::widget_background()/*.with_alpha(max(minAlpha, 0.3f))*/;
		Colour border = Colour::lerp(active,
			highlighted ? HubColours::selected_highlighted() : useColour.get(HubColours::border()/*.with_alpha(max(minAlpha, 0.8f))*/),
			highlighted ? HubColours::selected_highlighted() : useColour.get(HubColours::text()));
		Colour inside = Colour::lerp(active,
			Colour::lerp(highlightCurrent, insideNormal,
										   HubColours::widget_background_highlight()/*.with_alpha(max(minAlpha, 0.6f))*/),
			HubColours::highlight()/*.with_alpha(0.8f)*/);

		if (colourOverridesHighlight && colour.is_set())
		{
			border = colour.get().with_set_alpha(border.a);
			inside = colour.get().with_set_alpha(inside.a);
		}

		if (!colourOverridesDisabled)
		{
			border = useColour.get(Colour::lerp(enabledState, HubColours::mid(), border));
		}
		inside = Colour::lerp(enabledState, insideNormal, inside);

		overrideColour = HubColours::mid().with_alpha(colourOverridesDisabled? 0.0f : 1.0f - enabledState);

		//

		border = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(border, screen, this);
		inside = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(inside, screen, this);

		if (specialHighlight)
		{
			border = HubColours::special_highlight().with_set_alpha(1.0f);
			inside = HubColours::special_highlight_background_for(inside);
		}

		if (enabled || !noBorderWhenDisabled)
		{
			float scale = 2.0f;
			::System::Video3DPrimitives::fill_rect_2d(border, at, false);
			::System::Video3DPrimitives::fill_rect_2d(inside, at.expanded_by(-Vector2(2.0f * scale)), false);
			if (enabled)
			{
				if (activateOnHold)
				{
					::System::Video3DPrimitives::fill_rect_2d(border, at.expanded_by(-Vector2(4.0f * scale)), false);
					::System::Video3DPrimitives::fill_rect_2d(inside, at.expanded_by(-Vector2(6.0f * scale)), false);
				}
				if (activateOnTriggerHold)
				{
					Range2 r = at.expanded_by(-Vector2(4.0f * scale));
					float size = 4.0f * scale;
					::System::Video3DPrimitives::fill_rect_2d(border, Range2(Range(r.x.min, r.x.min + size), Range(r.y.min, r.y.min + size)), false);
					::System::Video3DPrimitives::fill_rect_2d(border, Range2(Range(r.x.min, r.x.min + size), Range(r.y.max - size, r.y.max)), false);
					::System::Video3DPrimitives::fill_rect_2d(border, Range2(Range(r.x.max - size, r.x.max), Range(r.y.min, r.y.min + size)), false);
					::System::Video3DPrimitives::fill_rect_2d(border, Range2(Range(r.x.max - size, r.x.max), Range(r.y.max - size, r.y.max)), false);
				}
			}
		}

		if (texturePart)
		{
			Framework::TexturePartDrawingContext context;
			//context.limits = at;
			context.colour = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(texturePartColour, screen, this);
			context.scale = scale;
			texturePart->draw_at(::System::Video3D::get(), round(at.get_at(alignPt)), context);
		}

		{
			scoped_call_stack_info(id.to_char());
			if (custom_draw)
			{
				custom_draw(_display, at, lastUsedColours);
			}
		}

	}

	base::render_to_display(_display);
}

void HubWidgets::Button::clean_up()
{
	base::clean_up();
	custom_draw = nullptr;
}
