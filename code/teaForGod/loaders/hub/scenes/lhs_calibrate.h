#pragma once

#include "..\loaderHubScene.h"
#include "..\loaderHubWidget.h"

#include "..\..\..\roomGenerators\roomGenerationInfo.h"

namespace TeaForGodEmperor
{
	struct PlayerPreferences;
};

namespace Loader
{
	namespace HubScenes
	{
		class Calibrate
		: public HubScene
		{
			FAST_CAST_DECLARE(Calibrate);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public:
			static float const max_distance;

			Calibrate();
			~Calibrate();

			void go_back(); // and go back to previous scene

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ void on_update(float _deltaTime);
			implement_ bool does_want_to_end() { return false; } // never wants to end hub, it should go back to previous scene

		private:
			RefCountObjectPtr<HubScene> prevScene;

			float height = 1.75f;
			float doorHeight = 2.0f;

			void show_screen();
			void calibrate();
			void store_calibration();
		};
	};
};
