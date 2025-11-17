#pragma once

#include "..\loaderHubScene.h"
#include "..\loaderHubWidget.h"
#include "..\utils\lhu_moveStick.h"

#include "..\..\..\game\playerSetup.h"

namespace Loader
{
	namespace HubScenes
	{
		class StartingPilgrimageSelection
		: public HubScene
		{
			FAST_CAST_DECLARE(StartingPilgrimageSelection);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ void on_update(float _deltaTime);
			implement_ void on_deactivate(HubScene* _next);
			implement_ void on_loader_deactivate();
			implement_ bool does_want_to_end() { return deactivateHub; }
			implement_ void process_input(int _handIdx, Framework::GameInput const& _input, float _deltaTime);

		private:
			bool deactivateHub = false;

			RefCountObjectPtr<TeaForGodEmperor::PlayerGameSlot> gameSlot;

			Utils::MoveStick moveStick[Hand::MAX];
			bool stickSelection = false;

			void deactivate_screens();

			//

			void start();

			void select_profile(String const& _profileName);
			void add_new_profile();

		};
	};
};
