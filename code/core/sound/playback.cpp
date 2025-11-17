#include "playback.h"
#include "sample.h"

#include "soundGlobalOptions.h"

#include "soundFadingUtils.h"

#include "..\casts.h"

#include "..\math\math.h"
#include "..\random\random.h"

#include "..\system\core.h"
#include "..\system\sound\soundSystem.h"
#include "..\system\sound\utils.h"

#include "..\debug\extendedDebug.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef INVESTIGATE_SOUND_SYSTEM
	#define INSPECT_CHANNEL
	#define INSPECT_PLAYBACK_VOLUME
	#define INSPECT_3D_INFO
#else
	#ifdef AN_DEVELOPMENT_OR_PROFILER
		//#define INSPECT_CHANNEL
	#endif
#endif

#define TIME_FADED_OUT 0.1f

//

using namespace Sound;

//

#define CHECK_IF_VALID() \
	if (!is_valid()) \
	{ \
		return; \
	}

#define CHECK_IF_VALID_RETURN(_value) \
	if (!is_valid()) \
	{ \
		return _value; \
	}

Playback::ChannelControlID Playback::s_systemChannelControlID = 0;

Playback::Playback()
{
}

Playback::Playback(Playback const & _other)
{
#ifdef AN_FMOD
	if (systemChannel != _other.systemChannel &&
		systemChannel)
	{
		warn(TXT("decouple playback first!"));
		// we won't be playing on that channel!
		systemChannel->stop();
	}
#endif
#ifdef INVESTIGATE_SOUND_SYSTEM
	devInfo = _other.devInfo;
#endif
	sample = _other.sample;
	sampleVolume = _other.sampleVolume;
	samplePitch = _other.samplePitch;
	channel = _other.channel;
#ifdef AN_FMOD
	systemChannel = _other.systemChannel;
	systemChannelControl = _other.systemChannelControl;
#ifdef AN_SOUND_LOUDNESS_METER
	loudnessMeter = _other.loudnessMeter;
#endif
#endif
	systemChannelControlID = _other.systemChannelControlID;
	volume = _other.volume;
	pitch = _other.pitch;
	useOwnFading = _other.useOwnFading;
	prevFade = _other.prevFade;
	currentFade = _other.currentFade;
	targetFade = _other.targetFade;
	timeFadedOut = _other.timeFadedOut;
	fadeTimeLeft = _other.fadeTimeLeft;
	justStarted = _other.justStarted;
#ifdef SOUND_SKIP_FADE_ON_FIRST_ADVANCEMENT
	firstAdvancement = _other.firstAdvancement;
#endif
	// reset
	sentVolume = -1.0f;
	sendPitch = -1.0f;
}

Playback & Playback::operator=(Playback const & _other)
{
#ifdef AN_FMOD
	if (systemChannel != _other.systemChannel &&
		systemChannel)
	{
		warn(TXT("decouple playback first!"));
		// we won't be playing on that channel!
		systemChannel->stop();
	}
#endif
#ifdef INVESTIGATE_SOUND_SYSTEM
	devInfo = _other.devInfo;
#endif
	sample = _other.sample;
	sampleVolume = _other.sampleVolume;
	samplePitch = _other.samplePitch;
	channel = _other.channel;
#ifdef AN_FMOD
	systemChannel = _other.systemChannel;
	systemChannelControl = _other.systemChannelControl;
#ifdef AN_SOUND_LOUDNESS_METER
	loudnessMeter = _other.loudnessMeter;
#endif
#endif
	systemChannelControlID = _other.systemChannelControlID;
	volume = _other.volume;
	pitch = _other.pitch;
	useOwnFading = _other.useOwnFading;
	prevFade = _other.prevFade;
	currentFade = _other.currentFade;
	targetFade = _other.targetFade;
	timeFadedOut = _other.timeFadedOut;
	fadeTimeLeft = _other.fadeTimeLeft;
	justStarted = _other.justStarted;
#ifdef SOUND_SKIP_FADE_ON_FIRST_ADVANCEMENT
	firstAdvancement = _other.firstAdvancement;
#endif
	// reset
	sentVolume = -1.0f;
	sendPitch = -1.0f;
	return *this;
}

/**
 *	Note: Playback tries to have always two fade points top.
 *	It makes it easier with such assumption but adding fade points by hand should be avoided.
 */

void Playback::fade_in_on_start(float _time)
{
#ifdef INSPECT_CHANNEL
	output(TXT("[playback] sc:%p \"%S\" fade in on start %.3f"), systemChannel, devInfo.to_char(), _time);
#endif
	CHECK_IF_VALID();
	if (useOwnFading)
	{
		currentFade = _time > 0.0f? 0.0f : 1.0f;
		prevFade = 0.0f; // mark we're fading in!
		targetFade = 1.0f;
		timeFadedOut = 0.0f;
		fadeTimeLeft = _time;
		update_volume();
		update_pitch();
	}
	else
	{
		update_volume();
		update_pitch();
#ifdef AN_FMOD
		FadingUtils::FMOD::fade_in_on_start(systemChannelControl, 1.0f, _time);
#endif
	}

}

void Playback::fade_in(float _time)
{
#ifdef INSPECT_CHANNEL
	output(TXT("[playback] sc:%p \"%S\" fade in %.3f"), systemChannel, devInfo.to_char(), _time);
#endif
	CHECK_IF_VALID();
	if (useOwnFading)
	{
		if (_time == 0.0f)
		{
			currentFade = 1.0f;
		}
		targetFade = 1.0f;
		timeFadedOut = 0.0f;
		fadeTimeLeft = _time * (1.0f - currentFade);
		update_volume();
		update_pitch();
	}
	else
	{
		update_volume();
		update_pitch();
#ifdef AN_FMOD
		FadingUtils::FMOD::fade_to(systemChannelControl, 1.0f, _time, true);
#endif
	}
}

void Playback::fade_out(float _time)
{
#ifdef INSPECT_CHANNEL
	output(TXT("[playback] sc:%p \"%S\" fade out %.3f"), systemChannel, devInfo.to_char(), _time);
#endif
	CHECK_IF_VALID();
	if (useOwnFading)
	{
		if (currentFade != 0.0f)
		{
			timeFadedOut = 0.0f;
		}
		if (_time == 0.0f)
		{
			currentFade = 0.0f;
		}
		keepAfterFadeOut = false;
		targetFade = 0.0f;
		fadeTimeLeft = _time * currentFade;
		update_volume();
		update_pitch();
	}
	else
	{
		update_volume();
		update_pitch();
#ifdef AN_FMOD
		FadingUtils::FMOD::fade_to(systemChannelControl, 0.0f, _time, false);
#endif
	}
}

void Playback::fade_to(float _volume, float _time)
{
#ifdef INSPECT_CHANNEL
	output(TXT("[playback] sc:%p \"%S\" fade to %.3f, time %.3f"), systemChannel, devInfo.to_char(), _volume, _time);
#endif
	CHECK_IF_VALID();
	if (useOwnFading)
	{
		if (currentFade != 0.0f || targetFade != 0.0f)
		{
			timeFadedOut = 0.0f;
		}
		if (_time == 0.0f)
		{
			currentFade = targetFade;
		}
		targetFade = _volume;
		fadeTimeLeft = _time * abs(targetFade - currentFade);
		update_volume();
		update_pitch();
	}
	else
	{
		update_volume();
		update_pitch();
#ifdef AN_FMOD
		FadingUtils::FMOD::fade_to(systemChannelControl, _volume, _time, true);
#endif
	}
}

void Playback::fade_out_and_keep(float _time)
{
#ifdef INSPECT_CHANNEL
	output(TXT("[playback] sc:%p \"%S\" fade out and keep %.3f"), systemChannel, devInfo.to_char(), _time);
#endif
	CHECK_IF_VALID();
	if (useOwnFading)
	{
		if (currentFade != 0.0f)
		{
			timeFadedOut = 0.0f;
		}
		keepAfterFadeOut = true;
		targetFade = 0.0f;
		fadeTimeLeft = _time * currentFade;
		update_volume();
		update_pitch();
	}
	else
	{
		update_volume();
		update_pitch();
#ifdef AN_FMOD
		FadingUtils::FMOD::fade_to(systemChannelControl, 0.0f, _time, true);
#endif
	}
}

PlaybackState::Type Playback::get_state() const
{
	if (is_playing())
	{
		if (useOwnFading)
		{
			if (currentFade != targetFade && fadeTimeLeft > 0.0f)
			{
				return currentFade < targetFade ? PlaybackState::FadingIn : PlaybackState::FadingOut;
			}
			if (currentFade == 0.0f)
			{
				return timeFadedOut <= TIME_FADED_OUT ? PlaybackState::AlmostSilence : PlaybackState::Silence;
			}
			return PlaybackState::Playing;
		}
		else
		{
#ifdef AN_FMOD
			unsigned long long dspClock, parentClock;
			systemChannelControl->getDSPClock(&dspClock, &parentClock);

			return get_state(dspClock);
#endif
			return PlaybackState::Silence;
		}
	}
	else
	{
		return PlaybackState::Silence;
	}
}

#ifdef AN_SOUND_LOUDNESS_METER
Optional<float> Playback::get_loudness() const
{
	Optional<float> result;
	result = loudnessMeter.get_loudness();
	return result;
}

Optional<float> Playback::get_recent_max_loudness() const
{
	Optional<float> result;
	loudnessMeter.update_recent_max_loudness();
	result = loudnessMeter.get_recent_max_loudness();
	return result;
}
#endif

float Playback::calculate_current_fade_volume() const
{
	if (useOwnFading)
	{
		return currentFade;
	}
	else
	{
#ifdef AN_FMOD
		return FadingUtils::FMOD::calculate_current_fade_volume(systemChannelControl);
#endif
		return 0.0f;
	}
}

PlaybackState::Type Playback::get_state(uint64 _dspClock) const
{
#ifdef AN_FMOD
	if (systemChannelControl)
	{
		if (useOwnFading)
		{
			return get_state();
		}
		else
		{
			unsigned long long dspClocks[2];
			float volumes[2];
			uint fadePointCount = 0;
			systemChannelControl->getFadePoints(&fadePointCount, dspClocks, volumes);
			an_assert(fadePointCount <= 2);
			if (volumes[0] != volumes[1])
			{
				if (fadePointCount == 2)
				{
					if (_dspClock <= dspClocks[0])
					{
						// fading in as we're about to fade in!
						return volumes[0] < volumes[1] ? PlaybackState::FadingIn : PlaybackState::Playing;
					}
					else if (_dspClock >= dspClocks[1])
					{
						return volumes[0] < volumes[1] ? PlaybackState::Playing : PlaybackState::Silence;
					}
					else
					{
						return volumes[0] < volumes[1] ? PlaybackState::FadingIn : PlaybackState::FadingOut;
					}
				}
			}
			else if (volumes[0] == 0.0f)
			{
				return PlaybackState::Silence;
			}
			return PlaybackState::Playing;
		}
	}
#endif
	return PlaybackState::Silence;
}

float Playback::calculate_current_fade_volume(uint64 _dspClock) const
{
	if (useOwnFading)
	{
		return currentFade;
	}
	else
	{
#ifdef AN_FMOD
		return FadingUtils::FMOD::calculate_current_fade_volume(systemChannelControl, _dspClock);
#endif
		return 0.0f;
	}
}

bool Playback::is_playing() const
{
	CHECK_IF_VALID_RETURN(false);
#ifdef AN_FMOD
	if (systemChannelControl)
	{
		bool isPlaying;
		if (systemChannelControl->isPlaying(&isPlaying) == FMOD_OK)
		{
			return isPlaying;
		}
	}
#endif
	return false;
}

bool Playback::is_fading_out_or_faded_out() const
{
	auto s = get_state();
	return s == PlaybackState::FadingOut || s == PlaybackState::Silence;
}

void Playback::decouple()
{
	sample = nullptr;
	channel = nullptr;
#ifdef AN_FMOD
#ifdef INSPECT_CHANNEL
	output(TXT("[playback] sc:%p \"%S\" decouple"), systemChannel, devInfo.to_char());
#endif
	systemChannel = nullptr;
	systemChannelControl = nullptr;
#ifdef AN_SOUND_LOUDNESS_METER
	loudnessMeter.decouple();
#endif
#endif
#ifdef INVESTIGATE_SOUND_SYSTEM
	devInfo = String::empty();
#endif
	systemChannelControlID = NONE;
}

void Playback::stop()
{
	CHECK_IF_VALID();
#ifdef AN_FMOD
#ifdef INSPECT_CHANNEL
	output(TXT("[playback] sc:%p \"%S\" stop"), systemChannel, devInfo.to_char());
#endif
	if (systemChannelControl)
	{
		systemChannelControl->stop();
	}
#endif
}

void Playback::pause()
{
	CHECK_IF_VALID();
#ifdef AN_FMOD
#ifdef INSPECT_CHANNEL
	output(TXT("[playback] sc:%p \"%S\" pause"), systemChannel, devInfo.to_char());
#endif
	if (systemChannelControl)
	{
		systemChannelControl->setPaused(true);
	}
#endif
}

void Playback::resume()
{
	CHECK_IF_VALID();
	justStarted = false;
#ifdef AN_FMOD
#ifdef INSPECT_CHANNEL
	output(TXT("[playback] sc:%p \"%S\" resume"), systemChannel, devInfo.to_char());
#endif
	if (systemChannelControl)
	{
		systemChannelControl->setPaused(false);
		systemChannelControl->setVolumeRamp(true);
	}
#endif
}

void Playback::resume_with_fade_in()
{
	CHECK_IF_VALID();
	justStarted = false;
#ifdef AN_FMOD
#ifdef INSPECT_CHANNEL
	output(TXT("[playback] sc:%p \"%S\" resume with fade in"), systemChannel, devInfo.to_char());
#endif
	if (systemChannelControl)
	{
		systemChannelControl->setVolumeRamp(false);
		if (sentVolume != -1.0f)
		{
			systemChannelControl->setVolume(0.0f);
		}
		systemChannelControl->setPaused(false);
		if (sentVolume != -1.0f)
		{
			systemChannelControl->setVolumeRamp(true);
			systemChannelControl->setVolume(sentVolume);
		}
	}
#endif
}

Playback::ChannelControlID Playback::get_new_channel_control_id()
{
	static Concurrency::SpinLock systemChannelControlIDLock = Concurrency::SpinLock(TXT("Sound.Playback.systemChannelControlIDLock"));
	systemChannelControlIDLock.acquire();
	++s_systemChannelControlID;
	if (s_systemChannelControlID == NONE)
	{
		++s_systemChannelControlID;
	}
	Playback::ChannelControlID returnValue = s_systemChannelControlID;
	systemChannelControlIDLock.release();
	return returnValue;
}

void Playback::set_new_sample(Sample const * _sample, Optional<bool> const& _forceOwnFading)
{
	sample = _sample;
	sampleVolume = sample ? Random::get(sample->get_volume()) : 1.0f;
	samplePitch = sample ? Random::get(sample->get_pitch()) : 1.0f;
	useOwnFading = _forceOwnFading.get(sample && (sample->get_params().is3D || sample->get_params().useAs3D));
}

#ifdef AN_FMOD
void Playback::set_new_channel(Channel const * _channel, FMOD::Channel* _systemChannel)
{
	an_assert(_systemChannel, TXT("there is no sound or not yet loaded"));
	channel = _channel;
	systemChannel = _systemChannel;
	systemChannelControl = _systemChannel;
	systemChannelControlID = get_new_channel_control_id();
	systemChannelControl->setUserData((void*)(systemChannelControlID));
	justStarted = true;
#ifdef SOUND_SKIP_FADE_ON_FIRST_ADVANCEMENT
	firstAdvancement = true;
#endif
#ifdef AN_SOUND_LOUDNESS_METER
	{
		bool addLoudnessMeter = true;
		{
			int dsps = 0;
			if (systemChannelControl->getNumDSPs(&dsps) == FMOD_OK)
			{
				for_count(int, i, dsps)
				{
					FMOD::DSP* dsp = nullptr;
					systemChannelControl->getDSP(i, &dsp);
					FMOD_DSP_TYPE dspType;
					if (dsp->getType(&dspType) == FMOD_OK)
					{
						if (dspType == FMOD_DSP_TYPE_LOUDNESS_METER)
						{
							loudnessMeter.couple(dsp);
							addLoudnessMeter = false;
							break;
						}
					}
				}
			}
		}
		if (addLoudnessMeter)
		{
			loudnessMeter.decouple();
			loudnessMeter.create_and_add_to(&System::Sound::get()->access_low_level_system(), systemChannel);
		}
	}
#endif
	// reset
	sentVolume = -1.0f;
	sendPitch = -1.0f;
#ifdef INSPECT_CHANNEL
	output(TXT("[playback] sc:%p \"%S\" new channel"), systemChannel, devInfo.to_char());
#endif
}
#endif

bool Playback::is_active() const
{
#ifdef AN_FMOD
	if (systemChannel)
	{
		return true;
	}
#endif
	return false;
}

bool Playback::is_valid() const
{
	if (!sample)
	{
		return false;
	}
#ifdef AN_FMOD
	if (systemChannelControl)
	{
		void* userData;
		systemChannelControl->getUserData(&userData);
		return (void*)(systemChannelControlID) == userData;
	}
#endif
	return false;
}

float Playback::get_at() const
{
	CHECK_IF_VALID_RETURN(0.0f);
#ifdef AN_FMOD
	if (systemChannel)
	{
		uint position;
		systemChannel->getPosition(&position, FMOD_TIMEUNIT_MS);
		return ((float)position) * 0.001f;
	}
	else
#endif
	{
		return 0.0f;
	}
}

void Playback::set_at(float _time) const
{
	CHECK_IF_VALID();
#ifdef AN_FMOD
	if (systemChannel)
	{
		if (_time < 0.0f)
		{
			if (sample &&
				sample->is_looped())
			{
				_time = mod(_time, sample->get_length());
			}
			else
			{
				if (auto * ss = ::System::Sound::get())
				{
					float const rate = ss->get_dsp_clock_rate_as_float();
					uint64 startDelay = (int)(-_time * rate);
					systemChannel->setDelay(startDelay, 0, false);
				}
				_time = 0.0f;
			}
		}
#ifdef INSPECT_CHANNEL
		output(TXT("[playback] sc:%p \"%S\" set at time : %.3f"), systemChannel, devInfo.to_char(), _time);
#endif
		uint position = (uint)(_time * 1000.0f);
		systemChannel->setPosition(position, FMOD_TIMEUNIT_MS);
	}
#endif
}

void Playback::advance_fading(float _deltaTime)
{
	if (!useOwnFading)
	{
		return;
	}

	prevFade = currentFade;

	// if we're still starting the sound, make it still being started
#ifdef SOUND_SKIP_FADE_ON_FIRST_ADVANCEMENT
	if (!firstAdvancement)
#endif
	{
		if (fadeTimeLeft <= 0.0f)
		{
			currentFade = targetFade;
		}
		else
		{
			float covered = clamp(_deltaTime / fadeTimeLeft, 0.0f, 1.0f);
			fadeTimeLeft = max(0.0f, fadeTimeLeft - _deltaTime);
			currentFade = currentFade + (targetFade - currentFade) * covered;
		}

		if (targetFade == 0.0f && currentFade == 0.0f)
		{
			if (timeFadedOut > TIME_FADED_OUT && !keepAfterFadeOut) // we need to wait a bit till it completely fades out, otherwise we will have a crackle
			{
				update_volume();
				stop();
			}
			timeFadedOut += _deltaTime;
		}
		else
		{
			timeFadedOut = 0.0f;
		}
	}
#ifdef SOUND_SKIP_FADE_ON_FIRST_ADVANCEMENT
	firstAdvancement = false;
#endif

#ifdef INSPECT_PLAYBACK_VOLUME
	if (sample)
	{
		output(TXT("[playback-advance] sc:%p \"%S\" [%S] at:%.3f/%.3f [%S] fade:%.3f->%.3f->%.3f"), systemChannel, devInfo.to_char(), is_playing() ? TXT("playing") : TXT("stopped"), get_at(), sample->get_length(), sample->is_looped() ? TXT("looped") : TXT("single"), prevFade, currentFade, targetFade);
	}
#endif

}

void Playback::set_3d_min_max_distances(Range const & _minMaxDistances)
{
#ifdef AN_FMOD
	if (systemChannel)
	{
		systemChannel->set3DMinMaxDistance(_minMaxDistances.min, _minMaxDistances.max);
#ifdef INSPECT_3D_INFO
		output(TXT("[playback-volume] sc:%p \"%S\" distances %.3f -> %.3f"), systemChannel, devInfo.to_char(), _minMaxDistances.min, _minMaxDistances.max);
#endif
	}
#endif
}

void Playback::set_sample_volume(float _volume)
{
	sampleVolume = _volume;
	update_volume();
}

void Playback::set_volume(float _volume)
{
	volume = _volume;
	update_volume();
}

void Playback::update_volume()
{
	CHECK_IF_VALID();
#ifdef AN_FMOD
	if (systemChannelControl)
	{
		float newVolume = volume * sampleVolume;
		if (useOwnFading)
		{
			newVolume *= currentFade;
		}
		if (sentVolume != newVolume)
		{
			bool useRamp;
			useRamp = !justStarted; // always use ramps, unless we're really just starting
			//useRamp = currentFade > 0.2f && prevFade > 0.2f && (fadeTimeLeft != 0.0f || prevFade != currentFade); // I want to use ramps but with less restrict rules it still gets that spike in the beginning. That's why there's 0.2 to prevent that spike from being observable
			systemChannelControl->setVolumeRamp(useRamp);
			systemChannelControl->setVolume(newVolume);
#ifdef INSPECT_PLAYBACK_VOLUME
			output(TXT("[playback-volume] sc:%p \"%S\" volume %S to %.3f"), systemChannel, devInfo.to_char(), useRamp? TXT("ramp") : TXT("sharp"), newVolume);
#endif
			sentVolume = newVolume;
		}
	}
#endif
}

void Playback::set_sample_pitch(float _pitch)
{
	samplePitch = _pitch;
	update_pitch();
}

void Playback::set_pitch(float _pitch)
{
	pitch = _pitch;
	update_pitch();
}

void Playback::update_pitch()
{
	CHECK_IF_VALID();
#ifdef AN_FMOD
	if (systemChannelControl)
	{
		float newPitch = pitch * samplePitch * System::Core::get_time_speed();
		if (sendPitch != newPitch)
		{
			systemChannelControl->setPitch(newPitch);
			sendPitch = newPitch;
		}
	}
#endif
}

void Playback::set_3d_params(Vector3 const & _location, Vector3 const & _velocity)
{
	CHECK_IF_VALID();
#ifdef AN_FMOD
	if (systemChannelControl)
	{
#ifdef SOUND_PLAYBACK_USE_ZERO_LOCATION
		FMOD_VECTOR loc = ::System::SoundUtils::convert(Vector3::yAxis);
#else
		FMOD_VECTOR loc = ::System::SoundUtils::convert(_location);
#endif
#ifdef SOUND_PLAYBACK_USE_ZERO_VELOCITY
		FMOD_VECTOR vel;
		vel.x = 0.0f;
		vel.y = 0.0f;
		vel.z = 0.0f;
#else
		FMOD_VECTOR vel = ::System::SoundUtils::convert(_velocity);
#endif
		systemChannelControl->set3DAttributes(&loc, &vel);
#ifdef INSPECT_3D_INFO
		output(TXT("[playback-volume] sc:%p \"%S\" loc %.3f,%.3f,%.3f vel %.3f,%.3f,%.3f"), systemChannel, devInfo.to_char(), loc.x, loc.y, loc.z, vel.x, vel.y, vel.z);
#endif
	}
#endif
}

void Playback::set_low_pass_gain(float _lowPassGain)
{
	CHECK_IF_VALID();
#ifdef AN_FMOD
#ifndef SOUND_NO_LOW_PASS
	if (systemChannelControl)
	{
		systemChannelControl->setLowPassGain(_lowPassGain);
	}
#endif
#endif
}

void Playback::set_use_reverbs(float _usesReverbs)
{
	CHECK_IF_VALID();
#ifdef AN_FMOD
	if (systemChannelControl)
	{
		for_range(int, i, 0, FMOD_REVERB_MAXINSTANCES)
		{
			systemChannelControl->setReverbProperties(i, _usesReverbs);
		}
	}
#endif
}

void Playback::set_3d_occlusion(float _directOcclusion, float _reverbOcclusion)
{
	CHECK_IF_VALID();
#ifdef AN_FMOD
	if (systemChannelControl)
	{
		systemChannelControl->set3DOcclusion(_directOcclusion, _reverbOcclusion);
#ifdef INSPECT_3D_INFO
		output(TXT("[playback-volume] sc:%p \"%S\" occlusion d:%.3f r:%.3f"), systemChannel, devInfo.to_char(), _directOcclusion, _reverbOcclusion);
#endif
	}
#endif
}

float Playback::distance_to_time(float _distance) const
{
	return 0.0f;
	todo_note(TXT("for time being don't alter time with distance"));
	return -_distance / 300.0f;
}

bool Playback::is_3d_sample() const
{
	return sample ? sample->get_params().is3D : false;
}
