#include "sample.h"

#include "playback.h"

#include "soundGlobalOptions.h"

#include "..\utils.h"
#include "..\mainSettings.h"

#include "..\concurrency\scopedSpinLock.h"
#include "..\io\file.h"
#include "..\io\xml.h"
#include "..\system\sound\soundSystem.h"
#include "..\types\string.h"

#include "..\serialisation\serialiser.h"

#ifdef AN_FMOD
#include "fmod_errors.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace ::Sound;

//

Importer<Sample, SampleParams> Sample::s_importer;

//

Sample* import_sample(String const & _fileName, SampleParams const & _options, int _filesizeLimit = 0)
{
	Sample* sample = new Sample();
#ifdef AN_DEVELOPMENT
#ifndef AN_ANDROID
	if (_filesizeLimit != 0)
	{
		uint filesize = IO::File::get_size_of(_fileName);
		if (filesize > (uint)_filesizeLimit)
		{
			warn(TXT("consider compression of sample \"%S\" (%ikB)"), _fileName.to_char(), filesize / 1024);
		}
	}
#endif
#endif
	if (sample->import_from_file(_fileName, _options))
	{
		return sample;
	}
	else
	{
		delete sample;
		return (Sample*)nullptr;
	}
}

void Sample::initialise_static()
{
	Sample::importer().register_file_type_with_options(String(TXT("wav")), [](String const & _fileName, SampleParams const & _options) { return import_sample(_fileName, _options, 512 * 1024); });
	Sample::importer().register_file_type_with_options(String(TXT("mp2")), [](String const & _fileName, SampleParams const & _options) { return import_sample(_fileName, _options); });
	Sample::importer().register_file_type_with_options(String(TXT("mp3")), [](String const & _fileName, SampleParams const & _options) { return import_sample(_fileName, _options); });
	Sample::importer().register_file_type_with_options(String(TXT("ogg")), [](String const & _fileName, SampleParams const & _options) { return import_sample(_fileName, _options); });
	Sample::importer().register_file_type_with_options(String(TXT("raw")), [](String const & _fileName, SampleParams const & _options) { return import_sample(_fileName, _options); });
}

void Sample::close_static()
{
	Sample::importer().unregister(String(TXT("wav")));
	Sample::importer().unregister(String(TXT("mp2")));
	Sample::importer().unregister(String(TXT("mp3")));
	Sample::importer().unregister(String(TXT("ogg")));
	Sample::importer().unregister(String(TXT("raw")));
}

Sample::Sample()
{
}

Sample::~Sample()
{
#ifdef AN_FMOD
#ifdef AN_DEVELOPMENT
	if (sound)
	{
		auto result = sound->release();
		output(TXT("sound sample release result: %i"), result);
		sound = nullptr;
	}
#else
	release_and_clear(sound);
#endif
#endif
}

Playback Sample::play(float _fadeIn, Optional<bool> const& _forceOwnFading, Channel const * _channelOverride, bool _keepPaused) const
{
	Playback playback = set_to_play_remain_paused(_forceOwnFading, _channelOverride);

	playback.fade_in_on_start(_fadeIn);

	// unpause now as we have volume and pitch set
	if (!_keepPaused)
	{
		playback.resume();
	}

	return playback;
}

Playback Sample::set_to_play_remain_paused(Optional<bool> const& _forceOwnFading, Channel const * _channelOverride) const
{
	Playback playback;
	playback.set_new_sample(this, _forceOwnFading);
	if (auto * ss = ::System::Sound::get())
	{
#ifdef AN_FMOD
		FMOD::Channel* ch;
		Channel const * useChannel = _channelOverride ? _channelOverride : channel;

		auto& lowLevelSystem = ss->access_low_level_system();
		// start paused to allow to change volume and pitch
		lowLevelSystem.playSound(sound, useChannel ? useChannel->channelGroup : nullptr, true, &ch);

		playback.set_new_channel(useChannel, ch);

#ifndef SOUND_NO_LOW_PASS
		playback.set_low_pass_gain(1.0f); // will adjust using occlusion
#endif
		playback.set_use_reverbs(params.usesReverbs.get(useChannel ? useChannel->usesReverbs : true) ? 1.0f : 0.0f);

		ch->setPriority(128 - params.priority);
		if (params.is3D)
		{
#ifdef SOUND_NO_DOPPLER
			ch->set3DDopplerLevel(0.0f);
#else
			ch->set3DDopplerLevel(params.dopplerLevel);
#endif
		}

		playback.update_pitch();
		playback.update_volume();
#endif
	}

	return playback;
}

bool Sample::import_from_file(String const & _filename, SampleParams const & _params)
{
	bool result = true;

#ifdef AN_FMOD
	an_assert(sound == nullptr);
#endif

	// just copy
	params = _params;

	// read plain data from file
	IO::File file;
	if (file.open(_filename))
	{
		file.set_type(IO::InterfaceType::Binary);
		plainData.clear();
		file.read_into(plainData);
	}

#ifdef AN_FMOD
	if (auto * ss = ::System::Sound::get())
	{
		auto& lowLevelSystem = ss->access_low_level_system();
		Concurrency::ScopedSpinLock lock(ss->access_loading_lock());

		int flags = _params.isStream? FMOD_CREATESTREAM : FMOD_CREATESAMPLE;
		params.prepare_create_sound_flags(REF_ flags);
#ifdef AN_WIDE_CHAR
		if (lowLevelSystem.createSound(_filename.to_char8_array().get_data(), flags, nullptr, &sound) == FMOD_OK)
#else
		if (lowLevelSystem.createSound(_filename.to_char(), flags, nullptr, &sound) == FMOD_OK)
#endif
		{
			on_loaded();
		}
		else
		{
			error(TXT("error importing sound from file \"%S\""), _filename.to_char());
			result = false;
		}
	}
	else
#endif
	{
		// just skip
		result = true;
	}

	if (result)
	{
		process_params();
	}

	return result;
}

#define VER_BASE 0
#define CURRENT_VERSION VER_BASE

bool Sample::serialise(Serialiser & _serialiser)
{
	version = CURRENT_VERSION;
	bool result = SerialisableResource::serialise(_serialiser);
	if (version > CURRENT_VERSION)
	{
		error(TXT("invalid sample version"));
		return false;
	}

	result &= params.serialise(_serialiser);

	serialise_plain_data_array(_serialiser, plainData);

#ifdef AN_FMOD
	if (_serialiser.is_reading())
	{
		if (auto * ss = ::System::Sound::get())
		{
			auto& lowLevelSystem = ss->access_low_level_system();
			Concurrency::ScopedSpinLock lock(ss->access_loading_lock());

			FMOD_CREATESOUNDEXINFO info;
			memory_zero(&info, sizeof(info));
			info.cbsize = sizeof(info);
			info.length = plainData.get_size();
			int flags = params.isStream ? FMOD_CREATESTREAM : FMOD_CREATESAMPLE;
			flags |= FMOD_OPENMEMORY;
			params.prepare_create_sound_flags(REF_ flags);
			auto fmodResult = lowLevelSystem.createSound(plainData.get_data(), flags, &info, &sound);
			if (fmodResult == FMOD_OK)
			{
				on_loaded();
			}
			else
			{
				String error = String::from_char8(FMOD_ErrorString(fmodResult));
				error(TXT("error deserialising/creating sample (%S)"), error.to_char());
				result = false;
			}
		}
		// plain data is no longer needed
		plainData.clear_and_prune();

		if (result)
		{
			process_params();
		}
	}
#else
	result = false;
#endif

	return result;
}

void Sample::process_params()
{
	if (!params.channel.is_valid())
	{
		params.channel = MainSettings::global().get_default_sound_channel();
	}
	if (auto * ss = ::System::Sound::get())
	{
		channel = ss->get_channel(params.channel);
	}
	else
	{
		channel = nullptr;
	}
}


void Sample::on_loaded()
{
#ifdef AN_FMOD
	an_assert(sound);
#endif

	uint lengthInMS = 0;
#ifdef AN_FMOD
	sound->getLength(&lengthInMS, FMOD_TIMEUNIT_MS);
#endif
	length = ((float)lengthInMS) * 0.001f;

	if (params.is3D)
	{
#ifdef AN_FMOD
		sound->set3DMinMaxDistance(params.distances.min, params.distances.max);
#endif
	}
}
