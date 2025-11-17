#include "aiPerceptionRequest.h"

using namespace Framework;
using namespace AI;

//

REGISTER_FOR_FAST_CAST(PerceptionRequest);

bool PerceptionRequest::process()
{
	processed = true;
	return true;
}
