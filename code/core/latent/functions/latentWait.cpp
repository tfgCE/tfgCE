#include "latentFunctions.h"

using namespace Latent;

LATENT_FUNCTION(Functions::wait)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("wait"));

	LATENT_PARAM(float, _howLong);
	LATENT_PARAM(bool, _useRareAdvance);

	LATENT_END_PARAMS();

	LATENT_VAR(float, timeLeft);
	LATENT_VAR(bool, isAlreadyWaiting);

	// variables for one run
	float timeToAdvance;
	
	LATENT_BEGIN_CODE();
	timeLeft = _howLong;
	isAlreadyWaiting = false;
	while (true)
	{
		if (_frame.should_end_waiting() && isAlreadyWaiting)
		{
			LATENT_END();
		}
		timeToAdvance = min(timeLeft, LATENT_DELTA_TIME);
		LATENT_DECREASE_DELTA_TIME_BY(timeToAdvance);
		timeLeft -= timeToAdvance;
		if (timeLeft <= 0.0f)
		{
			if (!isAlreadyWaiting)
			{
				// we want to skip at least one frame
				LATENT_YIELD();
			}
			LATENT_END();
		}
		isAlreadyWaiting = true;

		if (_useRareAdvance)
		{
			LATENT_WANTS_TO_WAIT_FOR(timeLeft);
		}

		LATENT_YIELD();
	}
	LATENT_ON_END();
		LATENT_ZERO_DELTA_TIME();
	LATENT_END_CODE();
	LATENT_RETURN();
}
