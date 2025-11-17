#pragma once

#include "..\random\randomUtils.h"
#include "..\system\core.h"
#include "..\system\input.h"

#include "pieceCombiner.h"
#include "pieceCombinerDebugVisualiser.h"
#include "pieceCombinerSteps.h"

namespace PieceCombiner
{
	#include "pieceCombiner.inl"
	#include "pieceCombinerDebugVisualiser.inl"
	namespace GenerationSteps
	{
		#include "pieceCombinerSteps.inl"
	};
};

