#include "ac_particlesUtils.h"

#include "ac_particlesBase.h"

#include "..\..\module\moduleAppearance.h"
#include "..\..\module\moduleSound.h"
#include "..\..\module\custom\mc_lightSources.h"
#include "..\..\modulesOwner\modulesOwner.h"
#include "..\..\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\object\temporaryObject.h"

//

using namespace Framework;

//

void ParticlesUtils::desire_to_deactivate(IModulesOwner* _imo, bool _attached)
{
	if (_imo)
	{
		if (auto* to = _imo->get_as_temporary_object())
		{
			to->mark_desired_to_deactivate();
		}
		if (auto* sound = _imo->get_sound())
		{
			sound->stop_looped_sounds();
		}

		for_every_ptr(appearance, _imo->get_appearances())
		{
			for_every_ref(controller, appearance->get_controllers())
			{
				if (auto* pc = fast_cast<AppearanceControllersLib::ParticlesBase>(controller))
				{
					pc->desire_to_deactivate();
				}
			}
		}

		if (auto* ls = _imo->get_custom<CustomModules::LightSources>())
		{
			ls->fade_out_all_on_particles_deactivation();
		}

		if (_attached)
		{
			Concurrency::ScopedSpinLock lock(_imo->get_presence()->access_attached_lock());
			for_every_ptr(at, _imo->get_presence()->get_attached())
			{
				desire_to_deactivate(at, _attached);
			}
		}
	}
}
