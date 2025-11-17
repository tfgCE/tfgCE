#pragma once

#include "fbxLib.h"

#include "..\..\core\types\string.h"
#include "..\..\core\concurrency\spinLock.h"
#include "..\..\core\concurrency\scopedSpinLock.h"

namespace FBX
{
#ifdef USE_FBX
	class Scene;
#endif

	class Importer
	{
	public:
		static Concurrency::SpinLock importerLock;

		static void finished_importing();
#ifdef USE_FBX
		static Scene* import(String const & _filepath);

	private:
		static Scene* s_scene;
		static String* s_scenePath;
#endif
	};
};
