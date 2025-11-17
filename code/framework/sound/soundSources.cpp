#include "soundSources.h"

#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\containers\arrayStack.h"

using namespace Framework;

SoundSources::SoundSources()
{
}

SoundSources::~SoundSources()
{
}

void SoundSources::stop_all_immediately()
{
	for_every(sound, sounds)
	{
		sound->get()->stop(0.0f);
	}
	sounds.clear();
}

void SoundSources::stop_all(float _fadeOut)
{
	for_every(sound, sounds)
	{
		sound->get()->stop(_fadeOut);
	}
}

SoundSource* SoundSources::play(Sample* _sample, SoundPlayParams const & _params)
{
	SoundSource* sound = new SoundSource();
	sound->play(_sample, _params);

	add(sound);

	return sound;
}

SoundSource* SoundSources::play(Sample* _sample, IModulesOwner* _owner, ModuleSoundData const * _usingSoundData, ModuleSoundSample const* _usingDataSample, Transform const & _relativePlacement, SoundPlayParams const & _params)
{
	SoundSource* sound = new SoundSource();
	sound->play(_sample, _params);
	sound->attach(_owner, _usingSoundData, _usingDataSample, _relativePlacement);

	add(sound);

	return sound;
}

SoundSource* SoundSources::play(Sample* _sample, Door* _door, Transform const & _relativePlacement, SoundPlayParams const & _params)
{
	SoundSource* sound = new SoundSource();
	sound->play(_sample, _params);
	sound->attach(_door, _relativePlacement);

	add(sound);

	return sound;
}

SoundSource* SoundSources::play(Sample* _sample, Room* _inRoom, Transform const & _placement, SoundPlayParams const & _params)
{
	SoundSource* sound = new SoundSource();
	sound->play(_sample, _params);
	sound->place(_inRoom, _placement);

	add(sound);

	return sound;
}

SoundSource* SoundSources::find_played(Sample* _sample, IModulesOwner* _owner, Door* _door, Room* _inRoom)
{
	for_every(sound, sounds)
	{
		if (sound->get()->get_sample_instance().is_set() &&
			!sound->get()->should_stop() &&
			sound->get()->get_sample_instance().get_sample() == _sample &&
			((sound->get()->get_owner() && sound->get()->get_owner() == _owner) || 
			 (sound->get()->get_door() && sound->get()->get_door() == _door) ||
			 (! sound->get()->get_owner() && !sound->get()->get_door() && sound->get()->get_in_room() == _inRoom)))
		{
			return sound->get();
		}
	}

	// check added
	{
		Concurrency::ScopedSpinLock lock(addedSoundsLock);
		// move new sounds from added sounds
		for_every(sound, addedSounds)
		{
			if (sound->get()->get_sample_instance().is_set() &&
				!sound->get()->should_stop() &&
				sound->get()->get_sample_instance().get_sample() == _sample &&
				((sound->get()->get_owner() && sound->get()->get_owner() == _owner) || 
				 (! sound->get()->get_owner() && sound->get()->get_in_room() == _inRoom)))
			{
				return sound->get();
			}
		}
	}

	return nullptr;
}

void SoundSources::add(SoundSource* _sound)
{
	Concurrency::ScopedSpinLock lock(addedSoundsLock);
	addedSounds.push_back(RefCountObjectPtr<SoundSource>(_sound));
}

bool SoundSources::does_require_advance() const
{
	return !sounds.is_empty() || !addedSounds.is_empty();
}

void SoundSources::advance(float _deltaTime)
{
	if (!sounds.is_empty())
	{
		ARRAY_STACK(bool, shouldStay, sounds.get_size());
		shouldStay.set_size(sounds.get_size());
		bool allStay = true;

		{
			bool* stay = shouldStay.get_data();
			for_every(sound, sounds)
			{
				an_assert(sound->is_set());
				bool thisStays = sound->get()->advance(_deltaTime);
				*stay = thisStays;
				allStay &= thisStays;
				++stay;
			}
		}

		if (!allStay)
		{
			// compress array and release all that are no longer needed, this is better than using .remove_at, as we modify it without multiple copying
			int soundStaySize = 0;
			RefCountObjectPtr<SoundSource>* dest = sounds.get_data();
			RefCountObjectPtr<SoundSource>* src = sounds.get_data();
			for_every(stay, shouldStay)
			{
				if (*stay)
				{
					*dest = *src;
					++dest;
					++soundStaySize;
				}
				else
				{
					(*src)->detach_if_needed();
					(*src) = nullptr;
				}
				++src;
			}
			sounds.set_size(soundStaySize);
		}
	}

	if (!addedSounds.is_empty())
	{
		Concurrency::ScopedSpinLock lock(addedSoundsLock);
		// move new sounds from added sounds
		sounds.make_space_for_additional(addedSounds.get_size());
		for_every(sound, addedSounds)
		{
			an_assert(sound->is_set());
			sounds.push_back(*sound);
		}
		addedSounds.clear();
	}
}
