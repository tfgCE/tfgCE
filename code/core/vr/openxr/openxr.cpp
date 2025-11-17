#include "openxrImpl.h"

#ifdef AN_WITH_OPENXR
#include "openxr.h"

//

using namespace VR;

//

DEFINE_STATIC_NAME(openxr);

//

bool OpenXR::is_available()
{
	return OpenXRImpl::is_available();
}

OpenXR::OpenXR(bool _firstChoice)
: base(NAME(openxr), new OpenXRImpl(this, _firstChoice))
{
	// xrEndFrame is not blocking us, xrWaitFrame does
	setup__set_game_sync_blocking(true);
	setup__set_begin_render_requires_game_sync(true);
	setup__set_end_render_blocking(false);
}

OpenXR::~OpenXR()
{
}
#endif