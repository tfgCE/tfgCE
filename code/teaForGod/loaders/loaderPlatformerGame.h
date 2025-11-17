#pragma once

#include "..\..\core\random\random.h"
#include "..\..\core\sound\playback.h"
#include "..\..\core\system\video\renderTargetPtr.h"

#include "..\..\framework\display\display.h"
#include "..\..\framework\game\gameInput.h"
#include "..\..\framework\loaders\iLoader.h"
#include "..\..\framework\library\usedLibraryStored.h"
#include "..\..\framework\sound\soundSample.h"

struct Vector2;
struct VectorInt2;
struct Colour;

namespace System
{
	class Video3D;
	class RenderTarget;
};

namespace Framework
{
	class Display;
};

namespace Platformer
{
	class Game;
};

namespace Loader
{
	class PlatformerGame
	: public ILoader
	{
		typedef ILoader base;
	public:
		PlatformerGame();
		~PlatformerGame();

	public: // ILoader
		implement_ void update(float _deltaTime);
		implement_ void display(::System::Video3D * _v3d, bool _vr);
		implement_ void deactivate();

	private:
		Framework::Display* displayObject;
		Framework::DisplayVRSceneContext displayVRSceneContext;

		::System::RenderTargetPtr mainRT;
#ifdef AN_MINIGAME_PLATFORMER
		::Platformer::Game* game = nullptr;
#endif

		::Framework::GameInput loadingMiniGameInput;

		Framework::UsedLibraryStored<Framework::Sample> loadingSample;
		Sound::Playback loadingSamplePlayback; // we don't want sound scene to handle this single sample playback
		Random::Generator randomGenerator;
		float timeToClear = 0.0f;
		float timeToDeactivation = 0.0f;
		bool gameLoaded = false;
		bool gamepadWasActive = false;
		bool displayingGamepadWasActive = false;
		bool switchedOff = false;

		void clear_display();

		void display_text(String const & _text);

		void update_game_loaded_info();

		void switch_off();
	};
};
