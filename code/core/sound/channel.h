#pragma once

#ifdef AN_FMOD
#include "fmod.hpp"
#include "fmod_studio.hpp"
#endif

#include "../tags/tag.h"
#include "../system/timeStamp.h"

#ifdef BUILD_NON_RELEASE
#define AN_SOUND_LOUDNESS_METER
#endif

namespace Sound
{
	/**
	 *	It's actually just channel handler to allow adding sounds to specific channels
	 */

	struct LoudnessMeter
	{
#ifdef AN_FMOD
		FMOD::DSP* meterDSP = nullptr;
		void couple(FMOD::DSP* _meterDSP) { meterDSP = _meterDSP; }
		void decouple();
		void create_and_add_to(FMOD::System* _lowLevelSystem, FMOD::ChannelControl* _channel);
#endif
		mutable float recentMaxLoudness = -80.0f;
		mutable System::TimeStamp recentMaxLoudnessTS;
		void update_recent_max_loudness() const;
		float get_recent_max_loudness() const { return recentMaxLoudness; }

		float get_loudness() const;

		static Optional<Colour> loudness_to_colour(float _loudness);
	};

	struct Channel
	{
		Name name;
		Tags tags;
#ifdef AN_FMOD
		FMOD::ChannelGroup* channelGroup = nullptr;
#endif
		LoudnessMeter loudnessMeter;

		bool usesReverbs = true;
		RUNTIME_ float setVolume = 0.0f; // as it is set in config + overall volume - should be updated as soon as possible
		RUNTIME_ int audioDuck = 0; // 1 will use normal audio duck level, 2 will use lower, 3 even lower
#ifdef SOUND_SYSTEM_OWN_FADING
		RUNTIME_ float usedSetVolume = -1.0f; // to force it to be set
		// change times are 0 to 1, 1 to 0 so in most cases they have to be altered to match target volume
		RUNTIME_ float fadeVolume = 0.0f;
		RUNTIME_ float fadeVolumeTarget = 1.0f;
		RUNTIME_ float fadeVolumeChangeTime = 0.1f;
		RUNTIME_ float audioDuckVolume = 0.0f;
		RUNTIME_ float audioDuckVolumeTarget = 1.0f;
		RUNTIME_ float audioDuckVolumeChangeTime = 0.1f;
		struct CustomVolume
		{
			Name id;
			float volume = 1.0f;
		};
		ArrayStatic<CustomVolume, 8> customVolumes;

		RUNTIME_ float usedSetFinalVolume = 1.0f;
		float calculate_final_volume() const;
#else
		RUNTIME_ float volumeLevel = 1.0f;
#endif

		Channel() {}
		Channel(Name const& _name, Tags const& _tags, bool _usesReverbs) : name(_name), tags(_tags), usesReverbs(_usesReverbs) {}
		~Channel() { }

#ifdef AN_FMOD
		Channel& with(FMOD::ChannelGroup* _channelGroup) { channelGroup = _channelGroup; return *this; }
#endif
	};
};
