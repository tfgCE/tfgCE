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
	class ArmActivator
	: public ILoader
	{
		typedef ILoader base;
	public:
		ArmActivator();
		~ArmActivator();

	public: // ILoader
		implement_ void update(float _deltaTime);
		implement_ void display(::System::Video3D * _v3d, bool _vr);
		implement_ void deactivate();

	private:
		bool allowDeactivating = false;
		Framework::Display* displayObject;
		Framework::DisplayVRSceneContext displayVRSceneContext;
		Framework::UsedLibraryStored<Framework::Sample> loadingSample;
		Sound::Playback loadingSamplePlayback; // we don't want sound scene to handle this single sample playback
		Random::Generator randomGenerator;

		bool pointingAtScreen = false;
		int show_start();
		int show_wait();
		int show_point_at_screen();
		int show_pull_trigger();
	};
};
