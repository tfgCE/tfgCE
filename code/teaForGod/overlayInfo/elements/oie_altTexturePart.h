#pragma once

#include "..\overlayInfoElement.h"

#include "..\..\..\framework\video\texturePartUse.h"

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
			struct AltTexturePart
			: public Element
			{
				FAST_CAST_DECLARE(AltTexturePart);
				FAST_CAST_BASE(Element);
				FAST_CAST_END();

				typedef Element base;
			public:
				AltTexturePart() {}
				AltTexturePart(Framework::TexturePart* _tp) { with_texture_part(_tp); }
				AltTexturePart(Framework::TexturePartUse const & _tpu) { with_texture_part_use(_tpu); }

				AltTexturePart* with_size(float _size) { size = _size; return this; }
				AltTexturePart* with_horizontal_size(float _horizontalSize) { horizontalSize = _horizontalSize; return this; }
				AltTexturePart* with_vertical_size(float _verticalSize) { verticalSize = _verticalSize; return this; }
				AltTexturePart* with_texture_part(Framework::TexturePart* _tp) { texturePartUse = Framework::TexturePartUse::as_is(_tp); return this; }
				AltTexturePart* with_texture_part_use(Framework::TexturePartUse const & _tpu) { texturePartUse = _tpu; return this; }
				AltTexturePart* with_alt_texture_part(Framework::TexturePart* _tp) { altTexturePartUse = Framework::TexturePartUse::as_is(_tp); return this; }
				AltTexturePart* with_alt_texture_part_use(Framework::TexturePartUse const & _tpu) { altTexturePartUse = _tpu; return this; }
				AltTexturePart* with_alt_offset_pt(Vector2 const & _offsetPt) { altOffsetPt = _offsetPt; return this; }
				AltTexturePart* with_timing(float _periodTime, float _pt, bool _altFirst = false) { periodTime = _periodTime; altPt = _pt; altFirst = _altFirst;  return this; }
				AltTexturePart* with_alt_on_top(bool _altOnTop = false) { altOnTop = _altOnTop;  return this; }
				AltTexturePart* with_alt_colour(Colour const& _colour) { altColour = _colour; return this; }

			public:
				implement_ void advance(System const* _system, float _deltaTime);
				implement_ void render(System const* _system, int _renderLayer) const;

			protected:
				Framework::TexturePartUse texturePartUse;
				Framework::TexturePartUse altTexturePartUse;
				float time = 0.0f;
				float periodTime = 1.0f;
				float altPt = 0.5f;
				bool altFirst = false;
				bool altOnTop = false;
				Optional<Vector2> altOffsetPt;
				Optional<float> size;
				Optional<float> horizontalSize;
				Optional<float> verticalSize;
				Optional<Colour> altColour;
			};
		}
	};
};
