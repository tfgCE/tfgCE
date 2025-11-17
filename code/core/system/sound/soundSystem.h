#pragma once

#ifdef AN_FMOD
#include "fmod.hpp"
#include "fmod_studio.hpp"
#endif

#include "..\..\containers\arrayStatic.h"
#include "..\..\math\math.h"
#include "..\..\sound\channel.h"
#include "..\..\types\name.h"
#include "..\..\tags\tagCondition.h"

class TagCondition;

namespace System
{
	struct SoundConfig;

	/**
	 *	We are running studio in synchronous mode as we don't want to create even more threads. It will be enough to have one more thread for fmod to handle mixing.
	 */
	class Sound
	{
	public:
		static void create(SoundConfig const * _sc = nullptr);
		static void terminate();
		static Sound* get() { return s_current && s_current->isValid ? s_current : nullptr; }

#ifdef AN_WINDOWS
		static void set_preferred_audio_device(GUID _guid); // has to be called before creation!
#endif

	public:
		void update(float _deltaTime);
		void update_volumes();
		void update_reverbs(); // which channels are affected by reverbs
		void update_audio_duck();

		void set_listener_3d_params(Vector3 const & _location, Vector3 const & _velocity = Vector3::zero);

		// _fadeTime is from current to target
		void fade_in(float _fadeTime, Optional<Name> const & _channel = NP);
		void fade_out(float _fadeTime, Optional<Name> const & _channel = NP);
		void fade_to(float _target, float _fadeTime, Optional<Name> const & _channel = NP);

		void fade_in(float _fadeTime, TagCondition const & _channelsTagged);
		void fade_out(float _fadeTime, TagCondition const & _channelsTagged);
		void fade_to(float _target, float _fadeTime, TagCondition const & _channelsTagged);
		float get_fade(TagCondition const& _channelsTagged) const;
		
		void set_custom_volume(TagCondition const & _tagged, Name const & _customVolumeId, float _volume);
		void clear_custom_volume(Name const & _customVolumeId);
		void clear_custom_volumes();

		void audio_duck(int _audioDuck, TagCondition const & _channelsTagged);
		void audio_duck_off_all();

#ifdef AN_DEVELOPMENT_OR_PROFILER
		void update_on_development_volume_change();
#endif

#ifdef AN_SOUND_LOUDNESS_METER
		Optional<float> get_loudness() const;
		Optional<float> get_recent_max_loudness() const;
#endif

	public:
#ifdef AN_FMOD
		FMOD::Studio::System& access_system() { return *fmodSystem; }
		FMOD::System& access_low_level_system() { return *fmodLowLevelSystem; }
		int get_dsp_clock_rate() const { return dspClockRate; }
		float get_dsp_clock_rate_as_float() const { return dspClockRateAsFloat; }
#endif

		::Sound::Channel const * get_channel(Name const & _name) const;

		Concurrency::SpinLock& access_loading_lock() { return loadingLock; }

		void log(LogInfoContext& _log) const;

	protected:
		Sound(SoundConfig const * _sc);
		~Sound();

	private:
#ifdef AN_WINDOWS
		static GUID s_preferredGuid; // preferred guid is stored here as it might be only overridden by other systems
#endif
		static Sound* s_current;

		bool isValid = false;

		ArrayStatic<::Sound::Channel, 8> channels;
		CACHED_ float audioDuckSourceChannelVolume = 1.0f;

		Concurrency::SpinLock loadingLock; // load/create only one sample at a time

#ifdef AN_FMOD
		FMOD::Studio::System* fmodSystem = nullptr;
		FMOD::System* fmodLowLevelSystem = nullptr;
		FMOD::ChannelGroup* mainChannelGroup = nullptr;
		int dspClockRate = 0;
		float dspClockRateAsFloat = 0.0f;
#endif
		::Sound::LoudnessMeter mainLoudnessMeter;

		float currentVolume = 0.0f;
		float targetVolume = 1.0f; // this is to control the main volume - if we take the headset off we want to lower the volume
		float volumeChangeTime = 0.5f; // this is change from 0 to 1, 1 to 0, only for main volume

		void clear_channels();

		float get_audio_duck_level(int _audioDuckLevel);
		static float get_audio_duck_fade_time();
	};

};
