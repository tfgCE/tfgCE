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
		class Question
		: public HubScreen
		{
			FAST_CAST_DECLARE(Question);
			FAST_CAST_BASE(HubScreen);
			FAST_CAST_END();

			typedef HubScreen base;
		public:
			enum Flags
			{
				YesOnTrigger = 1
			};
		public:
			static void ask(Hub* _hub, String const & _question, std::function<void()> _yes, std::function<void()> _no, Optional<int> const & _secondsLimit = NP, int _flags = 0);
			static void ask(Hub* _hub, Name const & _locStrId, std::function<void()> _yes, std::function<void()> _no, Optional<int> const & _secondsLimit = NP, int _flags = 0);

		public: // HubScreen
			override_ void advance(float _deltaTime, bool _beyondModal);

		private:
			Question(Hub* _hub, String const & _question, std::function<void()> _yes, std::function<void()> _no, Vector2 const & _size, Rotator3 const & _at, float _radius, Optional<int> const & _secondsLimit = NP, int _flags = 0);

		private:
			String question;
			std::function<void()> onYes;
			std::function<void()> onNo;

			Optional<int> secondsLimit; // to accept, by default gets "no"
			float timeToNextSecond;
			int secondsLeft = 0;
			int flags = 0;
			HubWidgets::Text* secondsWidget;
		};
	};
};
