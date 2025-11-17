#include "reverbInstance.h"

#include "reverb.h"

#include "..\concurrency\scopedSpinLock.h"
#include "..\system\sound\soundSystem.h"
#include "..\system\sound\utils.h"

//

using namespace ::Sound;

//

void ReverbInstance::start(Reverb const * _reverb)
{
#ifdef AN_FMOD
	if (!reverb3D)
	{
		if (auto * ss = ::System::Sound::get())
		{
			auto& lowLevelSystem = ss->access_low_level_system();
			Concurrency::ScopedSpinLock lock(ss->access_loading_lock());

			lowLevelSystem.createReverb3D(&reverb3D);

#ifdef INVESTIGATE_SOUND_SYSTEM
			output(TXT("[reverb] r:%p start"), reverb3D);
#endif
		}
	}

	if (reverb3D)
	{
		if (_reverb)
		{
			reverb3D->setProperties(&_reverb->properties);
		}
		else
		{
			FMOD_REVERB_PROPERTIES properties = FMOD_PRESET_OFF;
			reverb3D->setProperties(&properties);
		}
	}
#endif
	power = _reverb ? _reverb->power : 1.0f;

	set_active(0.0f, true);
}

void ReverbInstance::stop()
{
	set_active(0.0f, true);

#ifdef AN_FMOD
	if (reverb3D)
	{
#ifdef INVESTIGATE_SOUND_SYSTEM
		output(TXT("[reverb] r:%p stop"), reverb3D);
#endif

		reverb3D->release();
		reverb3D = nullptr;

		if (auto * ss = System::Sound::get())
		{
			ss->update_reverbs();
		}
	}
#endif
}

void ReverbInstance::set_active(float _active, bool _force)
{
	if (active == _active && ! _force)
	{
		return;
	}
	
	active = _active;

	update_active();
}

void ReverbInstance::update_active()
{
#ifdef AN_FMOD
	if (reverb3D)
	{
		// listener is at 0,0,0
		float useActive = clamp(active * power, 0.0f, 1.0f);
		FMOD_VECTOR loc = ::System::SoundUtils::convert(Vector3(0.0f, 1.0f - useActive, 0.0f));
		float const radius = 0.7f;
		reverb3D->set3DAttributes(&loc, 0.0f, radius);

#ifdef INVESTIGATE_SOUND_SYSTEM
		output(TXT("[reverb] r:%p use active %.3f (%.3f)"), reverb3D, loc.y, radius);
#endif

		if (auto * ss = System::Sound::get())
		{
			ss->update_reverbs();
		}
	}
#endif
}

