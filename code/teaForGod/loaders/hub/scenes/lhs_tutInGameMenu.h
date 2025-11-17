#pragma once

#include "..\loaderHubScene.h"
#include "..\loaderHubWidget.h"

namespace Loader
{
	namespace HubScreens
	{
		class TutInGameMenu;
	};

	namespace HubScenes
	{
		class TutInGameMenu
		: public HubScene
		{
			FAST_CAST_DECLARE(TutInGameMenu);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public:
			void go_back_to_game();
			void restart();
			void open_options();
			void end_game();

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ bool does_want_to_end() { return deactivateHub; }

		private:
			bool deactivateHub = false;

			void show_in_game_menu();

			friend class HubScreens::TutInGameMenu;
		};
	};
};
