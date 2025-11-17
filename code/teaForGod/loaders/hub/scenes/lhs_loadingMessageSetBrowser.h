#pragma once

#include "..\loaderHubScene.h"
#include "..\..\..\library\messageSet.h"

#include "..\..\..\..\core\types\optional.h"
#include "..\..\..\..\core\types\string.h"
#include "..\..\..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	class TexturePart;
};

namespace Loader
{
	namespace HubScenes
	{
		class LoadingMessageSetBrowser
		: public HubScene
		{
			FAST_CAST_DECLARE(LoadingMessageSetBrowser);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public:
			LoadingMessageSetBrowser(TagCondition const & _messageSetsTagged, Optional<String> const& _title, Optional<float> const& _delay, Optional<Name> const& _startWithMessage = NP, Optional<bool> const& _randomOrder = NP, Optional<bool> const& _startWithRandomMessage = NP);
			
			LoadingMessageSetBrowser* close_on_condition(std::function<bool()> _conditionToClose) { conditionToClose = _conditionToClose; return this; }

			void allow_early_close() { allowEarlyClose = true; }

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ void on_update(float _deltaTime);
			implement_ void on_deactivate(HubScene* _next);
			implement_ void on_loader_deactivate();
			implement_ bool does_want_to_end();

		private:
			TagCondition messageSetsTagged;
			String title;
			Optional<Name> startWithMessage;
			Optional<bool> randomOrder;
			Optional<bool> startWithRandomMessage;
			std::function<bool()> conditionToClose = nullptr; // if set, won't wait for loader to deactivate to proceed

			bool allowEarlyClose = false;

			float delay = 0.0f;
			float timeToShow = 0.0f;

			bool loaderDeactivated = false;

			Optional<float> blinkCloseTimeLeft;

			void show();
		};
	};
};
