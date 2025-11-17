#include "oculusMobileImpl.h"

#ifdef AN_WITH_OCULUS_MOBILE
#include "oculusMobile.h"

using namespace VR;

DEFINE_STATIC_NAME(oculusMobile);

bool OculusMobile::is_available()
{
	return OculusMobileImpl::is_available();
}

OculusMobile::OculusMobile()
: base(NAME(oculusMobile), new OculusMobileImpl(this))
{
}

OculusMobile::~OculusMobile()
{
}
#endif
