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
		struct Rect
		: public HubWidget
		{
			FAST_CAST_DECLARE(Rect);
			FAST_CAST_BASE(HubWidget);
			FAST_CAST_END();

		public:
			typedef HubWidget base;

			Colour colour;
			bool fill = false;

			Rect() {}
			Rect(Range2 const & _at, Colour const & _colour) : HubWidget(Name::invalid(), _at), colour(_colour) {}

			Rect* filled() { fill = true; return this; }

		public: // HubWidget
			implement_ void render_to_display(Framework::Display* _display);
		};
	};
};
