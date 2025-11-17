#pragma once

#include "soundCompressorConfig.h"

#include "..\..\containers\array.h"
#include "..\..\containers\map.h"
#include "..\..\types\name.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace System
{
	struct SoundConfig
	{
		static int const defaultMaxConcurrentSounds = 512;

		Array<String> preferredDevices;
		Array<String> preferredDevicesForRecording;

		struct Channel
		{
			float overallVolume = 1.0f; // from main settings (as defined to make some channels louder or quieter)
			float volume = 1.0f;
		};
		// main/general
#ifdef AN_PICO
		float volume = 0.5f;
#else
		float volume = 1.0f;
#endif
		Map<Name, Channel> channels;
		bool enableOcclusionLowPass = true;
		int maxConcurrentSounds = defaultMaxConcurrentSounds;

#ifdef AN_DEVELOPMENT_OR_PROFILER
		float developmentVolumeVR = 1.0f;
		float developmentVolumeNonVR = 1.0f;
		float developmentVolume = 1.0f; // when decided which one should be used

		void decide_development_volume(bool _vr) { developmentVolume = _vr ? developmentVolumeVR : developmentVolumeNonVR; }
#endif

		SoundConfig();

		bool is_option_set(Name const & _option) const;

		void fill_missing();

		bool load_from_xml(IO::XML::Node const * _node);
		void save_to_xml(IO::XML::Node * _node, bool _userConfig = false) const;
	};

};
