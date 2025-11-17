#pragma once

#include "..\sceneLayers\gameSceneLayer_DontUpdatePrev.h"

#include "..\..\..\core\globalDefinitions.h"
#include "..\..\..\core\sound\playback.h"

#include "..\..\..\framework\game\gameInput.h"
#include "..\..\..\framework\game\gameSceneLayer.h"
#include "..\..\..\framework\time\time.h"

namespace Framework
{
	class Player;
};

namespace TeaForGodEmperor
{
	class Game;
	class Player;

	namespace GameScenes
	{
		class Wait
		: public Framework::GameSceneLayer
		{
			FAST_CAST_DECLARE(Wait);
			FAST_CAST_BASE(Framework::GameSceneLayer);
			FAST_CAST_END();
			typedef Framework::GameSceneLayer base;
		public:
			typedef std::function<bool()> FinishedWaitingFunc;
			Wait(FinishedWaitingFunc _finishedWaitingFunc);
			virtual ~Wait();

		protected: // Framework::GameSceneLayer
			override_ void on_start();
			override_ void on_end();

			override_ void on_resumed();

			override_ void process_controls(Framework::GameAdvanceContext const & _advanceContext);
			override_ void advance(Framework::GameAdvanceContext const & _advanceContext);

			override_ bool should_end() const { return shouldEnd; }

#ifdef AN_DEVELOPMENT
			override_ bool allows_to_end_on_demand() const { return false; }
#endif

		private:
			FinishedWaitingFunc finishedWaitingFunc;
			
			RefCountObjectPtr<GameSceneLayers::DontUpdatePrev> startingLayer;

			float fadeInTime = 0.4f;
			float fadeOutTime = 0.4f;

			float timeToEnd = 0.0f;
			bool shouldEnd = false;

			::Framework::GameInput input;

			//

			Framework::Time activeFor = Framework::Time::zero();

			void finish();
		};
	};
};
