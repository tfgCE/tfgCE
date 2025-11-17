#pragma once

#include "..\framework.h"

namespace Framework
{
	namespace Render
	{

		namespace SceneMode
		{
			enum Type
			{
				Normal,
				VR_Left,
				VR_Right,
			};
		};

		namespace SceneView
		{
			enum Type
			{
				FromCamera,		// scene is build from a camera and that's what we see
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
				External, // all external, isometric, actual top down etc, camera
#endif
			};
		};
	};
};