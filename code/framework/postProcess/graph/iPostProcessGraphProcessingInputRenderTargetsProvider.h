#pragma once

#include "..\..\..\core\types\name.h"

namespace System
{
	class Video3D;
};

namespace Framework
{
	struct PostProcessRenderTargetPointer;

	interface_class IPostProcessGraphProcessingInputRenderTargetsProvider
	{
	public:
		virtual PostProcessRenderTargetPointer const & get_input_render_target(Name const & _inputName) const = 0;
	};
};
