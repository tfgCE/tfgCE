#pragma once

#include "..\loaderHubWidget.h"

namespace Loader
{
	struct HubScreen;
	class Hub;

	namespace HubWidgets
	{
		struct Slider
		: public HubWidget
		{
			FAST_CAST_DECLARE(Slider);
			FAST_CAST_BASE(HubWidget);
			FAST_CAST_END();

			typedef HubWidget base;
		public:
			float active = 0.0f;
			float activeTarget = 0.0f;

			Range valueRange;
			float value; // in and out
			float step = 0.0f;

			bool operateAsInt = false;
			RangeInt valueRangeInt;
			std::function<void(RangeInt&)> valueRangeIntDynamic;
			int valueInt; // in and out
			int stepInt = 0;

			Vector2 alignPt = Vector2::half;

			Slider() {}
			Slider(Name const & _id, Range2 const & _at, Range const & _valueRange, float _value) : base(_id, _at), valueRange(_valueRange), value(_value) { alignPt = Vector2::half; operateAsInt = false; }
			Slider(Name const& _id, Range2 const& _at, RangeInt const& _valueRange, int _value, std::function<void(RangeInt&)> _valueRangeIntDynamic = nullptr) : base(_id, _at), valueRangeInt(_valueRange), valueRangeIntDynamic(_valueRangeIntDynamic), valueInt(_value) { alignPt = Vector2::half; operateAsInt = true;  }

		public: // HubWidget
			implement_ void advance(Hub* _hub, HubScreen* _screen, float _deltaTime, bool _beyondModal); // sets in hub active widget

			implement_ void render_to_display(Framework::Display* _display);

			implement_ bool internal_on_hold(int _handIdx, bool _beingHeld, Vector2 const & _at);

			bool is_horizontal() const { return at.x.length() > at.y.length(); }
			Range calc_range() const;
			float slider_width() const;
		};
	};
};
