#pragma once

#include "..\..\..\core\globalDefinitions.h"
#include "..\..\..\core\types\colour.h"

#include "..\..\..\framework\game\gameSceneLayer.h"

namespace TeaForGodEmperor
{
	class Game;

	namespace GameSceneLayers
	{
		/**
		 *	Scene that stops updating anything below - should be used as boundary, starting point
		 */
		class DontUpdatePrev
		: public Framework::GameSceneLayer
		{
			FAST_CAST_DECLARE(DontUpdatePrev);
			FAST_CAST_BASE(Framework::GameSceneLayer);
			FAST_CAST_END();
			typedef Framework::GameSceneLayer base;
		public:
			DontUpdatePrev* provide_background(Colour const & _backgroundColour = Colour::black) { provideBackground = true; provideBackgroundColour = _backgroundColour; return this; }

		public:
			override_ void on_placed();
			override_ void on_removed();

			override_ void on_paused() { /* everything below should be paused already */ }
			override_ void on_resumed() { /* everything below should stay paused */ }

			override_ void pre_advance(Framework::GameAdvanceContext const & _advanceContext) {}
			override_ void advance(Framework::GameAdvanceContext const & _advanceContext) {}

			override_ void process_controls(Framework::GameAdvanceContext const & _advanceContext) {}
			override_ void process_vr_controls(Framework::GameAdvanceContext const & _advanceContext) {}

			override_ void prepare_to_sound(Framework::GameAdvanceContext const & _advanceContext);

			override_ void prepare_to_render(Framework::CustomRenderContext * _customRC);
			override_ void render_on_render(Framework::CustomRenderContext * _customRC);

		private:
			bool provideBackground = false;
			Colour provideBackgroundColour = Colour::black;
		};
	};
};
