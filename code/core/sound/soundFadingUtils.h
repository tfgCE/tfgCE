#pragma once

#include "..\globalDefinitions.h"

#ifdef AN_FMOD
#include "fmod.hpp"
#include "fmod_studio.hpp"
#endif

namespace Sound
{
	/**
	 *	It's actually just channel handler to allow adding sounds to specific channels
	 */
	namespace FadingUtils
	{
#ifdef AN_FMOD
		namespace FMOD
		{
			void update_delay(::FMOD::ChannelControl* _channelControl, float _time, bool _keepSounds);

			void fade_in_on_start(::FMOD::ChannelControl* _channelControl, float _volume, float _time); // sets initial volume to 0
			void fade_to(::FMOD::ChannelControl* _channelControl, float _volume, float _time, bool _keepSoundOnFadeOut = false);

			float calculate_current_fade_volume(::FMOD::ChannelControl* _channelControl);
			float calculate_current_fade_volume(::FMOD::ChannelControl* _channelControl, unsigned long long _dspClock);

			float get_fading_time_left(::FMOD::ChannelControl* _channelControl);
		};
#endif
	};
};
