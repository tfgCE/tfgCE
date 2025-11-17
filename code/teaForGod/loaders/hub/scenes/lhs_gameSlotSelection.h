#pragma once

#include "..\loaderHubScene.h"
#include "..\loaderHubWidget.h"
#include "..\utils\lhu_moveStick.h"

namespace Loader
{
	namespace HubScenes
	{
		class GameSlotSelection
		: public HubScene
		{
			FAST_CAST_DECLARE(GameSlotSelection);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ bool does_want_to_end() { return deactivateHub; }
			implement_ void process_input(int _handIdx, Framework::GameInput const& _input, float _deltaTime);

		private:
			bool deactivateHub = false;

			List<String> addNewLines;

			Utils::MoveStick moveStick[Hand::MAX];
			bool stickSelection = false;

			//

			void deactivate_screens();
			
			//

			void start();
			
			void select_game_slot_idx(int _gameSlotIdx);
			void add_new_game_slot();

			void exit_to_player_profile_selection();
		};
	};
};
