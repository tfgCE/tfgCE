#pragma once

#include "..\..\core\random\random.h"
#include "..\..\core\sound\playback.h"

#include "..\..\framework\display\display.h"
#include "..\..\framework\loaders\iLoader.h"
#include "..\..\framework\library\usedLibraryStored.h"
#include "..\..\framework\sound\soundSample.h"

struct Vector2;
struct VectorInt2;
struct Colour;

namespace System
{
	class Video3D;
};

namespace Framework
{
	class Display;
};

namespace Loader
{
	class Display
	: public ILoader
	{
		typedef ILoader base;
	public:
		Display();
		~Display();

	public: // ILoader
		implement_ void update(float _deltaTime);
		implement_ void display(::System::Video3D * _v3d, bool _vr);
		implement_ void deactivate();

	private:
		Framework::Display* displayObject;
		Framework::DisplayVRSceneContext displayVRSceneContext;
		Framework::UsedLibraryStored<Framework::Sample> loadingSample;
		Sound::Playback loadingSamplePlayback; // we don't want sound scene to handle this single sample playback
		Random::Generator randomGenerator;
		float timeToDeactivation;
	};
};
