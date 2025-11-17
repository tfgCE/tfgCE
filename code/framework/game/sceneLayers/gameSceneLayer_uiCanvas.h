#pragma once

#include "..\..\..\core\concurrency\spinLock.h"
#include "..\..\..\core\math\math.h"

#include "..\gameSceneLayer.h"

#include "..\..\ui\uiCanvas.h"

namespace Framework
{
	class Game;

	namespace GameSceneLayers
	{
		class UICanvas
		: public Framework::GameSceneLayer
		{
			FAST_CAST_DECLARE(UICanvas);
			FAST_CAST_BASE(Framework::GameSceneLayer);
			FAST_CAST_END();
			typedef Framework::GameSceneLayer base;
		public:
			UICanvas();
			virtual ~UICanvas();

			void mark_to_end() { weWantToEnd = true; }

		public:
			void set_background_colour(Optional<Colour> const& _backgroundColour) { backgroundColour = _backgroundColour; }

		public:
			UI::Canvas* access_canvas() { return canvas; }

		public: // Framework::GameSceneLayer
			override_ bool should_end() const { return endNow; }

			// we don't call previous, we stop here
			override_ void pre_advance(Framework::GameAdvanceContext const& _advanceContext);
			override_ void advance(Framework::GameAdvanceContext const& _advanceContext);

			override_ Optional<bool> is_input_grabbed() const { return false; }
			override_ void process_controls(Framework::GameAdvanceContext const& _advanceContext);
			override_ void process_all_time_controls(Framework::GameAdvanceContext const& _advanceContext);
			override_ void process_vr_controls(Framework::GameAdvanceContext const& _advanceContext);

			override_ void prepare_to_sound(Framework::GameAdvanceContext const& _advanceContext);

			override_ void prepare_to_render(Framework::CustomRenderContext* _customRC);
			override_ void render_on_render(Framework::CustomRenderContext* _customRC);

			override_ void render_after_post_process(Framework::CustomRenderContext* _customRC);

		private:
			bool weWantToEnd = false;
			bool endNow = false;

			Optional<Colour> backgroundColour; // renders background only if provided

			UI::Canvas* canvas = nullptr;
			Vector2 canvasLogicalSize = Vector2(1000.0f, 800.0f);
		};
	};
};
