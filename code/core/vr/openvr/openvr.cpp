#include "openvrImpl.h"

#ifdef AN_WITH_OPENVR
#include "openvr.h"

using namespace VR;

DEFINE_STATIC_NAME(openvr);

bool OpenVR::is_available()
{
	return OpenVRImpl::is_available();
}

OpenVR::OpenVR()
: base(NAME(openvr), new OpenVRImpl(this))
{
}

OpenVR::~OpenVR()
{
}

void OpenVR::create_config_files()
{
	OpenVRImpl::create_config_files();
}

#endif