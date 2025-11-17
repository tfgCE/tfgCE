#pragma once

#include <functional>

#include "..\loaderHubScene.h"

#include "..\..\..\..\core\types\string.h"

namespace Loader
{
	namespace HubScenes
	{
		class Question
		: public HubScene
		{
			FAST_CAST_DECLARE(Question);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public:
			Question(String const& _question, std::function<void()> _yes, std::function<void()> _no);
			Question(Name const& _locStrId, std::function<void()> _yes, std::function<void()> _no);

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ void on_update(float _deltaTime);
			implement_ void on_deactivate(HubScene* _next);
			implement_ bool does_want_to_end() { return questionDone; }

		private:
			bool questionDone = false;

			String question;
			std::function<void()> on_yes;
			std::function<void()> on_no;

			void show();
		};
	};
};
