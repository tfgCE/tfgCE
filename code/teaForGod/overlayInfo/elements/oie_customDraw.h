#pragma once

#include "..\overlayInfoElement.h"

namespace TeaForGodEmperor
{
	namespace OverlayInfo
	{
		namespace Elements
		{
			struct CustomDraw
			: public Element
			{
				FAST_CAST_DECLARE(CustomDraw);
				FAST_CAST_BASE(Element);
				FAST_CAST_END();

				typedef Element base;
			public:
				typedef std::function<void(float _active, float _pulse, Colour const & _colour)> CustomDrawFunc;
				
				CustomDraw() {}
				
				CustomDraw* with_draw(CustomDrawFunc _custom_draw) { custom_draw = _custom_draw; return this; }
				CustomDraw* with_size(float _size = 0.1f, bool _autoScale = true) { size = _size; autoScale = _autoScale;  return this; }
				CustomDraw* one_sided(bool _oneSided = true) { oneSided = _oneSided; return this; }
				CustomDraw* with_use_additional_scale(Optional<float> const & _scale) { useAdditionalScale = _scale; return this; }

			public: // util functions to make it easier for custom drawing methods inside
				static void start_blend(float _active);
				static void end_blend(float _active);

			public:
				implement_ void render(System const* _system, int _renderLayer) const;

			protected:
				CustomDrawFunc custom_draw = nullptr;
				float size = 0.1f;
				bool autoScale = true;
				bool oneSided = false;
				Optional<float> useAdditionalScale;
			};
		}
	};
};
