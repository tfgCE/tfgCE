#pragma once

#include "..\loaderHubWidget.h"

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
		struct Line
		: public HubWidget
		{
			FAST_CAST_DECLARE(Line);
			FAST_CAST_BASE(HubWidget);
			FAST_CAST_END();

		public:
			typedef HubWidget base;

			Vector2 from;
			Vector2 to;
			float width = 0.0f;

			Optional<bool> verticalFirst; // if set will use only horizontal and vertical lines

			Line() {}
			Line(Vector2 const & _from, Vector2 const & _to, Colour const & _colour, Optional<bool> const & _verticalFirst) : HubWidget(Name::invalid(), Range2(_from)), from(_from), to(_to), verticalFirst(_verticalFirst), colour(_colour) {}

			void set_colour(Colour const& _colour) { if (colour != _colour) mark_requires_rendering(); colour = _colour; }

		public: // HubWidget
			implement_ void render_to_display(Framework::Display* _display);

		private:
			Colour colour;
		};
	};
};
