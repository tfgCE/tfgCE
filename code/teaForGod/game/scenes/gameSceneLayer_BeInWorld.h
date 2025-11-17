#pragma once

#include "..\..\..\core\globalDefinitions.h"

#include "..\..\..\framework\game\gameSceneLayer.h"

namespace TeaForGodEmperor
{
	class Game;

	namespace GameSceneLayers
	{
		class DontUpdatePrev;
		class HUD;
		class World;
		class Display;
	}

	namespace GameScenes
	{
		class BeInWorld
		: public Framework::GameSceneLayer
		{
			FAST_CAST_DECLARE(BeInWorld);
			FAST_CAST_BASE(Framework::GameSceneLayer);
			FAST_CAST_END();
			typedef Framework::GameSceneLayer base;
		public:
			BeInWorld();
			virtual ~BeInWorld();

		public: // Framework::GameSceneLayer
			override_ void on_start();
			override_ void on_end();
			override_ bool should_end() const { return false; /* test, never end */ }

			override_ void advance(Framework::GameAdvanceContext const & _advanceContext);

			override_ Framework::GameSceneLayer* propose_layer_to_replace_yourself();

		protected:
			RefCountObjectPtr<GameSceneLayers::DontUpdatePrev> startingLayer;
			RefCountObjectPtr<GameSceneLayers::World> world;
		};
	};
};
