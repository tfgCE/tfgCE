#pragma once

#include "..\loaderHubScene.h"
#include "..\loaderHubWidget.h"

#include "..\..\..\game\runSetup.h"

namespace Loader
{
	namespace HubScenes
	{
		class QuickGameMenu
		: public HubScene
		{
			FAST_CAST_DECLARE(QuickGameMenu);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ void on_update(float _deltaTime);
			implement_ void on_deactivate(HubScene* _next);
			implement_ void on_loader_deactivate();
			implement_ bool does_want_to_end() { return deactivateHub; }

		private:
			bool deactivateHub = false;
			
			TeaForGodEmperor::RunSetup runSetup;

			void start_pilgrimage();
			void go_back_to_player_profile_selection();

		private:
			void change_difficulty(int _dir);

			void store_run_setup();

			RefCountObjectPtr<HubWidget> difficultyWidget;
			RefCountObjectPtr<HubWidget> difficultyInfoWidget;
			RefCountObjectPtr<HubWidget> customDifficultySetupWidget;
		};
	};
};
