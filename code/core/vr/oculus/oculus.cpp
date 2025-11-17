#include "oculusImpl.h"

#ifdef AN_WITH_OCULUS
#include "oculus.h"

using namespace VR;

DEFINE_STATIC_NAME(oculus);

bool Oculus::is_available()
{
	return OculusImpl::is_available();
}

Oculus::Oculus()
: base(NAME(oculus), new OculusImpl(this))
{
}

Oculus::~Oculus()
{
}
#endif
