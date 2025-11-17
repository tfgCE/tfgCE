#pragma once

#include "..\loaderHubScene.h"

#include "..\..\..\..\core\types\string.h"

#include <functional>

namespace Loader
{
	struct HubScreen;

	namespace HubScenes
	{
		class FadeOut
		: public HubScene
		{
			FAST_CAST_DECLARE(FadeOut);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public:
			FadeOut();

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ void on_update(float _deltaTime);
			implement_ bool does_want_to_end() { return true; /* will take care of fading */ }

		};
	};
};
