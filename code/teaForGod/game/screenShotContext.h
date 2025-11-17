#pragma once

//#define RENDER_TO_BIG_PANELS
//#define BIG_SCREEN_SHOTS

namespace System
{
	class RenderTarget;
};

namespace Framework
{
	struct CustomRenderContext;
};

namespace TeaForGodEmperor
{
	class Game;

	struct ScreenShotContext
	{
		static ScreenShotContext* get_active() { return s_active; }

		static void store_screen_shot(::System::RenderTarget* rt);

		static bool is_doing_screen_shot() { return s_active? s_active->doScreenShot : false; }

		Game* game;
#ifdef RENDER_TO_BIG_PANELS
		int screenShotIdx = NONE;
		int screenShotCount = 0;
		float screenShotViewCentreOffsetX = 0.0f;
#endif
		bool screenShotCustomRenderContextIdx = 0;
		Framework::CustomRenderContext* useCustomRenderContext = nullptr;

		ScreenShotContext(Game* _game);
		~ScreenShotContext();

		void pre_render();
		void post_render();

	private:
		static ScreenShotContext* s_active;
		bool doScreenShot = false;
	};
};
