#pragma once

#include "..\utils\skipScene.h"
#include "..\..\core\mesh\mesh3dInstance.h"
#include "..\..\core\sound\playback.h"
#include "..\..\framework\library\usedLibraryStored.h"
#include "..\..\framework\loaders\loaderBase.h"

//

class SimpleVariableStorage;

namespace Framework
{
	class Font;
	namespace Render
	{
		class Camera;
	};
};

namespace Loader
{
	class Countdown
	: public Loader::Base
	{
		typedef Loader::Base base;
	public:
		Colour backgroundColour = Colour::black;

	public:
		Countdown(int _preCountdown, int _countdown);
		~Countdown();

	public: // ILoader
		implement_ void update(float _deltaTime);
		implement_ void display(::System::Video3D * _v3d, bool _vr);
		implement_ bool activate();
		implement_ void deactivate();

	private:
		Framework::Font* font = nullptr;

		float timeToCountdown = 0.0f;
		float timeToNextCountdown = 0.0f;
		int countdown = 3;

		Optional<Transform> viewPlacement;
		Optional<Transform> refViewPlacement;

		void render_countdown(::System::RenderTarget* _rt, ::Framework::Render::Camera const& _camera, bool _vr);
	};
};
