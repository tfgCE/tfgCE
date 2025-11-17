#include "environmentPlayer.h"

#include "..\utils\dullPlayback.h"

#include "..\..\framework\game\gameUtils.h"
#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\framework\sound\soundSample.h"
#include "..\..\framework\world\door.h"
#include "..\..\framework\world\doorInRoom.h"
#include "..\..\framework\world\room.h"
#include "..\..\framework\world\roomRegionVariables.inl"

#include "..\..\core\physicalSensations\physicalSensations.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef INVESTIGATE_SOUND_SYSTEM
	#define LOG_ENVIRONMENT_PLAYER
#else
	#ifdef AN_DEVELOPMENT_OR_PROFILER
		#define LOG_ENVIRONMENT_PLAYER
	#endif
#endif

//

using namespace TeaForGodEmperor;

//

// room parameters
DEFINE_STATIC_NAME(environmentEffect);
DEFINE_STATIC_NAME(environmentEffectOnEntrance);
DEFINE_STATIC_NAME(environmentEffectOnEntranceChance);
DEFINE_STATIC_NAME(environmentEffectOnEntranceCooldown);

DEFINE_STATIC_NAME(environmentEffectBeyondDoor);
DEFINE_STATIC_NAME(environmentEffectBeyondDoorDistance);

DEFINE_STATIC_NAME(customEnvironmentEffect);
DEFINE_STATIC_NAME(customEnvironmentEffectBeyondDoor);
DEFINE_STATIC_NAME(customEnvironmentEffectBeyondDoorDistance);

// handled effects
DEFINE_STATIC_NAME(wind);
DEFINE_STATIC_NAME(windBlow);
DEFINE_STATIC_NAME_STR(desertWind, TXT("desert wind"));

// global references
DEFINE_STATIC_NAME_STR(grSoundWind, TXT("environment effect; sound; wind"));
DEFINE_STATIC_NAME_STR(grSoundWindBeyondDoor, TXT("environment effect; sound; wind beyond door"));
DEFINE_STATIC_NAME_STR(grSoundWindBlow, TXT("environment effect; sound; wind blow"));
DEFINE_STATIC_NAME_STR(grSoundDesertWind, TXT("environment effect; sound; desert wind"));
DEFINE_STATIC_NAME_STR(grSoundDesertWindBeyondDoor, TXT("environment effect; sound; desert wind beyond door"));

//

EnvironmentPlayer::EnvironmentPlayer()
{
	if (auto* lib = Framework::Library::get_current())
	{
		windSample = lib->get_global_references().get<Framework::Sample>(NAME(grSoundWind));
		windBeyondDoorSample = lib->get_global_references().get<Framework::Sample>(NAME(grSoundWindBeyondDoor));
		windBlowSample = lib->get_global_references().get<Framework::Sample>(NAME(grSoundWindBlow));
		desertWindSample = lib->get_global_references().get<Framework::Sample>(NAME(grSoundDesertWind));
		desertWindBeyondDoorSample = lib->get_global_references().get<Framework::Sample>(NAME(grSoundDesertWindBeyondDoor));
	}
}

EnvironmentPlayer::~EnvironmentPlayer()
{
}

void EnvironmentPlayer::load_on_demand_if_required_for(Framework::Room* _room)
{
	struct Utils
	{
		static void handle(Framework::Room* _room, Name const& _paramName)
		{
			auto& ee = _room->get_value<Framework::LibraryName>(_paramName, Framework::LibraryName::invalid(),
				false /* load only for this one */);
			if (ee.is_valid())
			{
				if (auto* sample = fast_cast<Framework::Sample>(Framework::Library::get_current()->get_samples().find_stored(ee)))
				{
					sample->load_on_demand_if_required();
				}
			}
		}
	};

	Utils::handle(_room, NAME(customEnvironmentEffect));
	Utils::handle(_room, NAME(customEnvironmentEffectBeyondDoor));
}

void EnvironmentPlayer::set_dull(float _dull, float _dullBlendTime)
{
	dullTarget = _dull;
	dullBlendTime = _dullBlendTime;
}

void EnvironmentPlayer::pause()
{
	if (isPaused)
	{
		return;
	}
	Concurrency::ScopedSpinLock lock(accessLock);
	isPaused = true;
	for_every(e, enviroEffects)
	{
		e->playback.pause();
	}
	for_every(e, enviroEffectsOnEntrace)
	{
		e->playback.pause();
	}
}

void EnvironmentPlayer::resume()
{
	if (!isPaused)
	{
		return;
	}
	Concurrency::ScopedSpinLock lock(accessLock);
	isPaused = false;
	for_every(e, enviroEffects)
	{
		e->playback.resume_with_fade_in();
	}
	for_every(e, enviroEffectsOnEntrace)
	{
		e->playback.resume_with_fade_in();
	}
}

void EnvironmentPlayer::clear()
{
	Concurrency::ScopedSpinLock lock(accessLock);
	for_every(e, enviroEffects)
	{
		e->playback.stop();
	}
	for_every(e, enviroEffectsOnEntrace)
	{
		e->playback.stop();
	}
	enviroEffects.clear();
	enviroEffectsOnEntrace.clear();
}

Framework::Sample* EnvironmentPlayer::get_custom_sample(Framework::LibraryName const& _name)
{
	Framework::Sample* sample = nullptr;
	for_every(c, customSamples)
	{
		if (c->get_name() == _name)
		{
			sample = c->get();
			break;
		}
	}
	if (!sample)
	{
		sample = fast_cast<Framework::Sample>(Framework::Library::get_current()->get_samples().find_stored(_name));
		if (sample)
		{
			customSamples.push_back(Framework::UsedLibraryStored<Framework::Sample>(sample));
		}
	}
	return sample;
}

void EnvironmentPlayer::update(Framework::IModulesOwner* _imo, float _deltaTime)
{
	if (isPaused)
	{
		return;
	}

	Concurrency::ScopedSpinLock lock(accessLock);

	MEASURE_PERFORMANCE_COLOURED(environmentPlayerUpdate, Colour::zxGreen);

	cooldowns.advance(_deltaTime);

	// remove one at a time
	{
		if (!enviroEffects.is_empty())
		{
			auto& e = enviroEffects.get_last();
			if (!e.playback.is_playing())
			{
				enviroEffects.pop_back();
			}
		}
		if (!enviroEffectsOnEntrace.is_empty())
		{
			auto& e = enviroEffectsOnEntrace.get_last();
			if (!e.playback.is_playing())
			{
				enviroEffectsOnEntrace.pop_back();
			}
		}
	}

	dull = blend_to_using_time(dull, dullTarget, dullBlendTime, _deltaTime);

	DullPlayback dullPlayback(dull);

	// update
	{
		for_every(e, enviroEffects)
		{
			e->playback.update_pitch();
			e->playback.update_volume();
			dullPlayback.handle(e->playback);
			e->playback.advance_fading(_deltaTime);
		}
		for_every(e, enviroEffectsOnEntrace)
		{
			e->playback.update_pitch();
			e->playback.update_volume();
			dullPlayback.handle(e->playback);
			e->playback.advance_fading(_deltaTime);
		}
	}

	for_every(ds, doorSounds)
	{
		if (auto* ss = ds->soundSource.get())
		{
			if (auto* td = ds->throughDoor.get())
			{
				if (auto* dt = td->get_door_type())
				{
					ss->access_sample_instance().set_volume(dt->get_sound_hearable_when_open().calculate_at(td->get_abs_open_factor()));
				}
			}
		}
	}

	Framework::Room* newRoom = nullptr;
	if (_imo)
	{
		if (auto* p = _imo->get_presence())
		{
			if (auto* r = p->get_in_room())
			{
				if (r != checkedForRoom)
				{
					if (Framework::GameUtils::is_controlled_by_local_player(_imo))
					{
						newRoom = r;
					}
				}
			}
		}
	}

	if (!newRoom && _imo)
	{
		return;
	}

	MEASURE_PERFORMANCE_COLOURED(checkNewRoom, Colour::zxBlue);

	checkedForRoom = newRoom;

	stop_door_sounds();

	update_effects();

	// play enviro effects on entrace
	if (newRoom)
	{
		Name enviroEffectOnEntrance;
		enviroEffectOnEntrance = newRoom->get_value<Name>(NAME(environmentEffectOnEntrance), enviroEffectOnEntrance,
			false /* we do not want origin room as it may be something completely different interior/exterior wise, better to depend on region */);

		if (enviroEffectOnEntrance.is_valid() &&
			cooldowns.is_available(enviroEffectOnEntrance))
		{
			float chance = newRoom->get_value<float>(NAME(environmentEffectOnEntranceChance), 1.0f,
				false /* we do not want origin room as it may be something completely different interior/exterior wise, better to depend on region */);

			if (Random::get_chance(chance))
			{
				float cooldown = newRoom->get_value<float>(NAME(environmentEffectOnEntranceCooldown), 0.0f,
					false /* we do not want origin room as it may be something completely different interior/exterior wise, better to depend on region */);

				if (cooldown > 0.0f)
				{
					cooldowns.set_if_longer(enviroEffectOnEntrance, cooldown);
				}

				if (enviroEffectOnEntrance == NAME(windBlow))
				{
					PhysicalSensations::OngoingSensation s(PhysicalSensations::OngoingSensation::Wind, 1.0f, false);
					float frontAngle = 40.0f;
					s.with_dir_os(Rotator3(0.0f, 180.0f + Random::get_float(-frontAngle, frontAngle), 0.0f).get_forward());
					PhysicalSensations::start_sensation(s);
					play_effect_on_entrance(enviroEffectOnEntrance, windBlowSample.get());
				}
				else
				{
					todo_implement;
				}
			}
		}
	}

	// update play door infos
	{
		doorSounds.clear();
		if (newRoom)
		{
			//FOR_EVERY_ALL_DOOR_IN_ROOM(door, newRoom)
			for_every_ptr(door, newRoom->get_doors()) 
			{
				if (door->is_world_active() &&
					door->is_visible())
				{
					if (auto* r = door->get_room_on_other_side())
					{
						if (r->is_world_active())
						{
							{
								Name enviroEffect;
								enviroEffect = r->get_value<Name>(NAME(environmentEffectBeyondDoor), enviroEffect,
									false /* we do not want origin room as it may be something completely different interior/exterior wise, better to depend on region */);

								float distance = 1.0f;
								distance = r->get_value<float>(NAME(environmentEffectBeyondDoorDistance), distance,
									false /* we do not want origin room as it may be something completely different interior/exterior wise, better to depend on region */);

								Framework::Sample* sample = nullptr;
								if (enviroEffect.is_valid())
								{
									if (enviroEffect == NAME(wind))
									{
										sample = windBeyondDoorSample.get();
									}
									else if (enviroEffect == NAME(desertWind))
									{
										sample = desertWindBeyondDoorSample.get();
									}
									else
									{
										todo_implement;
									}
								}

								if (sample)
								{
									play_door_sound(enviroEffect, sample, newRoom, door->get_door(), door->get_placement().to_world(Transform(Vector3::yAxis * distance, Quat::identity)));
								}
							}
							{
								Framework::LibraryName customEnviroEffect;
								customEnviroEffect = r->get_value<Framework::LibraryName>(NAME(customEnvironmentEffectBeyondDoor), customEnviroEffect,
									false /* we do not want origin room as it may be something completely different interior/exterior wise, better to depend on region */);

								float distance = 1.0f;
								distance = r->get_value<float>(NAME(customEnvironmentEffectBeyondDoorDistance), distance,
									false /* we do not want origin room as it may be something completely different interior/exterior wise, better to depend on region */);

								Framework::Sample* sample = nullptr;
								if (customEnviroEffect.is_valid())
								{
									sample = get_custom_sample(customEnviroEffect);
								}

								if (sample)
								{
									play_door_sound(Name::invalid(), sample, newRoom, door->get_door(), door->get_placement().to_world(Transform(Vector3::yAxis * distance, Quat::identity)));
								}
							}
						}
					}
				}
			}
		}
	}
}

void EnvironmentPlayer::update_effects()
{
	auto* newRoom = checkedForRoom;
	if (!newRoom)
	{
		return;
	}

	stop_effects();
	// check whether to play enviro effect
	{
		Name enviroEffect;
		Framework::LibraryName customEnviroEffect;
		if (newRoom)
		{
			enviroEffect = newRoom->get_value<Name>(NAME(environmentEffect), enviroEffect,
				false /* we do not want origin room as it may be something completely different interior/exterior wise, better to depend on region */);
			customEnviroEffect = newRoom->get_value<Framework::LibraryName>(NAME(customEnvironmentEffect), customEnviroEffect,
				false /* we do not want origin room as it may be something completely different interior/exterior wise, better to depend on region */);
		}

		bool stopExisting = true;
		bool playNew = false;
		if (customEnviroEffect.is_valid())
		{
			playNew = true;
			if (!enviroEffects.is_empty() &&
				enviroEffects.get_first().playback.is_playing() &&
				!enviroEffects.get_first().playback.is_fading_out_or_faded_out() &&
				enviroEffects.get_first().sample &&
				enviroEffects.get_first().sample->get_name() == customEnviroEffect)
			{
				stopExisting = false;
				playNew = false;
			}
		}
		else if (enviroEffect.is_valid())
		{
			playNew = true;
			if (!enviroEffects.is_empty() &&
				enviroEffects.get_first().playback.is_playing() &&
				!enviroEffects.get_first().playback.is_fading_out_or_faded_out() &&
				enviroEffects.get_first().id == enviroEffect)
			{
				stopExisting = false;
				playNew = false;
			}
		}
		if (stopExisting)
		{
			stop_effects();
		}
		if (playNew)
		{
			if (customEnviroEffect.is_valid())
			{
				if (Framework::Sample* sample = get_custom_sample(customEnviroEffect))
				{
					play_effect(Name::invalid(), sample);
				}
			}
			else
			{
				if (enviroEffect == NAME(wind))
				{
					play_effect(enviroEffect, windSample.get());
				}
				else if (enviroEffect == NAME(desertWind))
				{
					play_effect(enviroEffect, desertWindSample.get());
				}
				else
				{
					todo_implement;
				}
			}
		}
	}
}

void EnvironmentPlayer::play_effect(Name const& _id, Framework::Sample* _sample)
{
	stop_effects();
	if (_sample)
	{
		Playback e;
		e.id = _id;
		e.sample = _sample;
#ifdef LOG_ENVIRONMENT_PLAYER
		output(TXT("[EnvPlayer] play %S: %S"), _id.to_char(), _sample->get_name().to_string().to_char());
#endif
		e.playback = _sample->play(NP, ::Sound::PlaybackSetup().with_own_fading(), NP, true);
		{
			DullPlayback dullPlayback(dull);
			dullPlayback.handle(e.playback);
		}
		e.playback.resume();
		enviroEffects.push_front(e);
	}
}

void EnvironmentPlayer::play_door_sound(Name const& _id, Framework::Sample* _sample, Framework::Room* _room, Framework::Door* _throughDoor, Transform const& _placementWS)
{
	if (_sample)
	{
		DoorPlayback d;
		d.id = _id;
		d.throughDoor = _throughDoor;
		d.sample = _sample;
#ifdef LOG_ENVIRONMENT_PLAYER
		output(TXT("[EnvPlayer] play door sound %S: %S"), _id.to_char(), _sample->get_name().to_string().to_char());
#endif
		d.soundSource = _room->access_sounds().play(_sample, _room, _placementWS);
		doorSounds.push_back(d);
	}
}

void EnvironmentPlayer::stop_effects()
{
#ifdef LOG_ENVIRONMENT_PLAYER
	if (!enviroEffects.is_empty())
	{
		output(TXT("[EnvPlayer] stop effects"));
	}
#endif
	for_every(e, enviroEffects)
	{
		e->playback.fade_out(e->sample->get_fade_out_time());
	}
}

void EnvironmentPlayer::stop_door_sounds()
{
#ifdef LOG_ENVIRONMENT_PLAYER
	if (!doorSounds.is_empty())
	{
		output(TXT("[EnvPlayer] stop door sounds"));
	}
#endif
	for_every(d, doorSounds)
	{
		if (auto* ss = d->soundSource.get())
		{
			ss->stop();
			d->soundSource.clear();
		}
	}
	doorSounds.clear(); // we can clear it here without any issue
}

void EnvironmentPlayer::play_effect_on_entrance(Name const& _id, Framework::Sample* _sample)
{
	if (_sample)
	{
		Playback e;
		e.id = _id;
		e.sample = _sample;
#ifdef LOG_ENVIRONMENT_PLAYER
		output(TXT("[EnvPlayer] play effect on entrance %S: %S"), _id.to_char(), _sample->get_name().to_string().to_char());
#endif
		e.playback = _sample->play(NP, ::Sound::PlaybackSetup().with_own_fading(), NP, true);
		{
			DullPlayback dullPlayback(dull);
			dullPlayback.handle(e.playback);
		}
		e.playback.resume();
		enviroEffectsOnEntrace.push_front(e);
	}
}

void EnvironmentPlayer::log(LogInfoContext& _log) const
{
	Concurrency::ScopedSpinLock lock(accessLock);
	{
		_log.log(TXT("loops"));
		LOG_INDENT(_log);
		for_every(e, enviroEffects)
		{
#ifdef AN_SOUND_LOUDNESS_METER
			Optional<float> loudness;
			Optional<float> maxRecentLoudness;
			loudness = e->playback.get_loudness();
			maxRecentLoudness = e->playback.get_recent_max_loudness();
			_log.set_colour(::Sound::LoudnessMeter::loudness_to_colour(maxRecentLoudness.get(loudness.get(-100.0f))));
			_log.log(TXT("%03.0f%% [%03.0fdB^%03.0fdB]: %S : %S"), e->playback.get_current_volume() * 100.0f, loudness.get(0.0f), maxRecentLoudness.get(0.0f), e->id.to_char(), e->sample->get_name().to_string().to_char());
#else
			_log.log(TXT("%03.0f%%: %S : %S"), e->playback.get_current_volume() * 100.0f, e->id.to_char(), e->sample->get_name().to_string().to_char());
#endif
		}
		_log.set_colour();
	}
	{
		_log.log(TXT("on entrance"));
		LOG_INDENT(_log);
		for_every(e, enviroEffectsOnEntrace)
		{
#ifdef AN_SOUND_LOUDNESS_METER
			Optional<float> loudness;
			Optional<float> maxRecentLoudness;
			loudness = e->playback.get_loudness();
			maxRecentLoudness = e->playback.get_recent_max_loudness();
			_log.set_colour(::Sound::LoudnessMeter::loudness_to_colour(maxRecentLoudness.get(loudness.get(-100.0f))));
			_log.log(TXT("%03.0f%% [%03.0fdB^%03.0fdB]: %S : %S"), e->playback.get_current_volume() * 100.0f, loudness.get(0.0f), maxRecentLoudness.get(0.0f), e->id.to_char(), e->sample->get_name().to_string().to_char());
#else
			_log.log(TXT("%03.0f%%: %S : %S"), e->playback.get_current_volume() * 100.0f, e->id.to_char(), e->sample->get_name().to_string().to_char());
#endif
		}
		_log.set_colour();
	}
	{
		_log.log(TXT("door sounds"));
		LOG_INDENT(_log);
		for_every(d, doorSounds)
		{
			_log.log(TXT("%S : %S : %S"), d->soundSource.is_set() && d->soundSource->is_active()? TXT("actv") : TXT("----"), d->id.to_char(), d->sample->get_name().to_string().to_char());
		}
	}
}

