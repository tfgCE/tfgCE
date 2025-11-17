#pragma once

#include "..\latent.h"

namespace Latent
{
	class Functions
	{
	public:
		static LATENT_FUNCTION(wait); // latentWait.cpp - consumes whole delta time, waits at least one frame
		static LATENT_FUNCTION(wait_precise); // latentWaitPrecise.cpp - waits just the time, might be called multiple times in a single call
	};
};
