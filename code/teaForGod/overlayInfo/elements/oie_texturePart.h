#pragma once

#include "..\overlayInfoElement.h"

namespace Framework
{
	class TexturePart;
};

namespace TeaForGodEmperor
{
	namespace OverlayInfo
	{
		namespace Elements
		{
			struct TexturePart
			: public Element
			{
				FAST_CAST_DECLARE(TexturePart);
				FAST_CAST_BASE(Element);
				FAST_CAST_END();

				typedef Element base;
			public:
				TexturePart() {}
				TexturePart(Framework::TexturePart* _tp) { with_texture_part(_tp); }

				TexturePart* with_size(float _size) { size = _size; return this; }
				TexturePart* with_horizontal_size(float _horizontalSize) { horizontalSize = _horizontalSize; return this; }
				TexturePart* with_vertical_size(float _verticalSize) { verticalSize = _verticalSize; return this; }
				TexturePart* with_texture_part(Framework::TexturePart* _tp) { texturePart = _tp; return this; }
				TexturePart* with_show_pt(Optional<Range2> const& _showPt) { showPt = _showPt; return this; }

			public:
				implement_ void render(System const* _system, int _renderLayer) const;

			protected:
				Framework::TexturePart* texturePart = nullptr;
				Optional<float> size;
				Optional<float> horizontalSize;
				Optional<float> verticalSize;
				Optional<Range2> showPt;
			};
		}
	};
};
