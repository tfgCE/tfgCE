#pragma once

#include "..\loaderHubScene.h"
#include "..\loaderHubWidget.h"

#include "..\..\..\game\gameStats.h"

namespace Loader
{
	namespace HubScenes
	{
		class Summary
		: public HubScene
		{
			FAST_CAST_DECLARE(Summary);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public:
			Summary(TeaForGodEmperor::PostGameSummary::Type _summaryType);

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ void on_update(float _deltaTime);
			implement_ void on_deactivate(HubScene* _next);
			implement_ void on_loader_deactivate();
			implement_ bool does_want_to_end() { return deactivateHub; }

		private:
			bool showContinue = false;
			bool deactivateHub = false;

			TeaForGodEmperor::PostGameSummary::Type summaryType = TeaForGodEmperor::PostGameSummary::None;

			RefCountObjectPtr<HubWidget> pleaseWaitWidget;
			RefCountObjectPtr<HubWidget> continueFromCheckpointWidget;
			RefCountObjectPtr<HubWidget> exitWidget;

			void show_continue();
			
			void go_to_game_modifiers();
			void go_to_game_slots();
		};
	};
};
