#pragma once

#include "..\loaderHubScreen.h"

namespace Loader
{
	namespace HubScenes
	{
		class TutInGameMenu;
	};

	namespace HubScreens
	{
		class TutInGameMenu
		: public HubScreen
		{
			FAST_CAST_DECLARE(TutInGameMenu);
			FAST_CAST_BASE(HubScreen);
			FAST_CAST_END();

			typedef HubScreen base;
		public:
			TutInGameMenu(Hub* _hub);

			static TutInGameMenu* show_for_hub(Hub* _hub);

			void be_stand_alone(HubScenes::TutInGameMenu * _tutInGameMenuScene); // if interrupted from world

		public: // HubScreen
			override_ void advance(float _deltaTime, bool _beyondModal);

		public:
			void go_back_to_game();
			void end_tutorial();

		private:
			HubScenes::TutInGameMenu* tutInGameMenuScene = nullptr; // stand alone
		};
	};
};
