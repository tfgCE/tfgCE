#include "moduleSound.h"

#include "modules.h"
#include "moduleSoundData.h"
#include "registeredModule.h"

#include "..\advance\advanceContext.h"
#include "..\ai\aiMessage.h"
#include "..\appearance\appearanceController.h"
#include "..\appearance\appearanceControllerData.h"
#include "..\appearance\appearanceControllerPoseContext.h"
#include "..\appearance\mesh.h"
#include "..\appearance\skeleton.h"
#include "..\game\gameUtils.h"
#include "..\interfaces\aiObject.h"
#include "..\library\usedLibraryStored.inl"
#include "..\object\actor.h"
#include "..\object\temporaryObject.h"
#include "..\render\renderContext.h"
#include "..\sound\soundSample.h"
#include "..\sound\soundScene.h"
#include "..\video\texture.h"
#include "..\world\room.h"
#include "..\world\world.h"

#include "..\..\core\debug\extendedDebug.h"
#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\mesh\pose.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

// ai message params
DEFINE_STATIC_NAME(what);
DEFINE_STATIC_NAME(who);
DEFINE_STATIC_NAME(room);
DEFINE_STATIC_NAME(location);
DEFINE_STATIC_NAME(propagationDir);
DEFINE_STATIC_NAME(throughDoor);
DEFINE_STATIC_NAME(sound);
DEFINE_STATIC_NAME(soundId);

// params
DEFINE_STATIC_NAME(generalSoundVolume);

//

REGISTER_FOR_FAST_CAST(ModuleSound);

static ModuleSound* create_module(IModulesOwner* _owner)
{
	return new ModuleSound(_owner);
}

RegisteredModule<ModuleSound> & ModuleSound::register_itself()
{
	return Modules::sound.register_module(String(TXT("base")), create_module);
}

ModuleSound::ModuleSound(IModulesOwner* _owner)
: Module(_owner)
{
}

ModuleSound::~ModuleSound()
{
}

void ModuleSound::reset()
{
	Module::reset();
	autoPlayPlayed = false;
}

void ModuleSound::setup_with(ModuleData const * _moduleData)
{
	Module::setup_with(_moduleData);

	moduleSoundData = fast_cast<ModuleSoundData>(_moduleData);

	if (moduleSoundData)
	{
		{
			generalSoundVolume.clear();
			float gsv = _moduleData->get_parameter<float>(this, NAME(generalSoundVolume), -1.0f);
			if (gsv != -1.0f)
			{
				generalSoundVolume = gsv;
			}
		}

		for_every(soundSample, moduleSoundData->get_samples())
		{
			for_every_ref(smp, soundSample->samples)
			{
				smp->load_on_demand_if_required();
			}
		}
	}
}

void ModuleSound::initialise()
{
	base::initialise();
}

void ModuleSound::activate()
{
	base::activate();

	play_auto_play();
}

void ModuleSound::play_auto_play()
{
	if (autoPlayPlayed)
	{
		return;
	}

	if (!get_owner()->get_presence()->get_in_room())
	{
		return;
	}

	autoPlayPlayed = true;
	if (moduleSoundData)
	{
		for_every(soundSample, moduleSoundData->get_samples())
		{
			if (soundSample->autoPlay)
			{
				play_sound(for_everys_map_key(soundSample));
			}
		}
	}
}

bool ModuleSound::does_require_advance_sounds() const
{
	return sounds.does_require_advance() || soundCooldowns.does_require_advance() || aiMessageCooldowns.does_require_advance();
}

void ModuleSound::advance_sounds(float _deltaTime)
{
	bool anySounds = !sounds.get_sounds().is_empty();
	sounds.advance(_deltaTime);
	soundCooldowns.advance(_deltaTime);
	aiMessageCooldowns.advance(_deltaTime);
	if (auto* to = get_owner()->get_as_temporary_object())
	{
		if (anySounds &&
			sounds.get_sounds().is_empty())
		{
			to->no_more_sounds();
		}
	}
	if (AVOID_CALLING_ACK_ !does_require_advance_sounds())
	{
		get_owner()->clear_sounds_require_advancement();
	}
}

bool ModuleSound::does_handle_sound(Name const & _name) const
{
	if (moduleSoundData)
	{
		if (auto soundSamples = moduleSoundData->get_samples().get_existing(_name))
		{
			return true;
		}
	}

	return false;
}

SoundSource* ModuleSound::just_play_sound(Name const & _name, ModuleSoundData const* _usingData, OUT_ ModuleSoundSample const * & _soundSample, SoundPlayParams const & _params)
{
	_soundSample = nullptr;
	if (_usingData)
	{
		if (soundCooldowns.is_available(_name))
		{
			if (auto soundSamples = _usingData->get_samples().get_existing(_name))
			{
				if (!can_play_sound(soundSamples))
				{
					return nullptr;
				}
				_soundSample = soundSamples;
				if (!soundSamples->samples.is_empty() &&
					(!soundSamples->chance.is_set() || rg.get_chance(soundSamples->chance.get())))
				{
					auto * soundSample = &soundSamples->samples[rg.get_int(soundSamples->samples.get_size())];
#ifdef AN_DEVELOPMENT
					if (!soundSample->get() || !soundSample->get()->get_sample())
					{
						// this may happen in preview game where we accept problems while loading
						return nullptr;
					}
#endif
					SoundPlayParams params(_params);
					for_every(ppo, playParamsOverrides)
					{
						if (_name == ppo->sound ||
							soundSample->get()->get_name() == ppo->sample)
						{
							params.fill_with(ppo->params);
						}
					}
					params.fill_with(soundSamples->playParams);

					if (!should_sample_be_heard(soundSample->get(), _params))
					{
						return nullptr;
					}

					if (soundSamples->remapPitch.is_valid())
					{
						if (! soundSamples->remapPitchContinuous)
						{
							if (auto* usingVar = get_owner()->get_variables().get_existing<float>(soundSamples->remapPitchUsingVar))
							{
								float resPitch = soundSamples->remapPitch.remap(*usingVar);
								if (params.pitch.is_set())
								{
									params.pitch = params.pitch.get() * resPitch;
								}
								else
								{
									params.pitch = Range(resPitch);
								}
							}
						}
					}
						
					if (soundSamples->remapVolume.is_valid())
					{
						if (! soundSamples->remapVolumeContinuous)
						{
							if (auto* usingVar = get_owner()->get_variables().get_existing<float>(soundSamples->remapVolumeUsingVar))
							{
								float resVolume = soundSamples->remapVolume.remap(*usingVar);
								if (params.volume.is_set())
								{
									params.volume = params.volume.get() * resVolume;
								}
								else
								{
									params.volume = Range(resVolume);
								}
							}
						}
					}
					
					if (generalSoundVolume.is_set())
					{
						if (params.volume.is_set())
						{
							params.volume = params.volume.get() * generalSoundVolume.get();
						}
						else
						{
							params.volume = Range(generalSoundVolume.get());
						}
					}

					if (auto* played = access_sounds().play(soundSample->get(), get_owner(), _usingData, soundSamples, Transform::identity, params))
					{
						played->set_id(_name);
						played->set_tags(soundSamples->tags);
						soundCooldowns.add(_name, ! soundSamples->cooldown.is_empty() ? Optional<float>(soundSamples->cooldown.get_at(rg.get_float(0.0f, 1.0f))) : NP);
						on_play_sound(played);
						get_owner()->mark_sounds_require_advancement();
						return played;
					}
				}
			}
			else
			{
#ifdef AN_ALLOW_EXTENDED_DEBUG
				IF_EXTENDED_DEBUG(MissingSounds)
				{
					warn(TXT("couldn't find sample \"%S\" for \"%S\""), _name.to_char(), get_owner()->ai_get_name().to_char());
				}
#endif
			}
		}
	}

	return nullptr;
}

bool ModuleSound::should_sample_be_heard(Sample const * soundSample, SoundPlayParams const& _params) const
{
	Optional<int> hearingRoomDistanceRecentlySeenByPlayer = _params.hearingRoomDistanceRecentlySeenByPlayer;
	if (!hearingRoomDistanceRecentlySeenByPlayer.is_set())
	{
		if (auto* s = soundSample)
		{
			if (s->get_hearing_room_distance_recently_seen_by_player().is_set())
			{
				hearingRoomDistanceRecentlySeenByPlayer = s->get_hearing_room_distance_recently_seen_by_player();
			}
		}
	}
	if (hearingRoomDistanceRecentlySeenByPlayer.is_set())
	{
		if (get_owner()->get_room_distance_to_recently_seen_by_player() > hearingRoomDistanceRecentlySeenByPlayer.get())
		{
			return false;
		}
	}

	return true;
}

SoundSource* ModuleSound::play_sound(Sample* _sample, Optional<Name> const& _socket, Transform const& _relativePlacement, SoundPlayParams const& _params, bool _playDetached)
{
	if (!_sample)
	{
		return nullptr;
	}
	SoundPlayParams params(_params);
	for_every(ppo, playParamsOverrides)
	{
		if (_sample->get_name() == ppo->sample)
		{
			params.fill_with(ppo->params);
		}
	}

	if (generalSoundVolume.is_set())
	{
		if (params.volume.is_set())
		{
			params.volume = params.volume.get() * generalSoundVolume.get();
		}
		else
		{
			params.volume = Range(generalSoundVolume.get());
		}
	}

	if (!should_sample_be_heard(_sample, params))
	{
		return nullptr;
	}

	if (auto* played = access_sounds().play(_sample, get_owner(), nullptr, nullptr, Transform::identity, params))
	{
		played->set_id(Name::invalid());
		played->set_tags(_sample->get_tags());
		on_play_sound(played);
		get_owner()->mark_sounds_require_advancement();
		// attach for placement
		played->attach(get_owner(), nullptr, nullptr, _socket.get(Name::invalid()), _relativePlacement);
		if (! _playDetached)
		{
			played->detach_if_needed();
		}
		return played;
	}
	return nullptr;
}

SoundSource* ModuleSound::play_sound(Name const & _name, Optional<Name> const & _socket, Transform const & _relativePlacement, SoundPlayParams const & _params)
{
	return play_sound_using(_name, moduleSoundData, _socket, _relativePlacement, _params);
}

SoundSource* ModuleSound::play_sound_using(Name const & _name, ModuleSoundData const* _usingData, Optional<Name> const & _socket, Transform const & _relativePlacement, SoundPlayParams const & _params)
{
	if (_usingData)
	{
		ModuleSoundSample const * soundSample;
		if (auto* played = just_play_sound(_name, _usingData, soundSample, _params))
		{
			played->attach(get_owner(), _usingData, soundSample, _socket.is_set() ? _socket.get() : soundSample->socket, soundSample->placement.to_world(_relativePlacement));
			if (soundSample->playDetached)
			{
				played->detach_if_needed();
			}
			on_attach_placed_sound(_usingData, played, soundSample, _params);
			return played;
		}
	}

	return nullptr;
}

SoundSource* ModuleSound::play_sound_in_room(Name const & _name, Optional<Name> const & _socket, Transform const & _relativePlacement, SoundPlayParams const & _params)
{
	if (Room* inRoom = get_owner()->get_presence()->get_in_room())
	{
		Transform placementOS = _socket.is_set() && _socket.get().is_valid() ? get_owner()->get_appearance()->calculate_socket_os(_socket.get()) : Transform::identity;
		placementOS = placementOS.to_world(_relativePlacement);
		Transform placementWS = get_owner()->get_presence()->get_os_to_ws_transform().to_world(placementOS);

		get_owner()->get_presence()->move_through_doors(REF_ placementWS, REF_ inRoom);
		if (inRoom)
		{
			return play_sound_in_room(_name, REF_ inRoom, REF_ placementWS, _params);
		}
	}
	return nullptr;
}

SoundSource* ModuleSound::play_sound_in_room(Name const & _name, Room const * _room, Transform const & _placement, SoundPlayParams const & _params)
{
	if (_room)
	{
		ModuleSoundSample const * soundSample;
		if (auto* played = just_play_sound(_name, moduleSoundData, soundSample, _params))
		{
			played->place(_room, _placement);
			on_attach_placed_sound(moduleSoundData, played, soundSample, _params);
			return played;
		}
	}

	return nullptr;
}

bool ModuleSound::is_playing(Name const& _name) const
{
	for_every(sound, sounds.get_sounds())
	{
		if (auto* soundSource = sound->get())
		{
			if (soundSource->is_playing() &&
				soundSource->get_id() == _name)
			{
				return true;
			}
		}
	}
	return false;
}

bool ModuleSound::is_playing(Tags const& _tags) const
{
	for_every(sound, sounds.get_sounds())
	{
		if (auto* soundSource = sound->get())
		{
			if (soundSource->is_playing() &&
				soundSource->get_sample_instance().get_sample()->get_tags().does_match_any_from(_tags))
			{
				return true;
			}
		}
	}
	return false;
}

void ModuleSound::stop_sounds(Tags const & _tags, Optional<float> const & _fadeOut)
{
	stop_sounds_except(_tags, nullptr, _fadeOut);
}

void ModuleSound::stop_sounds_except(Tags const & _tags, SoundSource* _exceptSound, Optional<float> const & _fadeOut)
{
	for_every(sound, sounds.access_sounds())
	{
		if (auto * soundSource = sound->get())
		{
			if (soundSource != _exceptSound &&
				soundSource->is_playing() &&
				soundSource->get_sample_instance().get_sample()->get_tags().does_match_any_from(_tags))
			{
				soundSource->stop(_fadeOut);
			}
		}
	}
}

void ModuleSound::stop_sound(Sample* _sample, Optional<float> const & _fadeOut)
{
	for_every(sound, sounds.access_sounds())
	{
		if (auto * soundSource = sound->get())
		{
			if (soundSource->is_playing() &&
				soundSource->get_sample_instance().get_sample() == _sample)
			{
				soundSource->stop(_fadeOut);
			}
		}
	}
}

void ModuleSound::stop_sound(Name const & _name, Optional<float> const & _fadeOut)
{
	for_every(sound, sounds.access_sounds())
	{
		if (auto * soundSource = sound->get())
		{
			if (soundSource->is_playing() &&
				soundSource->get_id() == _name)
			{
				soundSource->stop(_fadeOut);
			}
		}
	}
}

void ModuleSound::stop_looped_sounds(Optional<float> const & _fadeOut)
{
	for_every(sound, sounds.access_sounds())
	{
		if (auto * soundSource = sound->get())
		{
			if (soundSource->is_playing() &&
				soundSource->get_sample_instance().get_sound_sample() &&
				soundSource->get_sample_instance().get_sound_sample()->is_looped())
			{
				soundSource->stop(_fadeOut);
			}
		}
	}
}

void ModuleSound::on_attach_placed_sound(ModuleSoundData const* _usingData, SoundSource* played, ModuleSoundSample const * soundSample, SoundPlayParams const & _params)
{
	an_assert(soundSample);
	if (soundSample->onStartPlay.is_valid())
	{
		ModuleSoundSample const *soundSamplePlayed;
		if (SoundSource* onStartPlayed = just_play_sound(soundSample->onStartPlay, _usingData, soundSamplePlayed))
		{
			onStartPlayed->attach_or_place_as(played);
			on_attach_placed_sound(_usingData, onStartPlayed, soundSamplePlayed, _params);
			played->set_on_start_played(onStartPlayed);
		}
	}
	played->set_on_stop_play(soundSample->onStopPlay);

	static int const MAX_ROOM_TO_SCAN = 64;
	ARRAY_STACK(Sound::ScannedRoom, rooms, MAX_ROOM_TO_SCAN);

	for_every(aiMessage, soundSample->aiMessages)
	{
		if (aiMessage->playerOnly)
		{
			if (! GameUtils::is_controlled_by_player(get_owner()))
			{
				continue;
			}
		}
		if (aiMessageCooldowns.is_available(aiMessage->name))
		{
			rooms.clear();
			Sound::ScannedRoom::find(rooms, played->get_in_room(), played->get_placement_in_room(played->get_in_room()).get_translation(),
				Sound::ScanRoomInfo()
					.with_concurrent_rooms_limit(1) // one is enough, get closest
					.with_depth_limit(6)
					.with_max_distance(played->get_hearing_distance())
					.with_max_distance_beyond_first_room(played->get_hearing_distance_beyond_first_room())
					.with_propagation_dir());
			for_every(room, rooms)
			{
				FOR_EVERY_OBJECT_IN_ROOM(object, room->room) // may be called at any time, logic, post move
				{
					// only objects with AI can hear it
					if (object != get_owner() && object->get_ai())
					{
						if (auto* message = get_owner()->get_in_world()->create_ai_message(aiMessage->name))
						{
							message->delayed(aiMessage->delay);
							message->access_param(NAME(who)).access_as<SafePtr<Framework::IModulesOwner>>() = get_owner()->get_top_instigator();
							message->access_param(NAME(what)).access_as<SafePtr<Framework::IModulesOwner>>() = get_owner();
							message->access_param(NAME(room)).access_as<SafePtr<Room>>() = room->room;
							message->access_param(NAME(location)).access_as<Vector3>() = room->entrancePoint;
							message->access_param(NAME(propagationDir)).access_as<Vector3>() = room->propagationDir;
							message->access_param(NAME(throughDoor)).access_as<SafePtr<DoorInRoom>>() = room->enteredThroughDoor;
							message->access_param(NAME(sound)).access_as<SoundSourcePtr>() = played;
							message->access_param(NAME(soundId)).access_as<Name>() = soundSample->id;
							message->to_ai_object(object);
						}
					}
				}
			}

			aiMessageCooldowns.add(*aiMessage);
		}
	}
}

void ModuleSound::clear_play_params_overrides()
{
	MODULE_OWNER_LOCK(TXT("play params setup"));
	playParamsOverrides.clear();
}

void ModuleSound::clear_play_params_overrides(Name const& _sound)
{
	MODULE_OWNER_LOCK(TXT("play params setup"));
	for_every(ppo, playParamsOverrides)
	{
		if (ppo->sound == _sound)
		{
			playParamsOverrides.remove_at(for_everys_index(ppo));
			break;
		}
	}
}

void ModuleSound::clear_play_params_overrides(Framework::LibraryName const& _sample)
{
	MODULE_OWNER_LOCK(TXT("play params setup"));
	for_every(ppo, playParamsOverrides)
	{
		if (ppo->sample == _sample)
		{
			playParamsOverrides.remove_at(for_everys_index(ppo));
			break;
		}
	}
}

void ModuleSound::set_play_params_overrides(Name const& _sound, SoundPlayParams const& _params)
{
	MODULE_OWNER_LOCK(TXT("play params setup"));
	for_every(ppo, playParamsOverrides)
	{
		if (ppo->sound == _sound)
		{
			ppo->params = _params;
			return;
		}
	}
	SoundPlayParamsOverride ppo;
	ppo.sound = _sound;
	ppo.params = _params;
	playParamsOverrides.push_back(ppo);
}

void ModuleSound::set_play_params_overrides(Framework::LibraryName const& _sample, SoundPlayParams const& _params)
{
	MODULE_OWNER_LOCK(TXT("play params setup"));
	for_every(ppo, playParamsOverrides)
	{
		if (ppo->sample == _sample)
		{
			ppo->params = _params;
			return;
		}
	}
	SoundPlayParamsOverride ppo;
	ppo.sample = _sample;
	ppo.params = _params;
	playParamsOverrides.push_back(ppo);
}

