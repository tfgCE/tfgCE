#include "soundFadingUtils.h"

#include "..\math\math.h"

#include "..\system\sound\soundSystem.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Sound;

//

#ifdef AN_FMOD
void FadingUtils::FMOD::fade_in_on_start(::FMOD::ChannelControl* _channelControl, float _volume, float _time)
{
	if (_channelControl)
	{
		unsigned long long parentClock;
		_channelControl->getDSPClock(nullptr, &parentClock);

		if (auto * ss = ::System::Sound::get())
		{
			float const rate = ss->get_dsp_clock_rate_as_float();
			_channelControl->removeFadePoints(0, (unsigned long long)-1); // all
			_channelControl->addFadePoint(parentClock, 0.0f);
			unsigned long long end = parentClock + (int)(max(0.001f, _time) * rate);
			_channelControl->addFadePoint(end, _volume);
		}
	}
}

void FadingUtils::FMOD::fade_to(::FMOD::ChannelControl* _channelControl, float _volume, float _time, bool _keepSoundOnFadeOut)
{
	if (_channelControl)
	{
		unsigned long long parentClock;
		_channelControl->getDSPClock(nullptr, &parentClock);

		if (auto * ss = ::System::Sound::get())
		{
			float const rate = ss->get_dsp_clock_rate_as_float();
			float currentFadeVolume = calculate_current_fade_volume(_channelControl, parentClock);
			_channelControl->removeFadePoints(0, (unsigned long long)-1); // all
			_channelControl->addFadePoint(parentClock, currentFadeVolume);
			unsigned long long end = parentClock + (int)(max(0.001f, _time) * rate);
			_channelControl->addFadePoint(end, _volume);
			if (_volume == 0.0f && !_keepSoundOnFadeOut)
			{
				// fading out, stop sound
				_channelControl->setDelay(0, end, true);
			}
		}
	}
}

float FadingUtils::FMOD::calculate_current_fade_volume(::FMOD::ChannelControl* _channelControl)
{
	if (_channelControl)
	{
		unsigned long long parentClock;
		_channelControl->getDSPClock(nullptr, &parentClock);

		return calculate_current_fade_volume(_channelControl, parentClock);
	}
	else
	{
		return 1.0f;
	}
}

float FadingUtils::FMOD::calculate_current_fade_volume(::FMOD::ChannelControl* _channelControl, unsigned long long _dspClock)
{
	if (_channelControl)
	{
		unsigned long long dspClocks[2];
		float volumes[2];
		uint fadePointCount = 0;
		_channelControl->getFadePoints(&fadePointCount, nullptr, nullptr); // get fade point count
		an_assert(fadePointCount <= 2);
		if (fadePointCount > 0)
		{
			_channelControl->getFadePoints(&fadePointCount, dspClocks, volumes);
		}
		if (fadePointCount == 2)
		{
			if (_dspClock <= dspClocks[0])
			{
				return volumes[0];
			}
			else if (_dspClock >= dspClocks[1])
			{
				return volumes[1];
			}
			else
			{
				float pt = clamp((float)(_dspClock - dspClocks[0]) / (float)(dspClocks[1] - dspClocks[0]), 0.0f, 1.0f);
				return volumes[0] * (1.0f - pt) + volumes[1] * pt;
			}
		}
		return 1.0f;
	}
	return 1.0f;
}


float FadingUtils::FMOD::get_fading_time_left(::FMOD::ChannelControl* _channelControl)
{
	if (_channelControl)
	{
		unsigned long long parentClock;
		_channelControl->getDSPClock(nullptr, &parentClock);

		unsigned long long dspClocks[2];
		float volumes[2];
		uint fadePointCount = 0;
		_channelControl->getFadePoints(&fadePointCount, dspClocks, volumes);

		if (volumes[0] != volumes[1])
		{
			if (fadePointCount == 2)
			{
				// assume that we're fading now - sort of ok
				if (auto * ss = ::System::Sound::get())
				{
					float const rate = ss->get_dsp_clock_rate_as_float();
					return (float)(volumes[1] - parentClock) / rate;
				}
			}
		}
	}
	return 0.0f;
}
#endif
