#include "aiObject.h"

#include "..\modulesOwner\modulesOwner.h"
#include "..\object\object.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(IAIObject);

IAIObject::IAIObject()
: SafeObject<IAIObject>(nullptr)
{
}

void IAIObject::cache_ai_object_pointers()
{
	asIModulesOwner.setup(this);
	asObject.setup(this);
}
