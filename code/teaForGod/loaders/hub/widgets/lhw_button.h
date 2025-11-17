#pragma once

#include "lhw_text.h"

namespace Framework
{
	class TexturePart;
};

namespace Loader
{
	struct HubScreen;
	class Hub;

	namespace HubWidgets
	{
		struct Button
		: public Text
		{
			FAST_CAST_DECLARE(Button);
			FAST_CAST_BASE(Text);
			FAST_CAST_END();

			typedef Text base;
		public:
			bool enabled = true;
			float enabledState = 1.0f;
			
			float active = 0.0f;
			float activeTarget = 0.0f;

			float highlightCurrent = 0.0f;

			bool activateOnHold = false;
			bool activateAlwaysRestart = false;
			bool activateOnTriggerHold = false;
			bool triggerWasReleasedAtLeastOnce = false;
			float activationHeld = 0.0f;
			float activateTimeLength = 0.5f;

			Framework::TexturePart const * texturePart = nullptr;
			Colour texturePartColour = Colour::white;

			HubCustomDrawFunc custom_draw = nullptr;

			bool noBorderWhenDisabled = false;

			Button() {}
			Button(Name const & _id, Range2 const & _at, String const & _text) : base(_id, _at, _text) { alignPt = Vector2::half; }
			Button(Name const & _id, Range2 const & _at, Name const & _locStrId) : base(_id, _at, _locStrId) { alignPt = Vector2::half; }
			Button(Name const & _id, Range2 const & _at, Framework::TexturePart const * _tp, Optional<Colour> const & _texturePartColour = NP) : base(_id, _at, Name::invalid()), texturePart(_tp), texturePartColour(_texturePartColour.get(Colour::white)) { alignPt = Vector2::half; }

			void enable(bool _enabled = true) { enabled = _enabled; }
			void disable() { enabled = false; }

			void appear_as_text() { appearAsText = true; }

			virtual void set_colour(Optional<Colour> const& _colour) { set_colour(_colour, false, false); }
			virtual void set_colour(Optional<Colour> const& _colour, Optional<bool> const& _overrideDisabled, Optional<bool> const& _overrideHighlight = NP) { base::set_colour(_colour); colourOverridesDisabled = _overrideDisabled.get(colourOverridesDisabled); colourOverridesHighlight = _overrideHighlight.get(colourOverridesHighlight); }

		public: // HubWidget
			implement_ void advance(Hub* _hub, HubScreen* _screen, float _deltaTime, bool _beyondModal); // sets in hub active widget

			implement_ void render_to_display(Framework::Display* _display);

			implement_ bool does_allow_to_process_on_click(HubScreen* _screen, int _handIdx, Vector2 const& _at, HubInputFlags::Flags _flags) { return !activateOnHold && enabled; }
			implement_ bool internal_on_hold(int _handIdx, bool _beingHeld, Vector2 const& _at) { return base::internal_on_hold(_handIdx, _beingHeld, _at) || activateOnHold; }

			implement_ void clean_up();

		private:
			bool firstAdvance = true;
			bool useColourForBorder = true;
			bool appearAsText = false;

			bool colourOverridesDisabled = false;
			bool colourOverridesHighlight = false;
		};
	};
};
