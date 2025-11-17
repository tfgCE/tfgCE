#pragma once

#include "..\loaderHubScene.h"

#include "..\..\..\..\core\math\math.h"
#include "..\..\..\..\core\types\string.h"

namespace Loader
{
	namespace HubScenes
	{
		class VRModeSelection
		: public HubScene
		{
			FAST_CAST_DECLARE(VRModeSelection);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public:
			VRModeSelection(float _delay);

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
			bool allowToShow = true;

			Optional<Rotator3> atDir;

			void show();

			void open_language_selection();
		};
	};
};
