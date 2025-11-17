#pragma once

#include "..\loaderHubScene.h"

#include "..\..\..\..\core\types\string.h"

namespace Loader
{
	namespace HubScenes
	{
		class HandyMessages
		: public HubScene
		{
			FAST_CAST_DECLARE(HandyMessages);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public:
			HandyMessages();

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ void on_update(float _deltaTime);
			implement_ bool does_want_to_end() { return false; }

		private:
			RefCountObjectPtr<HubScene> prevScene;

			Array<String> messages;
			int idx = NONE;
		};
	};
};
