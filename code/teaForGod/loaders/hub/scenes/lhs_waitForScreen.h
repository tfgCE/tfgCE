#pragma once

#include "..\loaderHubScene.h"

#include "..\..\..\..\core\types\string.h"

#include <functional>

namespace Loader
{
	struct HubScreen;

	namespace HubScenes
	{
		class WaitForScreen
		: public HubScene
		{
			FAST_CAST_DECLARE(WaitForScreen);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public:
			WaitForScreen(HubScreen* _screen);
			WaitForScreen(std::function< HubScreen*(Loader::Hub* _hub)> _create_screen);

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ bool does_want_to_end();

		private:
			RefCountObjectPtr<HubScreen> screen;
			std::function< HubScreen* (Loader::Hub* _hub)> create_screen = nullptr;
		};
	};
};
