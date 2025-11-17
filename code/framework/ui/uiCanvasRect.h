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
		struct CanvasRect
		: public SoftPooledObject<CanvasRect>
		, public ICanvasElement
		{
			FAST_CAST_DECLARE(CanvasRect);
			FAST_CAST_BASE(ICanvasElement);
			FAST_CAST_END();

			typedef ICanvasElement base;
		public:
			static CanvasRect* get(Vector2 const & _leftBottom, Vector2 const & _rightTop, Colour const & _colour);

		public: // SoftPooledObject<Canvas>
			override_ void on_get();
			override_ void on_release();

		public: // ICanvasElement
			implement_ void release_element() { on_release_element(); release(); }

			implement_ void offset_placement_by(Vector2 const& _offset);
			implement_ Range2 get_placement(Canvas const* _canvas) const;

			implement_ void render(CanvasRenderContext & _context) const;
			implement_ void update_hover_on(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw) const;

		private:
			Vector2 leftBottom;
			Vector2 rightTop;
			Colour colour;
		};
	};
};

TYPE_AS_CHAR(Framework::UI::CanvasRect);
