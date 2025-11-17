#pragma once

#include "..\loaderHubScene.h"

#include "..\..\..\..\core\types\string.h"

namespace Loader
{
	namespace HubScenes
	{
		class Empty
		: public HubScene
		{
			FAST_CAST_DECLARE(Empty);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public:
			Empty(bool _waitForVR = false);

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ void on_deactivate(HubScene* _next);
			implement_ bool does_want_to_end();

		private:
			bool loaderDeactivated = false;
			bool waitForVR = false;
		};
	};
};
