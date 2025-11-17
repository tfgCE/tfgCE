#pragma once

#include "..\loaderHubScene.h"
#include "..\loaderHubWidget.h"

#include "..\..\..\game\playerSetup.h"

namespace Loader
{
	namespace HubScenes
	{
		class SetupNewGameSlot
		: public HubScene
		{
			FAST_CAST_DECLARE(SetupNewGameSlot);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ bool does_want_to_end() { return deactivateHub; }

		private:
			bool deactivateHub = false;

			RefCountObjectPtr<TeaForGodEmperor::PlayerGameSlot> gameSlot;

			bool startImmediately = false;

			enum State
			{
				Init,
				//
				SetupGameDefinitionPart1,
				SetupGameDefinitionPart2,
				SetupGameDefinitionPart3,
				//
				Done
			};

			State state = State::Init;
			bool firstTimePart2 = true;

			void deactivate_screens();

			void prev_state();
			void next_state();
			void setup_state(bool _asNext);
		};
	};
};
