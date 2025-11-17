#pragma once

#include "..\loaderHubScene.h"
#include "..\loaderHubWidget.h"

namespace TeaForGodEmperor
{
	class ModulePilgrim;
};

namespace Loader
{
	namespace HubScenes
	{
		class InGameMenu
		: public HubScene
		{
			FAST_CAST_DECLARE(InGameMenu);
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
			implement_ void on_update(float _deltaTime);
			implement_ bool does_want_to_end() { return deactivateHub; }

		private:
			bool deactivateHub = false;
			TeaForGodEmperor::ModulePilgrim* pilgrim = nullptr;
			RefCountObjectPtr<HubWidget> exitWidget;

			void show_in_game_menu();
		};
	};
};
