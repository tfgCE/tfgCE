#include "wavevrImpl.h"

#ifdef AN_WITH_WAVEVR
#include "wavevr.h"

using namespace VR;

DEFINE_STATIC_NAME(openvr);

bool WaveVR::is_available()
{
	return WaveVRImpl::is_available();
}

WaveVR::WaveVR()
: base(NAME(openvr), new WaveVRImpl(this))
{
}

WaveVR::~WaveVR()
{
}

void WaveVR::create_config_files()
{
}

#endif