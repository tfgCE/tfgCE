#pragma once

#include "..\loaderHubScene.h"
#include "..\loaderHubWidget.h"
#include "..\utils\lhu_moveStick.h"

namespace TeaForGodEmperor
{
	class Tutorial;
};

namespace Loader
{
	namespace HubScenes
	{
		class TutorialSelection
		: public HubScene
		{
			FAST_CAST_DECLARE(TutorialSelection);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public:
			void redo_all();
			void start_tutorial();
			void go_back();

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ bool does_want_to_end() { return deactivateHub; }
			implement_ void process_input(int _handIdx, Framework::GameInput const& _input, float _deltaTime);

		private:
			bool deactivateHub = false;
			RefCountObjectPtr<HubWidget> listWidget;

			Utils::MoveStick moveStick[Hand::MAX];

			void show_list();

			TeaForGodEmperor::Tutorial* get_selected_tutorial() const;
		};
	};
};
