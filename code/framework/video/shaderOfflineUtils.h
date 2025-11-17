#pragma once

#include "..\..\core\types\optional.h"

namespace Framework
{
	// these are functions that output code that can be used by shaders
	namespace ShaderOfflineUtils
	{
		void output_ssao_kernel(Optional<int> const& _samples, Optional<int> const& _seed);
	};
};
