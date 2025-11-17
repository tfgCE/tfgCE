#include "loaderHubInSequence.h"

#include "loaderHub.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Loader;

//

HubInSequence::HubInSequence(Hub* _hub, HubScene* _scene, Optional<bool> const& _allowFadeOut)
: hub(_hub)
, scene(_scene)
, allowFadeOut(_allowFadeOut.get(true))
{
}

HubInSequence::~HubInSequence()
{
}

void HubInSequence::update(float _deltaTime)
{
	hub->update(_deltaTime);
	if (is_active() && !hub->is_active())
	{
		deactivate();
	}
}

void HubInSequence::display(::System::Video3D* _v3d, bool _vr)
{
	hub->display(_v3d, _vr);
}

bool HubInSequence::activate()
{
	base::activate();
	bool result = hub->activate();
	hub->allow_fade_out(allowFadeOut);
	hub->set_scene(scene.get());
	return result;
}

void HubInSequence::deactivate()
{
	if (hub->does_allow_deactivating_with_loader_immediately())
	{
		//hub->set_scene(nullptr);
	}
	hub->deactivate();
	if (! hub->is_active())
	{
		base::deactivate();
	}
}

