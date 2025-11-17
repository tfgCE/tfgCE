#pragma once

#include "..\loaderHubScene.h"
#include "..\loaderHubWidget.h"

#include "..\..\..\game\runSetup.h"
#include "..\..\..\game\gameState.h"

namespace Framework
{
	class LibraryBasedParameters;
};

namespace Loader
{
	namespace HubScenes
	{
		class PilgrimageSetup
		: public HubScene
		{
			FAST_CAST_DECLARE(PilgrimageSetup);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public:
			static void request_game_modifiers_on_next_setup();
			static void request_choose_pilgrimage_on_next_setup();
			static void mark_reached_end(bool _mark = true);

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ void on_update(float _deltaTime);
			implement_ void on_deactivate(HubScene* _next);
			implement_ void on_loader_deactivate();
			implement_ bool does_want_to_end() { return deactivateHub; }

		private:
			bool deactivateHub = false;
			
			TeaForGodEmperor::RunSetup runSetup;

			Optional<TeaForGodEmperor::GameStateLevel::Type> startWhat;
			Framework::LibraryBasedParameters* startTestScenario = nullptr;

			void start_pilgrimage(Optional<TeaForGodEmperor::GameStateLevel::Type> const & _what = NP); // will ask if we want to start
			void start_pilgrimage_now(); // just triggers pilgrimage start/restart
			void start_test_scenario(Framework::LibraryBasedParameters* _startTestScenario);

			void abort_mission_question();
			void abort_mission();

			void switch_game_slot_mode_unlock_question(TeaForGodEmperor::GameSlotMode::Type _gameSlotMode);
			void switch_game_slot_mode_question(TeaForGodEmperor::GameSlotMode::Type _gameSlotMode);
			void switch_game_slot_mode(TeaForGodEmperor::GameSlotMode::Type _gameSlotMode);

			void manage_profile();
			void rename_profile(Optional<String> const& _profileName = NP);
			void remove_profile();
			void remove_game_slot();
			void change_profile_game_slot();

			String get_profile_name() const;
			String get_game_slot_info() const;

			void deactivate_screens();

		private:
			void store_run_setup();

		private:
			String showRenameMessagePending;
			void show_rename_message(String const& _renamedTo) { showRenameMessagePending = _renamedTo; }

		private:
			enum ShowMainPanel
			{
				MP_MainMenu,
				MP_GameModifiers,
				MP_Unlocks,
				MP_Stats,
				MP_ChooseStart,
				MP_ChooseMission,
				MP_ChooseTestStart,

				MP_NUM
			};

			static ShowMainPanel showMainPanel;

			static bool s_gameModifiersRequestedOnNextSetup;
			static bool s_choosePilgrimageOnNextSetup;
			static bool s_reachedEnd;

			int realTimeSeconds = NONE;

			bool allPilgrimagesUnlockedCheat = false;

			int changePilgrimageRestart = 0;
			CACHED_ int chosenPilgrimageRestart = 0;
			CACHED_ int availablePilgrimages = 0;

			RefCountObjectPtr<HubDraggable> selectedGameStateElement;

			Optional<float> mainPanelHeight;

			Optional<Rotator3> mainDir;

			void show_main_panel(Optional<ShowMainPanel> const & _showMainPanel);

			void change_pilgrimage_restart(int _by);
		};
	};
};
