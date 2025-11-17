#include "aiLogicTask_idle.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\ai\aiTaskHandle.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

LATENT_FUNCTION(TeaForGodEmperor::AI::Logics::Tasks::idle)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("idle"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_BEGIN_CODE();

	// keep this loop running
	while (true)
	{
		if (mind)
		{
			::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
			locomotion.stop();
		}

		LATENT_WAIT(Random::get_float(0.6f, 1.2f));
	}

	LATENT_END();

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

