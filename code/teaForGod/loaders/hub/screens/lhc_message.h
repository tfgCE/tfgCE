#pragma once

#include "..\loaderHubScreen.h"

namespace Loader
{
	namespace HubWidgets
	{
		struct Text;
	};

	namespace HubScreens
	{
		struct MessageSetup
		{
			Optional<bool> follow;
			Optional<int> secondsLimit;
			float alignXPt = 0.5f;
		};

		class Message
		: public HubScreen
		{
			FAST_CAST_DECLARE(Message);
			FAST_CAST_BASE(HubScreen);
			FAST_CAST_END();

			typedef HubScreen base;
		public:
			static Message* show(Hub* _hub, String const & _message, std::function<void()> _ok, MessageSetup const & _messageSetup = MessageSetup());
			static Message* show(Hub* _hub, Name const & _locStrId, std::function<void()> _ok, MessageSetup const& _messageSetup = MessageSetup());

		public: // HubScreen
			override_ void advance(float _deltaTime, bool _beyondModal);

		private:
			Message(Hub* _hub, String const & _message, std::function<void()> _ok, Vector2 const & _size, Rotator3 const & _at, float _radius, MessageSetup const& _messageSetup = MessageSetup());

		private:
			String message;
			std::function<void()> onOk; // if nullptr, no button
			
			Optional<int> secondsLimit; // to accept, by default gets "no"
			float timeToNextSecond;
			int secondsLeft = 0;
			HubWidgets::Text* secondsWidget;
		};
	};
};
