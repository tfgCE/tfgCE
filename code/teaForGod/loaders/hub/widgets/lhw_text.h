#pragma once

#include "..\..\..\..\core\containers\list.h"
#include "..\loaderHubWidget.h"
#include "lhws_innerScroll.h"

namespace Loader
{
	struct HubScreen;
	class Hub;

	namespace HubWidgets
	{
		struct Text
		: public HubWidget
		{
			FAST_CAST_DECLARE(Text);
			FAST_CAST_BASE(HubWidget);
			FAST_CAST_END();

			typedef HubWidget base;
		protected:
			float active = 0.0f;
			float activeTarget = 0.0f;

			String text;
			Name locStrId;

		public:
			Vector2 alignPt = Vector2::half;
			Vector2 scale = Vector2::one;
			bool forceFixedSize = false;

			InnerScroll scroll;

			Text() {}
			Text(Name const & _id, Range2 const & _at, String const & _text) : base(_id, _at), text(_text) { }
			Text(Name const & _id, Range2 const & _at, Name const & _locStrId) : base(_id, _at), locStrId(_locStrId) { }
			
			Text* with_align_pt(Vector2 const& _alignPt) { alignPt = _alignPt; return this; }
			Text* with_smaller_decimals(Optional<float> _smallerDecimals = 0.75f) { smallerDecimals = _smallerDecimals; return this; }

			// in pixels!
			static void measure(Framework::Font const* _font, Optional<Vector2> const & _scale, Optional<bool> const& _forceFixedSize, String const & _text, OUT_ int & _lineCount, OUT_ int & _maxWidth, Optional<int> const & _maxWidthAvailable = NP, ::List<String> * _outputLines = nullptr);

			void set(String const& _text) { if (text != _text) mark_requires_rendering(); text = _text; locStrId = Name::invalid(); }
			void set(Name const & _locStrId) { if (locStrId != _locStrId) mark_requires_rendering(); text = String::empty(); locStrId = _locStrId; }
			void clear() { mark_requires_rendering(); text = String::empty(); locStrId = Name::invalid(); }

			virtual void set_colour(Optional<Colour> const& _colour) { if (colour != _colour) mark_requires_rendering(); colour = _colour; }
			void set_highlighted(bool _highlighted) { if (highlighted != _highlighted) mark_requires_rendering(); highlighted = _highlighted; }

			bool is_highlighted() const { return highlighted; }

			String const & get_text() const { return locStrId.is_valid() ? LOC_STR(locStrId) : text; }

			float calculate_vertical_size();

		public: // HubWidget
			implement_ void advance(Hub* _hub, HubScreen* _screen, float _deltaTime, bool _beyondModal); // sets in hub active widget

			implement_ void render_to_display(Framework::Display* _display);

			implement_ void internal_on_click(HubScreen* _screen, int _handIdx, Vector2 const & _at, HubInputFlags::Flags _flags);
			implement_ bool internal_on_hold(int _handIdx, bool _beingHeld, Vector2 const & _at);
			implement_ void internal_on_release(int _handIdx, Vector2 const & _at);
			implement_ bool internal_on_hold_grip(int _handIdx, bool _beingHeld, Vector2 const & _at);
			implement_ void internal_on_release_grip(int _handIdx, Vector2 const & _at);

			implement_ void process_input(int _handIdx, Framework::GameInput const & _input, float _deltaTime);

		protected:
			Colour overrideColour = Colour::none; // uses alpha, only by derived classes

			bool highlighted = false;
			Optional<Colour> colour;

			String textToDraw;
			String textSplitted;
			float textReadiedForX = 0.0f;
			::List<String> linesToDraw;
			Array<Range> linesOnScreenY;
			String splitter;
			Optional<float> smallerDecimals;

			CACHED_ HubWidgetUsedColours lastUsedColours; // in case derived class needs it

			void calculate_auto_values();

			void ready_text(float _xAvailable);
		};
	};
};
