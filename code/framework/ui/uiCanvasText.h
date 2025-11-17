#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\memory\softPooledObject.h"
#include "..\..\core\types\colour.h"

#include "uiCanvasElement.h"

namespace Framework
{
	class Font;

	namespace UI
	{
		struct CanvasText
		: public SoftPooledObject<CanvasText>
		, public ICanvasElement
		{
			FAST_CAST_DECLARE(CanvasText);
			FAST_CAST_BASE(ICanvasElement);
			FAST_CAST_END();

			typedef ICanvasElement base;
		public:
			static CanvasText* get();

			CanvasText* set_at(Vector2 const& _at) { at = _at; return this; }
			CanvasText* set_anchor_rel_placement(Vector2 const& _anchorRelPlacement = Vector2::half) { anchorRelPlacement = _anchorRelPlacement; return this; }
			CanvasText* set_colour(Colour const & _colour) { colour = _colour; return this; }
			CanvasText* set_text(String const & _text) { text = _text; return this; }
			CanvasText* set_text(tchar const * _text) { text = _text; return this; }
			CanvasText* set_scale(float _scale = 1.0f) { scale = _scale; return this; }

		public: // SoftPooledObject<Canvas>
			override_ void on_get();
			override_ void on_release();

		public: // ICanvasElement
			implement_ void release_element() { on_release_element(); release(); }

			implement_ void offset_placement_by(Vector2 const& _offset);
			implement_ Range2 get_placement(Canvas const* _canvas) const;

			implement_ void render(CanvasRenderContext & _context) const;

		private:
			String text;
			Vector2 at = Vector2::zero;
			Vector2 anchorRelPlacement = Vector2::half;
			Optional<Colour> colour;
			Optional<float> height;
			float scale = 1.0f;

			float calculate_use_height(Canvas const* _canvas) const;
		};
	};
};

TYPE_AS_CHAR(Framework::UI::CanvasText);
