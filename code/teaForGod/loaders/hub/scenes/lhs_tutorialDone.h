#pragma once

#include "..\loaderHubScene.h"
#include "..\loaderHubWidget.h"

namespace Loader
{
	namespace HubScenes
	{
		class TutorialDone
		: public HubScene
		{
			FAST_CAST_DECLARE(TutorialDone);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public:
			void exit();
			void repeat();
			void next();

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ bool does_want_to_end() { return deactivateHub; }

		private:
			bool deactivateHub = false;

			void show_menu();
		};
	};
};
