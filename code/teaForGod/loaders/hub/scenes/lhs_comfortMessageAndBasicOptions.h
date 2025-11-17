#pragma once

#include "..\loaderHubScene.h"

#include "..\..\..\..\core\types\string.h"

namespace Loader
{
	namespace HubScenes
	{
		class ComfortMessageAndBasicOptions
		: public HubScene
		{
			FAST_CAST_DECLARE(ComfortMessageAndBasicOptions);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public:
			ComfortMessageAndBasicOptions(float _delay);

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ void on_update(float _deltaTime);
			implement_ void on_deactivate(HubScene* _next);
			implement_ void on_loader_deactivate();
			implement_ bool does_want_to_end() { return okToEnd; }

		private:
			bool okToEnd = false;
			float delay = 0.0f;
			float timeToShow = 0.0f;

			bool showGameIsLoading = true;

			void show();

			void update_buttons();
		};
	};
};
