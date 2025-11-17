#include "lhs_empty.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"
#include "..\loaderHubWidget.h"

#include "..\..\..\..\core\vr\iVR.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Loader;
using namespace HubScenes;

//

REGISTER_FOR_FAST_CAST(Empty);

Empty::Empty(bool _waitForVR)
: waitForVR(_waitForVR)
{
}

void Empty::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	loaderDeactivated = false;

	get_hub()->allow_to_deactivate_with_loader_immediately(!waitForVR);

	get_hub()->deactivate_all_screens();
}

void Empty::on_deactivate(HubScene* _next)
{
	base::on_deactivate(_next);
	loaderDeactivated = true;
}

bool Empty::does_want_to_end()
{
	if (waitForVR)
	{
		if (auto* vr = VR::IVR::get())
		{
			if (vr->is_controls_view_valid() &&
				vr->is_render_base_valid())
			{
				return true;
			}
		}
		else
		{
			return true;
		}
		return false;
	}
	return loaderDeactivated;
}
