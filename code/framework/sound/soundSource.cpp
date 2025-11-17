#include "soundSource.h"

#include "soundSample.h"
#include "soundSourceModifier.h"

#include "..\modulesOwner\modulesOwner.h"
#include "..\module\moduleAppearance.h"
#include "..\module\modulePresence.h"
#include "..\module\moduleSound.h"
#include "..\module\moduleSoundData.h"
#include "..\world\door.h"
#include "..\world\doorInRoom.h"
#include "..\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

SoundSource::SoundSource()
{
}

SoundSource::~SoundSource()
{
	while (!modifiers.is_empty())
	{
		remove_modifier(modifiers.get_first().get());
	}
}

SoundSource::SoundSource(SoundSource const & _other)
{
	an_assert(false, TXT("do not use this"));
}

SoundSource & SoundSource::operator= (SoundSource const & _other)
{
	an_assert(false, TXT("do not use this"));
	return *this;
}

void SoundSource::play(Sample* _sample, SoundPlayParams const & _params)
{
	isPlaying = true;
	sampleInstance.set(_sample);
	at = sampleInstance.get_sound_sample() ? sampleInstance.get_sound_sample()->get_length() * _sample->get_start_at_pt() : 0.0f;
	justStarted = true;
	shouldStop = false;
	fadeIn = _params.fadeIn.get(_sample->get_fade_in_time());
	fadeOut = _params.fadeOut.get(_sample->get_fade_out_time());
	earlyFadeOut = _params.earlyFadeOut.get(_sample->get_early_fade_out_time());
	hearingDistance = _params.hearingDistance.get(_sample->get_hearing_distance());
	hearableOnlyByOwner = _params.hearableOnlyByOwner.get(hearableOnlyByOwner);
	hearingDistanceBeyondFirstRoom = _params.hearingDistanceBeyondFirstRoom;
	if (!hearingDistanceBeyondFirstRoom.is_set() && _sample->get_hearing_distance_beyond_first_room().is_set())
	{
		hearingDistanceBeyondFirstRoom = _sample->get_hearing_distance_beyond_first_room();
	}
	pitchRelatedToFadeBase = _params.pitchRelatedToFadeBase;
	if (_params.volume.is_set())
	{
		sampleInstance.set_sample_volume(sampleInstance.get_sample_volume() * Random::get(_params.volume.get()));
	}
	if (_params.pitch.is_set())
	{
		sampleInstance.set_sample_pitch(sampleInstance.get_sample_pitch() * Random::get(_params.pitch.get()));
	}
	if (_params.distances.is_set())
	{
		distances = _params.distances.get();
	}
	if (_params.hearingDistanceBeyondFirstRoom.is_set())
	{
		hearingDistanceBeyondFirstRoom = _params.hearingDistanceBeyondFirstRoom;
	}
}

void SoundSource::stop(Optional<float> const & _fadeOut)
{
	if (shouldStop)
	{
		// already issued
		return;
	}

	if (onStopPlay.is_valid())
	{
		if (auto* imo = owner.get())
		{
			if (auto* ms = imo->get_sound())
			{
				if (SoundSource* onStopPlayed = ms->play_sound_using(onStopPlay, usingModuleSoundData, socket, placement.is_set() ? placement.get() : Transform::identity))
				{
					onStopPlayed->attach_or_place_as(this);
				}
			}
		}
	}
	if (auto* osp = onStartPlayed.get())
	{
		if (osp->is_playing())
		{
			osp->stop();
		}
		onStartPlayed.clear();
	}

	shouldStop = true;
	if (_fadeOut.is_set())
	{
		fadeOut = _fadeOut;
	}

	autoDropTime = (fadeOut.is_set()? fadeOut.get() : 0.0f) + 0.1f; // add minor margin 
}

void SoundSource::drop()
{
	isPlaying = false;
	isDropped = true;
	shouldStop = true;
}

bool SoundSource::advance(float _deltaTime)
{
	for_every_ref(modifier, modifiers)
	{
		modifier->advance(_deltaTime);
	}
	justStarted = false;
	bool advanceAt = true;
	if (beAt.is_set())
	{
		// we need one frame of delay to clear "be at"
		if (clearBeAt)
		{
			beAt.clear();
		}
		else
		{
			at = beAt.get(); // update at
			advanceAt = false;
			clearBeAt = true;
		}
	}
	if (autoDropTime.is_set())
	{
		autoDropTime = autoDropTime.get() - _deltaTime;
		if (autoDropTime.get() <= 0.0f)
		{
			drop();
		}
	}
	if (sampleInstance.is_set() && ! isDropped)
	{
		if (autoStopTime.is_set())
		{
			autoStopTime = autoStopTime.get() - _deltaTime;
			if (autoStopTime.get() <= 0.0f)
			{
				autoStopTime.clear();
				stop();
			}
		}
		::Sound::Sample const * soundSample = sampleInstance.get_sound_sample();
		float pitch = sampleInstance.get_final_pitch();
		pitch *= get_pitch_alterations();
		if (advanceAt)
		{
			at += _deltaTime * pitch;
		}
		float const sampleLength = soundSample->get_length();
		if (soundSample->is_looped())
		{
			at = mod(at, sampleLength);
		}
		else
		{
			if (at >= sampleLength)
			{
				// not looped, mark to stop and that it won't be heard anyway
				stop();
				drop();
				return false;
			}
			else if (pitch != 0.0f)
			{
				// mark to fade out, order to stop with early fade out
				float earlyFadeOutTime = get_early_fade_out();
				if (earlyFadeOutTime > 0.0f && 
					at >= sampleLength - earlyFadeOutTime / pitch)
				{
					stop((sampleLength - at) / pitch);
				}
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}

Transform SoundSource::get_placement_in_room(Room* _inRoom) const
{
	if (owner.get())
	{
		if (socket.is_valid())
		{
			Transform placementOS = Transform::identity;
			if (ModuleAppearance const * appearance = owner->get_appearance())
			{
				placementOS = appearance->calculate_socket_os(socket);
			}
			if (ModulePresence const * presence = owner->get_presence())
			{
				an_assert(presence->get_in_room() == _inRoom);
				return presence->get_placement().to_world(placementOS.to_world(placement.is_set()? placement.get() : Transform::identity));
			}
		}
		if (ModulePresence const * presence = owner->get_presence())
		{
			an_assert(presence->get_in_room() == _inRoom);
			return presence->get_placement().to_world(placement.is_set() ? placement.get() : Transform::identity);
		}
	}
	if (door.get())
	{
		if (door->get_linked_door_a() && door->get_linked_door_a()->get_in_room() == _inRoom)
		{
			return Transform(door->get_linked_door_a()->get_hole_centre_WS(), door->get_linked_door_a()->get_placement().get_orientation()).to_world(placement.is_set() ? placement.get() : Transform::identity);
		}
		if (door->get_linked_door_b() && door->get_linked_door_b()->get_in_room() == _inRoom)
		{
			return Transform(door->get_linked_door_b()->get_hole_centre_WS(), door->get_linked_door_b()->get_placement().get_orientation()).to_world(placement.is_set() ? placement.get() : Transform::identity);
		}
	}
	if (placement.is_set())
	{
		return placement.get();
	}
	an_assert(false, TXT("not set?"));
	return Transform::identity;
}

Room* SoundSource::get_in_room() const
{
	if (owner.get())
	{
		if (ModulePresence const * presence = owner->get_presence())
		{
			return presence->get_in_room();
		}
	}
	return inRoom.get();
}

void SoundSource::attach_or_place_as(SoundSource const * _other)
{
	if (auto* newOwner = _other->owner.get())
	{
		attach(newOwner, _other->usingModuleSoundData, _other->usingModuleSoundSample, _other->socket, _other->placement.is_set()? _other->placement.get() : Transform::identity);
	}
	else if (auto* newDoor = _other->door.get())
	{
		attach(newDoor, _other->placement.is_set() ? _other->placement.get() : Transform::identity);
	}
	else if (auto* newRoom = _other->inRoom.get())
	{
		place(newRoom, _other->placement.is_set() ? _other->placement.get() : Transform::identity);
	}
}

float SoundSource::get_pitch_alterations() const
{
	float result = 1.0f;
	if (owner.is_set() && usingModuleSoundSample &&
		usingModuleSoundSample->remapPitchContinuous &&
		usingModuleSoundSample->remapPitch.is_valid() &&
		usingModuleSoundSample->remapPitchUsingVar.is_valid())
	{
		if (auto* usingVar = get_owner()->get_variables().get_existing<float>(usingModuleSoundSample->remapPitchUsingVar))
		{
			result *= usingModuleSoundSample->remapPitch.remap(*usingVar);
		}
	}
	return result;
}

float SoundSource::get_volume_alterations() const
{
	float result = 1.0f;
	if (owner.is_set() && usingModuleSoundSample &&
		usingModuleSoundSample->remapVolumeContinuous &&
		usingModuleSoundSample->remapVolume.is_valid() &&
		usingModuleSoundSample->remapVolumeUsingVar.is_valid())
	{
		if (auto* usingVar = get_owner()->get_variables().get_existing<float>(usingModuleSoundSample->remapVolumeUsingVar))
		{
			result *= usingModuleSoundSample->remapVolume.remap(*usingVar);
		}
	}
	return result;
}

void SoundSource::attach(IModulesOwner* _owner, ModuleSoundData const* _usingSoundData, ModuleSoundSample const* _usingDataSample, Transform const & _relativePlacement)
{
	owner = _owner;
	usingModuleSoundData = _usingSoundData;
	usingModuleSoundSample = _usingDataSample;
	inRoom = nullptr;
	placement = _relativePlacement;
	socket = Name::invalid();
}

void SoundSource::attach(IModulesOwner* _owner, ModuleSoundData const* _usingSoundData, ModuleSoundSample const* _usingDataSample, Name const & _socket, Transform const & _relativePlacement)
{
	owner = _owner;
	usingModuleSoundData = _usingSoundData;
	usingModuleSoundSample = _usingDataSample;
	door = nullptr;
	inRoom = nullptr;
	placement = _relativePlacement;
	socket = _socket;
}

void SoundSource::attach(Door* _door, Transform const & _relativePlacement)
{
	owner = nullptr;
	usingModuleSoundData = nullptr;
	usingModuleSoundSample = nullptr;
	door = _door;
	inRoom = nullptr;
	placement = _relativePlacement;
	socket = Name::invalid();
}

void SoundSource::place(Room const * _inRoom, Transform const & _placement)
{
	owner = nullptr;
	usingModuleSoundData = nullptr;
	usingModuleSoundSample = nullptr;
	inRoom = _inRoom;
	placement = _placement;
	socket = Name::invalid();
}

void SoundSource::detach_if_needed()
{
	if (owner.get())
	{
		place(get_in_room(), get_placement_in_room(get_in_room()));
	}
}

void SoundSource::add_modifier(SoundSourceModifier* _modifier)
{
	for_every_ref(mod, modifiers)
	{
		if (mod == _modifier)
		{
			return;
		}
	}

	modifiers.push_back(RefCountObjectPtr<SoundSourceModifier>(_modifier));
	_modifier->added_to(this);
}

void SoundSource::remove_modifier(SoundSourceModifier* _modifier)
{
	for_every_ref(mod, modifiers)
	{
		if (mod == _modifier)
		{
			_modifier->removed_from(this);
			modifiers.remove_at(for_everys_index(mod));
			return;
		}
	}
}
