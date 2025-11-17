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
		struct CustomDraw
		: public HubWidget
		{
			FAST_CAST_DECLARE(CustomDraw);
			FAST_CAST_BASE(HubWidget);
			FAST_CAST_END();

		public:
			typedef HubWidget base;

			typedef std::function<void(Framework::Display* _display, Range2 const& _at)> CustomDrawFunc;

			CustomDrawFunc custom_draw = nullptr;

			CustomDraw() {}
			CustomDraw(Name const & _id, Range2 const & _at, CustomDrawFunc _custom_draw) : base(_id, _at), custom_draw(_custom_draw) {}

			implement_ void clean_up();

		public: // HubWidget
			implement_ void render_to_display(Framework::Display* _display);
		};
	};
};
