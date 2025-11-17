#include "latentFunctions.h"

#include "..\..\ai\aiTaskHandle.h"
#include "..\..\ai\aiLatentTask.h"

using namespace Framework;
using namespace Framework::Latent;

LATENT_FUNCTION(Functions::wait_for_task)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("wait_for_task"));

	LATENT_PARAM(float, _howLong);
	LATENT_PARAM(AI::LatentTaskHandle, _task);

	LATENT_END_PARAMS();

	if (!_task.is_running())
	{
		_frame.end_waiting();
	}
	
	LATENT_BEGIN_CODE();

	// just wait, if we're to end, we end
	LATENT_WAIT_NO_RARE_ADVANCE(_howLong);

	LATENT_ON_END();
	LATENT_END_CODE();
	LATENT_RETURN();
}
