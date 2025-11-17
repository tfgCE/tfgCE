#include "soundSystem.h"

#include "soundConfig.h"
#include "utils.h"

#include "..\..\mainConfig.h"
#include "..\..\mainSettings.h"
#include "..\..\recordVideoSettings.h"
#include "..\..\utils.h"
#include "..\..\tags\tagCondition.h"
#include "..\..\splash\splashLogos.h"

#include "..\..\debug\debug.h"
#include "..\..\memory\memory.h"
#include "..\..\performance\performanceUtils.h"
#include "..\..\sound\soundFadingUtils.h"
#include "..\..\sound\soundGlobalOptions.h"

#ifdef AN_ANDROID
#include "..\..\system\javaEnv.h"
#endif

#include "..\core.h"

#ifdef AN_DEVELOPMENT_OR_PROFILER
#include "..\input.h"
#endif

#ifdef AN_FMOD
#include "fmod_errors.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define AN_LIST_DEVICES

//#define AN_LOG_COMPRESSOR
#define AN_LOG_DSPS

//#define INSPECT_CHANNEL_FADING

//

using namespace System;

//

#ifdef AN_WINDOWS
GUID System::Sound::s_preferredGuid = GUID_NULL;
#endif

::System::Sound* ::System::Sound::s_current = nullptr;

#ifdef AN_WINDOWS
void ::System::Sound::set_preferred_audio_device(GUID _guid)
{
	an_assert(!s_current);
	s_preferredGuid = _guid;
}
#endif

void ::System::Sound::create(SoundConfig const * _sc)
{
	an_assert(s_current == nullptr);
	s_current = new ::System::Sound(_sc);
}

void ::System::Sound::terminate()
{
	an_assert(s_current != nullptr);
	delete_and_clear(s_current);
}

::System::Sound::Sound(SoundConfig const * _sc)
{
	SET_EXTRA_DEBUG_INFO(channels, TXT("Sound.channels"));

#ifdef AN_FMOD
	FMOD_RESULT result;

#ifdef AN_ANDROID
	System::JavaEnv::get_env();
#endif

	result = FMOD::Studio::System::create(&fmodSystem);
	if (result != FMOD_OK)
	{
		String error = String::from_char8(FMOD_ErrorString(result));
		error(TXT("fmod error (on create): (%d) %S\n"), result, error.to_char());
		return;
	}

	// create normal, do not try to use own threads as that leaves occasional gaps
	// FMOD_STUDIO_INIT_NORMAL to normal functioning
	// FMOD_STUDIO_INIT_SYNCHRONOUS_UPDATE we will be calling from one thread only
	int useLowPass = _sc->enableOcclusionLowPass ? FMOD_INIT_CHANNEL_LOWPASS : 0;
	int useDistanceFilter = _sc->enableOcclusionLowPass ? FMOD_INIT_CHANNEL_DISTANCEFILTER : 0;
#ifdef SOUND_NO_LOW_PASS
	useLowPass = 0;
#endif
#ifdef SOUND_NO_DISTANCE_FILTER
	useDistanceFilter = 0;
#endif
	result = fmodSystem->initialize(MainConfig::global().get_sound().maxConcurrentSounds, (/*FMOD_STUDIO_INIT_NORMAL*/ FMOD_STUDIO_INIT_SYNCHRONOUS_UPDATE),
		FMOD_INIT_NORMAL /*| FMOD_INIT_THREAD_UNSAFE*/ | FMOD_INIT_3D_RIGHTHANDED | useLowPass | useDistanceFilter, 0);
	if (result != FMOD_OK)
	{
		String error = String::from_char8(FMOD_ErrorString(result));
		error_stop(TXT("fmod error (on initialize): (%d) %S\n"), result, error.to_char());
		return;
	}

	Splash::Logos::add(TXT("fmod"), SPLASH_LOGO_SYSTEM);

	fmodSystem->getCoreSystem(&fmodLowLevelSystem);

	{
		FMOD_STUDIO_ADVANCEDSETTINGS settings;
		settings.cbsize = sizeof(FMOD_STUDIO_ADVANCEDSETTINGS);
		fmodSystem->getAdvancedSettings(&settings);
		output(TXT("fmod command queue size %i"), settings.commandqueuesize);
		output(TXT("fmod studio update period %i"), settings.studioupdateperiod);
		//fmodSystem->setAdvancedSettings(&settings);
	}

#ifdef AN_WINDOWS
#ifdef AN_RECORD_VIDEO
	auto const& preferredDevices = _sc->preferredDevicesForRecording;
#else
	auto const& preferredDevices = _sc->preferredDevices;
#endif
	if (
#ifndef AN_RECORD_VIDEO
		s_preferredGuid != GUID_NULL ||
#endif
		!preferredDevices.is_empty())
	{
		int driverCount = 0;
		fmodLowLevelSystem->getNumDrivers(&driverCount);

#ifdef AN_LIST_DEVICES
		output(TXT("  audio devices:"));
#endif
		int driver = 0;
		int foundDriver = driverCount;
		bool foundDriverIsPreferredOne = false;
		int foundDriverPreferredIndex = NONE;
		String foundDriverName;
		while (driver < driverCount)
		{
			char name[256] = { 0 };
			FMOD_GUID fmodGuid = { 0 };
			fmodLowLevelSystem->getDriverInfo(driver, name, 256, &fmodGuid, nullptr, nullptr, nullptr);
#ifdef AN_LIST_DEVICES
			output(TXT("   %S"), String::from_char8_utf(name).to_char());
#endif

			if (!foundDriverIsPreferredOne)
			{
#ifdef AN_WINDOWS
#ifndef AN_RECORD_VIDEO
				if (s_preferredGuid != GUID_NULL &&
					s_preferredGuid.Data1 == fmodGuid.Data1 &&
					s_preferredGuid.Data2 == fmodGuid.Data2 &&
					s_preferredGuid.Data3 == fmodGuid.Data3 &&
					memory_compare(s_preferredGuid.Data4, fmodGuid.Data4, 8))
				{
					foundDriverIsPreferredOne = true;
					foundDriver = driver;
					foundDriverName = String::from_char8_utf(name);
				}
				else
#endif
#endif
				{
					String nameAsString = String::from_char8_utf(name);
					int preferredIdx = preferredDevices.find_index(nameAsString);
					if (preferredIdx == NONE)
					{
						for_every(pd, preferredDevices)
						{
							if (String::does_contain_icase(nameAsString.to_char(), pd->to_char()))
							{
								preferredIdx = for_everys_index(pd);
								break;
							}
						}
					}
					if (preferredIdx != NONE &&
						(preferredIdx < foundDriverPreferredIndex || foundDriverPreferredIndex == NONE))
					{
						foundDriverPreferredIndex = preferredIdx;
						foundDriver = driver;
						foundDriverName = nameAsString;
					}
				}
			}

			++ driver;
		}

		if (foundDriver < driverCount)
		{
			output(TXT("using preferred audio device \"%S\""), foundDriverName.to_char());
			fmodLowLevelSystem->setDriver(foundDriver);
		}
		else
		{
			warn(TXT("preferred audio device not found"));
		}
	}
#endif
	fmodLowLevelSystem->getSoftwareFormat(&dspClockRate, 0, 0);
	dspClockRateAsFloat = (float)dspClockRate;

	fmodSystem->setNumListeners(1);

	{	// setup axes
		FMOD_3D_ATTRIBUTES attributes;
		fmodSystem->getListenerAttributes(0, &attributes);
		attributes.forward = SoundUtils::convert(Vector3::yAxis);
		attributes.up = SoundUtils::convert(Vector3::zAxis);
		fmodSystem->setListenerAttributes(0, &attributes);
	}

	fmodLowLevelSystem->set3DSettings(1.0f, 1.0f, 1.0f);

	if (fmodLowLevelSystem->createChannelGroup("<main channel group>", &mainChannelGroup) != FMOD_OK)
	{
		String error = String::from_char8(FMOD_ErrorString(result));
		error(TXT("fmod error (on create main channel): (%d) %S\n"), result, error.to_char());
		return;
	}
	
	mainChannelGroup->setVolumeRamp(true);

#ifdef AN_SOUND_LOUDNESS_METER
	mainLoudnessMeter.create_and_add_to(fmodLowLevelSystem, mainChannelGroup);
#endif

	// create channels
	for_every(channelDef, MainSettings::global().get_sound_channels())
	{
		FMOD::ChannelGroup* channelGroup = nullptr;
#ifdef AN_WIDE_CHAR
		if (fmodLowLevelSystem->createChannelGroup(channelDef->name.to_string().to_char8_array().get_data(), &channelGroup) != FMOD_OK)
#else
		if (fmodLowLevelSystem->createChannelGroup(channelName->to_char(), &channelGroup) != FMOD_OK)
#endif
		{
			String error = String::from_char8(FMOD_ErrorString(result));
			error(TXT("fmod error (on create channels): (%d) %S\n"), result, error.to_char());
			return;
		}
		channelGroup->setVolumeRamp(true);
		mainChannelGroup->addGroup(channelGroup);
		if (channelDef->compressor.is_set())
		{
			FMOD::DSP* compressorDSP = nullptr;
			if (fmodLowLevelSystem->createDSPByType(FMOD_DSP_TYPE_COMPRESSOR, &compressorDSP) == FMOD_OK)
			{
#ifdef AN_LOG_COMPRESSOR
				output(TXT("created compressor DSP for \"%S\""), channelDef->name.to_char());
#endif
				auto& c = channelDef->compressor.get();
				if (compressorDSP->setParameterFloat(FMOD_DSP_COMPRESSOR_THRESHOLD, c.threshold) != FMOD_OK) { error(TXT("could not set dsp param")); }
				if (compressorDSP->setParameterFloat(FMOD_DSP_COMPRESSOR_RATIO, c.ratio) != FMOD_OK) { error(TXT("could not set dsp param")); }
				if (compressorDSP->setParameterFloat(FMOD_DSP_COMPRESSOR_ATTACK, c.attack * 1000.0f) != FMOD_OK) { error(TXT("could not set dsp param")); }
				if (compressorDSP->setParameterFloat(FMOD_DSP_COMPRESSOR_RELEASE, c.release * 1000.0f) != FMOD_OK) { error(TXT("could not set dsp param")); }
				// we're adding compressor at head (head is output, tail is input)
				// this is because we want the initial fader to affect the volume and only if it gets too loud to alter it further
				if (channelGroup->addDSP(FMOD_CHANNELCONTROL_DSP_HEAD, compressorDSP) == FMOD_OK)
				{
#ifdef AN_LOG_COMPRESSOR
					output(TXT("compressor DSP attached"));
#endif
					if (compressorDSP->setActive(true) == FMOD_OK)
					{
#ifdef AN_LOG_COMPRESSOR
						output(TXT("compressor DSP activated"));
#endif
					}
					else
					{
						error(TXT("error activating DSP compressor"));
					}
				}
				else
				{
					error(TXT("error attaching DSP compressor"));
				}
			}
			else
			{
				error(TXT("could not create DSP compressor DSP for \"%S\""), channelDef->name.to_char());
			}
		}
		channels.push_back(::Sound::Channel(channelDef->name, channelDef->tags, channelDef->reverb).with(channelGroup));
#ifdef AN_SOUND_LOUDNESS_METER
		channels.get_last().loudnessMeter.create_and_add_to(fmodLowLevelSystem, channelGroup);
#endif
#ifdef AN_LOG_DSPS
		{
			int dsps = 0;
			if (channelGroup->getNumDSPs(&dsps) == FMOD_OK)
			{
				output(TXT("channel \"%S\"'s DSPs:"), channelDef->name.to_char());
				for_count(int, i, dsps)
				{
					FMOD::DSP* dsp = nullptr;
					channelGroup->getDSP(i, &dsp);
					char name[64];
					if (dsp->getInfo(name, nullptr, nullptr, nullptr, nullptr) == FMOD_OK)
					{
						String txt = String::from_char8(name);
						output(TXT("  %i: %S"), i, txt.to_char());
					}
					else
					{
						warn_dev_ignore(TXT("  %i: <unknown>"), i);
					}
				}
			}
		}
#endif
	}
	isValid = true;
#endif

	if (isValid)
	{
		update_volumes();
		update_reverbs();

		::System::Core::on_quick_exit(TXT("close sound"), []() {terminate(); });
	}
}

::System::Sound::~Sound()
{
	::System::Core::on_quick_exit_no_longer(TXT("close sound"));

	clear_channels();
#ifdef AN_FMOD
	release_and_clear(fmodSystem);
	fmodLowLevelSystem = nullptr;
#endif
}

void ::System::Sound::clear_channels()
{
	channels.clear();
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
void ::System::Sound::update_on_development_volume_change()
{
	if (mainChannelGroup)
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		float developmentVolume = MainConfig::global().get_sound().developmentVolume;
		mainChannelGroup->setVolume(currentVolume * developmentVolume);
#else
		mainChannelGroup->setVolume(currentVolume);
#endif
	}
}
#endif

void ::System::Sound::update(float _deltaTime)
{
#ifdef AN_FMOD
	float prevVolume = currentVolume;
#endif
	targetVolume = (::System::Core::is_vr_paused() || ::System::Core::is_rendering_paused() || ::System::Core::is_paused())? 0.0f : 1.0f;
	currentVolume = blend_to_using_speed_based_on_time(currentVolume, targetVolume, volumeChangeTime, ::System::Core::get_raw_delta_time());
#ifdef AN_FMOD
	if (fmodSystem)
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_STANDARD_INPUT
		if (System::Input::get()->has_key_been_pressed(System::Key::F10))
		{
			prevVolume = currentVolume - 1.0f; // to force update
			auto & soundConfig = MainConfig::access_global().access_sound();
			if (soundConfig.developmentVolume < 1.0f)
			{
				soundConfig.developmentVolume = 1.0f;
			}
			else
			{
				soundConfig.developmentVolume = 0.1f;
			}
		}
#endif
		float developmentVolume = MainConfig::global().get_sound().developmentVolume;
#endif
		if (currentVolume != prevVolume && mainChannelGroup)
		{
#ifdef AN_DEVELOPMENT_OR_PROFILER
			mainChannelGroup->setVolume(currentVolume * developmentVolume);
#else
			mainChannelGroup->setVolume(currentVolume);
#endif
		}
#ifdef SOUND_SYSTEM_OWN_FADING
		// individual channels
		{
			for_every(ch, channels)
			{
				// for the initial setup, as if setVolume is set to 0 at the start, ch->usedSetFinalVolume might be 0 due to fadeVolume and be kept there
				// but each channel is created with volume set to 100%
				bool forceSetVolume = ch->usedSetVolume != ch->setVolume;
				ch->usedSetVolume = ch->setVolume; // in case our overall volume has changed
#ifdef INSPECT_CHANNEL_FADING
				if (ch->fadeVolume != ch->fadeVolumeTarget)
				{
					output(TXT("[fading snd sys] fading %S %.3f -> %.3f in %.3f, dt %.3f"), ch->name.to_char(), ch->fadeVolume, ch->fadeVolumeTarget, ch->fadeVolumeChangeTime, _deltaTime);
				}
#endif

				ch->fadeVolume = blend_to_using_speed_based_on_time(ch->fadeVolume, ch->fadeVolumeTarget, ch->fadeVolumeChangeTime, _deltaTime);
				ch->audioDuckVolume = blend_to_using_speed_based_on_time(ch->audioDuckVolume, ch->audioDuckVolumeTarget, ch->audioDuckVolumeChangeTime, _deltaTime);
				float currentChVolume = ch->calculate_final_volume();
				if ((currentChVolume != ch->usedSetFinalVolume || forceSetVolume) && ch->channelGroup)
				{
					ch->usedSetFinalVolume = currentChVolume;
					ch->channelGroup->setVolume(currentChVolume);
				}
			}
		}
#endif
		MEASURE_PERFORMANCE_COLOURED(soundSystemUpdate, Colour::black);
		fmodSystem->update();
	}
#endif

#ifdef AN_SOUND_LOUDNESS_METER
	mainLoudnessMeter.update_recent_max_loudness();
	for_every(ch, channels)
	{
		ch->loudnessMeter.update_recent_max_loudness();
	}
#endif
}

::Sound::Channel const * ::System::Sound::get_channel(Name const & _name) const
{
	for_every(channel, channels)
	{
		if (channel->name == _name)
		{
			return channel;
		}
	}
	return nullptr;
}

void ::System::Sound::set_listener_3d_params(Vector3 const & _location, Vector3 const & _velocity)
{
#ifdef AN_FMOD
	if (!fmodSystem)
	{
		return;
	}
	FMOD_3D_ATTRIBUTES attributes;
	fmodSystem->getListenerAttributes(0, &attributes);
	attributes.position = SoundUtils::convert(_location);
	attributes.velocity = SoundUtils::convert(_velocity);
	attributes.forward = SoundUtils::convert(Vector3::yAxis);
	attributes.up = SoundUtils::convert(Vector3::zAxis);
	fmodSystem->setListenerAttributes(0, &attributes);
#endif
}

void ::System::Sound::fade_in(float _fadeTime, TagCondition const & _channelsTagged)
{
	fade_to(_fadeTime, 1.0f, _channelsTagged);
}

void ::System::Sound::fade_out(float _fadeTime, TagCondition const & _channelsTagged)
{
	fade_to(_fadeTime, 0.0f, _channelsTagged);
}

void ::System::Sound::fade_to(float _target, float _fadeTime, TagCondition const & _channelsTagged)
{
	for_every(channel, channels)
	{
		if (_channelsTagged.check(channel->tags))
		{
#ifdef SOUND_SYSTEM_OWN_FADING
			channel->fadeVolumeTarget = _target;
			channel->fadeVolumeChangeTime = channel->fadeVolumeTarget != channel->fadeVolume ? _fadeTime / abs(channel->fadeVolumeTarget - channel->fadeVolume) : 0.0f;
#ifdef INSPECT_CHANNEL_FADING
			output(TXT("[fading snd sys] start fading tagged %S %.3f -> %.3f in %.3f"), channel->name.to_char(), channel->fadeVolume, _target, channel->fadeVolumeChangeTime);
#endif
#else
			channel->volumeLevel = _target;
#ifdef AN_FMOD
			::Sound::FadingUtils::FMOD::fade_to(channel->channelGroup, channel->volumeLevel * get_audio_duck_level(channel->audioDuck), _fadeTime, true);
#endif
#endif
		}
	}
}

float ::System::Sound::get_fade(TagCondition const& _channelsTagged) const
{
	Optional<float> fadeVolume;
	for_every(channel, channels)
	{
		if (_channelsTagged.check(channel->tags))
		{
			fadeVolume = max(fadeVolume.get(0.0f), channel->fadeVolume); // compare only if channel in use, tagged
		}
	}
	return fadeVolume.get(1.0f); // if not provided, act as nothing faded out
}

void ::System::Sound::fade_in(float _fadeTime, Optional<Name> const & _channel)
{
	fade_to(_fadeTime, 1.0f, _channel);
}

void ::System::Sound::fade_out(float _fadeTime, Optional<Name> const & _channel)
{
	fade_to(_fadeTime, 0.0f, _channel);
}

void ::System::Sound::fade_to(float _target, float _fadeTime, Optional<Name> const & _channel)
{
	if (_channel.is_set())
	{
		for_every(channel, channels)
		{
			if (channel->name == _channel.get())
			{
#ifdef SOUND_SYSTEM_OWN_FADING
				channel->fadeVolumeTarget = _target;
				channel->fadeVolumeChangeTime = channel->fadeVolumeTarget != channel->fadeVolume ? _fadeTime / abs(channel->fadeVolumeTarget - channel->fadeVolume) : 0.0f;
#ifdef INSPECT_CHANNEL_FADING
				output(TXT("[fading snd sys] fading selected %S %.3f -> %.3f in %.3f"), channel->name.to_char(), channel->fadeVolume, _target, channel->fadeVolumeChangeTime);
#endif
#else
				channel->volumeLevel = _target;
#ifdef AN_FMOD
				::Sound::FadingUtils::FMOD::fade_to(channel->channelGroup, channel->volumeLevel * get_audio_duck_level(channel->audioDuck), _fadeTime, true);
#endif
#endif
			}
		}
	}
	else
	{
#ifdef AN_FMOD
		if (auto* ch = mainChannelGroup) // global
		{
			::Sound::FadingUtils::FMOD::fade_to(ch, _target, _fadeTime, true);
		}
#endif
	}
}

void ::System::Sound::update_audio_duck()
{
	for_every(channel, channels)
	{
#ifdef SOUND_SYSTEM_OWN_FADING
		channel->audioDuckVolumeTarget = get_audio_duck_level(channel->audioDuck);
		channel->audioDuckVolumeChangeTime = channel->audioDuckVolumeTarget != channel->audioDuckVolume ? get_audio_duck_fade_time() / abs(channel->audioDuckVolumeTarget - channel->audioDuckVolume) : 0.0f;
#else
#ifdef AN_FMOD
		::Sound::FadingUtils::FMOD::fade_to(channel->channelGroup, channel->volumeLevel * get_audio_duck_level(channel->audioDuck), get_audio_duck_fade_time(), true);
#endif
#endif
	}
}

void ::System::Sound::audio_duck(int _audioDuck, TagCondition const& _channelsTagged)
{
	for_every(channel, channels)
	{
		if (_channelsTagged.check(channel->tags))
		{
			channel->audioDuck = _audioDuck;
		}
	}

	update_audio_duck();
}

void ::System::Sound::audio_duck_off_all()
{
	for_every(channel, channels)
	{
		channel->audioDuck = 0;
	}

	update_audio_duck();
}

void ::System::Sound::update_volumes()
{
	audioDuckSourceChannelVolume = 1.0f;
	Name audioDuckSourceChannel = MainSettings::global().get_audio_duck_source_channel();
	auto const& soundConfig = MainConfig::global().get_sound();
	for_every(channel, channels)
	{
		if (auto const * channelConfig = soundConfig.channels.get_existing(channel->name))
		{
			channel->setVolume = channelConfig->volume * channelConfig->overallVolume * soundConfig.volume;
#ifdef SOUND_SYSTEM_OWN_FADING
			// leave it as it is, we will update it during ::update call
#else
#ifdef AN_FMOD
			channel->channelGroup->setVolume(channel->setVolume);
#endif
#endif
			if (channel->name == audioDuckSourceChannel)
			{
				audioDuckSourceChannelVolume = channelConfig->volume;
			}
		}
	}
	audioDuckSourceChannelVolume = clamp(audioDuckSourceChannelVolume, 0.0f, 1.0f);
}

void ::System::Sound::update_reverbs()
{
	for_every(channel, channels)
	{
		if (!channel->usesReverbs)
		{
#ifdef AN_FMOD
			for_range(int, i, 0, FMOD_REVERB_MAXINSTANCES)
			{
				channel->channelGroup->setReverbProperties(i, 0.0f);
			}
#endif
		}
	}
}

float ::System::Sound::get_audio_duck_level(int _audioDuckLevel)
{
	if (_audioDuckLevel > 0)
	{
		float level = MainSettings::global().get_audio_duck_level();
		level /= (float)_audioDuckLevel;
		// if source channel volume at 1, use audio duck level as set
		// if source channel volume at 0, audio duck level not used at all - full volume
		level = lerp(audioDuckSourceChannelVolume, 1.0f, level);
		return level;
	}
	else
	{
		return 1.0f;
	}
}

float ::System::Sound::get_audio_duck_fade_time()
{
	return MainSettings::global().get_audio_duck_fade_time();
}

void ::System::Sound::log(LogInfoContext& _log) const
{
	struct LoudnessUtils
	{
		static void set_colour(LogInfoContext& _log, float loudness)
		{
			auto colour = ::Sound::LoudnessMeter::loudness_to_colour(loudness);
			_log.set_colour(colour);
		}
	};

	{
		float v = 1.0f;
		float loudness = -100.0f;
		float maxRecentLoudness = -100.0f;
#ifdef AN_SOUND_LOUDNESS_METER
		loudness = mainLoudnessMeter.get_loudness();
		maxRecentLoudness = mainLoudnessMeter.get_recent_max_loudness();
#endif
#ifdef SOUND_SYSTEM_OWN_FADING
		v = currentVolume;
#else
#ifdef AN_FMOD
		mainChannelGroup->getVolume(&v);
		float fv = ::Sound::FadingUtils::FMOD::calculate_current_fade_volume(mainChannelGroup);
		v *= fv;
#endif
#endif
		LoudnessUtils::set_colour(_log, max(loudness, maxRecentLoudness));
		_log.log(TXT("[%03.0f]:[%03.0fdB^%03.0fdb] main channel"), v * 100.0f, loudness, maxRecentLoudness);
		_log.set_colour();
	}
	LOG_INDENT(_log);
	for_every(ch, channels)
	{
		float v = 1.0f;
		float loudness = -90.0f;
		float maxRecentLoudness = -90.0f;
#ifdef AN_SOUND_LOUDNESS_METER
		bool isActive = true;
#ifdef AN_FMOD
		ch->channelGroup->isPlaying(&isActive);
#endif
		if (isActive)
		{
			loudness = ch->loudnessMeter.get_loudness();
			maxRecentLoudness = ch->loudnessMeter.get_recent_max_loudness();
		}
#endif
		LoudnessUtils::set_colour(_log, max(loudness, maxRecentLoudness));
#ifdef SOUND_SYSTEM_OWN_FADING
		v = ch->calculate_final_volume();
		_log.log(TXT("[%03.0f]:[%03.0fdB^%03.0fdb] %c%c %S"), v * 100.0f, loudness, maxRecentLoudness, ch->audioDuckVolumeTarget < 0.99f? 'D' : ' ', ch->fadeVolumeTarget > 0.9f? '+' : (ch->fadeVolumeTarget < 0.1f ? '-' : '?'), ch->name.to_char());
#else
#ifdef AN_FMOD
		ch->channelGroup->getVolume(&v);
		float fv = ::Sound::FadingUtils::FMOD::calculate_current_fade_volume(ch->channelGroup);
		v *= fv;
#endif
		_log.log(TXT("[%03.0f]:[%03.0fdB^%03.0fdb] %c%c %S"), v * 100.0f, loudness, maxRecentLoudness, ch->audioDuck? 'D' : ' ', ch->volumeLevel > 0.9f? '+' : (ch->volumeLevel < 0.1f ? '-' : '?'), ch->name.to_char());
#endif
		_log.set_colour();
	}
}

#ifdef AN_SOUND_LOUDNESS_METER
Optional<float> System::Sound::get_loudness() const
{
	Optional<float> result;
	result = mainLoudnessMeter.get_loudness();
	return result;
}

Optional<float> System::Sound::get_recent_max_loudness() const
{
	Optional<float> result;
	result = mainLoudnessMeter.get_recent_max_loudness();
	return result;
}
#endif

void ::System::Sound::set_custom_volume(TagCondition const& _tagged, Name const& _customVolumeId, float _volume)
{
#ifdef SOUND_SYSTEM_OWN_FADING
	for_every(ch, channels)
	{
		if (_tagged.check(ch->tags))
		{
			bool found = false;
			for_every(cv, ch->customVolumes)
			{
				if (cv->id == _customVolumeId)
				{
					cv->volume = _volume;
					found = true;
				}
			}
			if (!found)
			{
				::Sound::Channel::CustomVolume cv;
				cv.id = _customVolumeId;
				cv.volume = _volume;
				ch->customVolumes.push_back(cv);
			}
		}
	}
#else
	todo_implement
#endif
}

void ::System::Sound::clear_custom_volume(Name const& _customVolumeId)
{
#ifdef SOUND_SYSTEM_OWN_FADING
	for_every(ch, channels)
	{
		for (int i = 0; i < ch->customVolumes.get_size(); ++i)
		{
			if (ch->customVolumes[i].id == _customVolumeId)
			{
				ch->customVolumes.remove_fast_at(i);
				--i;
			}
		}
	}
#else
	todo_implement
#endif
}

void ::System::Sound::clear_custom_volumes()
{
#ifdef SOUND_SYSTEM_OWN_FADING
	for_every(ch, channels)
	{
		ch->customVolumes.clear();
	}
#else
	todo_implement
#endif
}

