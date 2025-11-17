#pragma once

#include "soundSource.h"

#include "..\..\core\math\math.h"
#include "..\..\core\memory\pooledObject.h"

namespace Framework
{
	interface_class IModulesOwner;
	class ModuleSoundData;
	class Room;
	class Sample;
	struct ModuleSoundSample;

	/**
	 *	Manages adding, playing sounds.
	 *
	 *	When sound is added, it can be done anywhere within the game frame. It is stored in addedSounds.
	 *	At the end of game frame (or anytime else), when advance is called, all sounds in sounds are advanced (and removed if necessary) and new sounds are moved to sounds.
	 *
	 *	This way newly added sounds won't be advanced (!), they will have "at" set to 0.0 and "just started" marked.
	 */
	class SoundSources
	{
	public:
		SoundSources();
		virtual ~SoundSources();

		SoundSource* play(Sample* _sample, SoundPlayParams const & _params = NP);
		SoundSource* play(Sample* _sample, IModulesOwner* _owner, ModuleSoundData const * _usingSoundData, ModuleSoundSample const* _usingDataSample, Transform const & _relativePlacement = Transform::identity, SoundPlayParams const & _params = NP);
		SoundSource* play(Sample* _sample, Door* _door, Transform const & _relativePlacement = Transform::identity, SoundPlayParams const & _params = NP);
		SoundSource* play(Sample* _sample, Room* _inRoom, Transform const & _placement, SoundPlayParams const & _params = NP);

		SoundSource* find_played(Sample* _sample) { return find_played(_sample, nullptr, nullptr, nullptr); }
		SoundSource* find_played(Sample* _sample, IModulesOwner* _owner) { return find_played(_sample, _owner, nullptr, nullptr); }
		SoundSource* find_played(Sample* _sample, Door* _door) { return find_played(_sample, nullptr, _door, nullptr); }
		SoundSource* find_played(Sample* _sample, Room* _inRoom) { return find_played(_sample, nullptr, nullptr, _inRoom); }

		bool is_empty() const { return sounds.is_empty(); }

		bool does_require_advance() const;
		void advance(float _deltaTime); // removes sound that are no longer playing, moves recently added to sounds

		Array<RefCountObjectPtr<SoundSource>> & access_sounds() { return sounds; }
		Array<RefCountObjectPtr<SoundSource>> const & get_sounds() const { return sounds; }

		void stop_all_immediately(); // stop and remove!
		void stop_all(float _fadeOut);

	private:
		Array<RefCountObjectPtr<SoundSource>> sounds;

		Concurrency::SpinLock addedSoundsLock = Concurrency::SpinLock(TXT("Framework.SoundSources.addedSoundsLock"));
		Array<RefCountObjectPtr<SoundSource>> addedSounds;

		void add(SoundSource* _sound);
		SoundSource* find_played(Sample* _sample, IModulesOwner* _owner, Door* _door, Room* _inRoom);
	};

};
