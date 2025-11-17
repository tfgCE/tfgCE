#include "aiPerceptionRequestLatent.h"

#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\system\core.h"

using namespace Framework;
using namespace AI;

//

REGISTER_FOR_FAST_CAST(PerceptionRequestLatent);

PerceptionRequestLatent::PerceptionRequestLatent(LATENT_FUNCTION_VARIABLE(_executeFunction))
: executeFunction(_executeFunction)
{
	SETUP_LATENT(latentFrame);

	ADD_LATENT_PARAM(latentFrame, PerceptionRequest*, this);
}

PerceptionRequestLatent::~PerceptionRequestLatent()
{
	stop_latent();
}

bool PerceptionRequestLatent::process()
{
	if (latentEnded)
	{
		return true;
	}

	PERFORMANCE_GUARD_LIMIT(0.002f, TXT("perception request"));

	if (PERFORM_LATENT(latentFrame, ::System::Core::get_delta_time(), executeFunction))
	{
		latentEnded = true;
		return base::process();
	}

	return false;
}

void PerceptionRequestLatent::stop_latent()
{
	if (!latentEnded)
	{
		BREAK_LATENT(latentFrame, executeFunction);
	}
}
