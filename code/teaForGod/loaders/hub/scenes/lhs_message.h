#pragma once

#include "..\loaderHubScene.h"

#include "..\..\..\..\core\types\string.h"

namespace Loader
{
	namespace HubScenes
	{
		class Message
		: public HubScene
		{
			FAST_CAST_DECLARE(Message);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public:
			Message(String const & _message, float _delay = 0.0f);
			Message(tchar const * _message, float _delay = 0.0f);
			Message(Name const & _locStrId, float _delay = 0.0f);

			Message* with_margin(int _margin) { margin = _margin; return this; }
			Message* requires_acceptance() { requiresAcceptance = true; return this; }

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ void on_update(float _deltaTime);
			implement_ void on_deactivate(HubScene* _next);
			implement_ void on_loader_deactivate();
			implement_ bool does_want_to_end() { return messageAccepted; }

		private:
			String message;
			bool requiresAcceptance = false;
			bool messageAccepted = false;
			int margin = 1; // in characters
			float delay = 0.0f;
			float timeToShow = 0.0f;

			void show();
		};
	};
};
