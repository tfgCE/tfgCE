#pragma once

#include "..\loaderHubScene.h"
#include "..\loaderHubWidget.h"

#include "..\..\..\game\runSetup.h"

namespace Loader
{
	namespace HubScenes
	{
		class SetupNewPlayerProfile
		: public HubScene
		{
			FAST_CAST_DECLARE(SetupNewPlayerProfile);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ void on_update(float _deltaTime);
			implement_ bool does_want_to_end() { return deactivateHub; }

		private:
			bool deactivateHub = false;

			bool nextStatePending = false;

			TeaForGodEmperor::RunSetup runSetup;

			enum State
			{
				Init,
				//
				EnterName,
				//
				Done
			};

			State state = State::Init;

			//

			void deactivate_screens();

			//

			void next_state();

			void ask_for_name(String _usingName = String::empty());
			void add_new_profile(String const& _usingName);
		};
	};
};
