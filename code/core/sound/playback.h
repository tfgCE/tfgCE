#pragma once

#include "..\globalDefinitions.h"
#include "..\math\math.h"
#include "soundGlobalOptions.h"

#include "channel.h"

#ifdef AN_FMOD
#include "fmod.hpp"
#include "fmod_studio.hpp"
#endif

namespace Sound
{
	class Sample;
	struct Channel;
		
	namespace PlaybackState
	{
		enum Type
		{
			FadingIn,
			Playing,
			FadingOut,
			AlmostSilence,
			Silence,
		};
	};

	struct PlaybackSetup
	{
		Optional<bool> useOwnFading;

		PlaybackSetup& with_own_fading(bool _useOwnFading = true) { useOwnFading = _useOwnFading; return *this; }
	};

	/**
	 *	It's structure that allows manipulating fmod playback, it's lightweight as it is just wrapper/handler of channel control.
	 *	If you create a copy, you have to wrappers to the same channel.
	 *	If you destroy it, it won't stop the channel, it will continue playing.
	 *	If you copy one playback to another, it may stop the previous playback. Either decouple or make sure you stopped it.
	 *	It allows for setting values, fading, etc. Always played.
	 */
	struct Playback
	{
	public:
		Playback();
		Playback(Playback const & _other);
		Playback & operator=(Playback const & _other);

#ifdef INVESTIGATE_SOUND_SYSTEM
		void set_dev_info(tchar const* _info) { devInfo = _info; }
		void set_dev_info(String const & _info) { devInfo = _info; }
#endif

		void fade_in_on_start(float _time); // sets initial volume to 0
		void fade_in(float _time);
		void fade_out(float _time);
		void fade_out_and_keep(float _time); // fade out but keep playing
		void fade_to(float _volume, float _time);

		PlaybackState::Type get_state() const;

		void decouple(); // allow channel to continue playing but make this playback no longer connected
		void stop();
		void pause();
		void resume();
		void resume_with_fade_in();

		bool is_active() const; // is connected to any channel/requires decoupling
		bool is_valid() const; // checks channel control id

		bool is_playing() const;
		bool is_fading_out_or_faded_out() const;

		float get_at() const;
		void set_at(float _time) const;

		float get_current_fade() const { return calculate_current_fade_volume(); }
		float get_current_volume() const { return get_current_fade() * sampleVolume * volume; }
		float get_current_pitch() const { return samplePitch * pitch; }

#ifdef AN_SOUND_LOUDNESS_METER
		Optional<float> get_loudness() const;
		Optional<float> get_recent_max_loudness() const;
#endif

		void set_3d_min_max_distances(Range const & _minMaxDistances);

		void set_volume(float _volume = 1.0f);
		void set_sample_volume(float _volume = 1.0f);
		void set_pitch(float _pitch = 1.0f);
		void set_sample_pitch(float _pitch = 1.0f);
		void update_volume();
		void update_pitch();
		void advance_fading(float _deltaTime);

		bool is_3d_sample() const;
		void set_3d_params(Vector3 const & _location, Vector3 const & _velocity = Vector3::zero);
		void set_3d_occlusion(float _directOcclusion, float _reverbOcclusion);
		
		void set_low_pass_gain(float _lowPassGain);

		void set_use_reverbs(float _usesReverbs);

		float distance_to_time(float _distance) const;

		Sample const * get_sample() const { return sample; }
		Channel const * get_channel() const { return channel; }
		
	private: friend class Sample;
		// should be called only when new sound (or event instance etc) has started
		void set_new_sample(Sample const * _sample, Optional<bool> const& _forceOwnFading);
#ifdef AN_FMOD
		void set_new_channel(Channel const * _channel, FMOD::Channel* _systemChannel);
#endif

	private:
#ifdef AN_32
		typedef int ChannelControlID;
#else
		typedef uint64 ChannelControlID;
#endif
		static ChannelControlID s_systemChannelControlID;

#ifdef INVESTIGATE_SOUND_SYSTEM
		String devInfo;
#else
#ifdef AN_DEVELOPMENT_OR_PROFILER
		String devInfo;
#endif
#endif

		// remember about constructor and operator!
		Sample const * sample = nullptr;
		float sampleVolume = 1.0f;
		float samplePitch = 1.0f;
		bool justStarted = false;
#ifdef SOUND_SKIP_FADE_ON_FIRST_ADVANCEMENT
		bool firstAdvancement = false;
#endif
		Channel const* channel = nullptr;
#ifdef AN_FMOD
		FMOD::Channel* systemChannel = nullptr;
		FMOD::ChannelControl* systemChannelControl = nullptr;
#ifdef AN_SOUND_LOUDNESS_METER
		LoudnessMeter loudnessMeter;
#endif
#endif
		ChannelControlID systemChannelControlID = NONE;
		float volume = 1.0f; // this is volume of this playback - sample's volume is just for internal things
		float pitch = 1.0f;

		// some sounds use own fading (they need to have advance_fading called)
		bool useOwnFading = false;
		float prevFade = 0.0f;
		float currentFade = 0.0f;
		float targetFade = 0.0f;
		float timeFadedOut = 0.0f;
		float fadeTimeLeft = 0.0f;
		bool keepAfterFadeOut = false;

		float sentVolume = -1.0f;
		float sendPitch = -1.0f;

		PlaybackState::Type get_state(uint64 _dspClock) const;
		float calculate_current_fade_volume() const; // returns fading volume (in <0 - 1> range)
		float calculate_current_fade_volume(uint64 _dspClock) const;

		static ChannelControlID get_new_channel_control_id();
	};
};
