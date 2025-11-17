#pragma once

#include "..\..\..\core\math\math.h"

namespace System
{
	class Video3D;
	class Camera;
};

namespace Framework
{
	class PostProcessRenderTargetManager;

	struct PostProcessChainProcessingContext
	{
		::System::Camera const * camera;
		::System::Video3D* video3D;
		PostProcessRenderTargetManager* renderTargetManager;

		PostProcessChainProcessingContext();
	};

};
