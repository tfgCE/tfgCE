#pragma once

namespace Random
{
	class Generator;
};

namespace Sound
{
	class Sample;
};

namespace Framework
{
	class Sample;

	struct SampleInstance
	{
	public:
		::Sound::Sample const * get_sound_sample() const { return soundSample; }
		Sample const * get_sample() const { return sample; }

		bool is_set() const { return soundSample != nullptr; }

		void set(Sample const * _sample, Random::Generator * _randomGenerator = nullptr);
		void clear();

		float get_final_volume() const { return volume * sampleVolume; }
		float get_final_pitch() const { return pitch * samplePitch; }

		void set_volume(float _volume) { volume = _volume; }
		void set_pitch(float _pitch) { pitch = _pitch; }

		float get_set_volume() const { return volume; }

		// to alter in general
		float get_sample_volume() const { return sampleVolume; }
		void set_sample_volume(float _sampleVolume) { sampleVolume = _sampleVolume; }

		float get_sample_pitch() const { return samplePitch; }
		void set_sample_pitch(float _samplePitch) { samplePitch = _samplePitch; }

	protected:
		int soundSampleIndex = 0;
		::Sound::Sample const * soundSample = nullptr;
		Sample const * sample = nullptr;

		float volume = 1.0f;
		float pitch = 1.0f;
		float sampleVolume = 1.0f;
		float samplePitch = 1.0f;
	};
};
