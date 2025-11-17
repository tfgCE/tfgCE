#include "latentFunctions.h"

using namespace Latent;

LATENT_FUNCTION(Functions::wait_precise)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("wait_precise"));

	LATENT_PARAM(float, _howLong);

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
			LATENT_END();
		}
		isAlreadyWaiting = true;
		LATENT_YIELD();
	}
	LATENT_ON_END();
	LATENT_END_CODE();
	LATENT_RETURN();
}
