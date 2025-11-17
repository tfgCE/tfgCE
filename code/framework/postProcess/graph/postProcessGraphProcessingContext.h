#pragma once

#include "..\..\..\core\math\math.h"
#include "..\graph\iPostProcessGraphProcessingInputRenderTargetsProvider.h"
#include "..\graph\iPostProcessGraphProcessingResolutionProvider.h"

namespace System
{
	class Video3D;
	class Camera;
};

namespace Framework
{
	class PostProcessRenderTargetManager;

	struct PostProcessGraphProcessingContext
	{
		bool withMipMaps = false;
		::System::Camera const * camera; // camera that input render target was rendered with - it isn't required if it's just 2d post processing without any spacial information
		::System::Video3D* video3D;
		PostProcessRenderTargetManager* renderTargetManager;
		IPostProcessGraphProcessingInputRenderTargetsProvider const* inputRenderTargetsProvider;
		IPostProcessGraphProcessingResolutionProvider const* resolutionProvider;

		PostProcessGraphProcessingContext();

		PostProcessGraphProcessingContext for_previous() const
		{
			PostProcessGraphProcessingContext result = *this;
			result.withMipMaps = false;
			return result;
		}
	};

};
