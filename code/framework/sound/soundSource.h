#pragma once

#include "soundPlayParams.h"
#include "soundSampleInstance.h"

#include "..\modulesOwner\modulesOwner.h"

#include "..\..\core\math\math.h"
#include "..\..\core\memory\pooledObject.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\tags\tag.h"

namespace Framework
{
	class Door;
	class Room;
	class ModuleSoundData;
	class Sample;
	class SoundSource;
	struct ModuleSoundSample;

	namespace Sound
	{
		class Scene;
	};

	class SoundSourceModifier;
	
	typedef RefCountObjectPtr<SoundSource> SoundSourcePtr;

	/**
	 *	Sound source is something that can be placed within world.
	 *	It's better to manage it through SoundSources, as it might be inside module or just placed in the room.
	 *	You can find more information about how they are advanced in SoundSources info.
	 *	It handles fading in and out on its own.
	 */
	class SoundSource
	: public PooledObject<SoundSource>
	, public RefCountObject
	{
	public:
		SoundSource();
		SoundSource(SoundSource const & _other);
		virtual ~SoundSource();

		SoundSource & operator= (SoundSource const & _other);

		Name const & get_id() const { return id; }
		void set_id(Name const & _id) { id = _id; }

		Tags const & get_tags() const { return tags; }
		void set_tags(Tags const & _tags) { tags.set_tags_from(_tags); }

		void play(Sample* _sample, SoundPlayParams const & _params = NP); // mark to play (!) it will be played by sound scene
		void stop(Optional<float> const & _fadeOut = NP); // this allows to stop with fade out
		
		void add_modifier(SoundSourceModifier* _modifier);
		void remove_modifier(SoundSourceModifier* _modifier);

		SampleInstance & access_sample_instance() { return sampleInstance; }
		SampleInstance const & get_sample_instance() const { return sampleInstance; }

		void attach(IModulesOwner* _owner, ModuleSoundData const * _usingSoundData, ModuleSoundSample const* _usingDataSample, Transform const & _relativePlacement);
		void attach(IModulesOwner* _owner, ModuleSoundData const* _usingSoundData, ModuleSoundSample const* _usingDataSample, Name const & _socket, Transform const & _relativePlacement = Transform::identity);
		void attach(Door* _door, Transform const & _relativePlacement);
		void detach_if_needed();
		void place(Room const * _inRoom, Transform const & _placement);

		void attach_or_place_as(SoundSource const * _other);

		IModulesOwner* get_owner() const { return owner.get(); }
		ModulePresence* get_presence() const { return owner.get()? owner->get_presence() : nullptr; }
		Door* get_door() const { return door.get(); }

		bool advance(float _deltaTime); // returns true if should stay

		Transform get_placement_in_room(Room* _inRoom) const; // calculate placement
		Room* get_in_room() const;

		// use it to jump to particular place
		void be_at(float _at) { beAt = _at; clearBeAt = false; }
		Optional<float> const & get_be_at() const { return beAt; }

		// to have information where we are
		void store_at(float _at) { at = _at; }
		float get_at() const { return at; }

		bool is_active() const { return is_playing() && !should_stop(); }
		bool is_playing() const { return isPlaying; }
		bool has_just_started() const { return justStarted; }
		bool should_stop() const { return shouldStop; }
		bool is_dropped() const { return isDropped; }

		void set_auto_stop_time(Optional<float> _autoStopTime = NP) { autoStopTime = _autoStopTime; }

		// overriden with sound play params etc
		bool is_hearable_only_by_owner() const { return hearableOnlyByOwner; }
		float get_fade_in() const { return fadeIn.is_set() ? fadeIn.get() : 0.0f; }
		float get_fade_out() const { return fadeOut.is_set() ? fadeOut.get() : 0.0f; }
		float get_early_fade_out() const { return earlyFadeOut.is_set() ? earlyFadeOut.get() : 0.0f; }
		Optional<float> const & get_pitch_related_to_fade_base() const { return pitchRelatedToFadeBase; }
		Optional<Range> const & get_distances() const { return distances; }
		float get_hearing_distance() const { return hearingDistance; }
		Optional<float> const & get_hearing_distance_beyond_first_room() const { return hearingDistanceBeyondFirstRoom; }

		// mapping etc
		float get_volume_alterations() const;
		float get_pitch_alterations() const;

	private: friend class Sound::Scene;
		void drop(); // will just marked that it is dropped

	private: friend class ModuleSound;
		void set_on_start_played(SoundSource* _onStartPlayed) { onStartPlayed = _onStartPlayed; }
		void set_on_stop_play(Name const & _onStopPlay) { onStopPlay = _onStopPlay; }

	private:
		Name id; // for reference, sound module identifies sounds this way
		Tags tags; // for any extra feature to allow removal of specific sounds etc

		::SafePtr<IModulesOwner> owner;
		ModuleSoundData const* usingModuleSoundData = nullptr; // this should exist as long as library exists
		ModuleSoundSample const* usingModuleSoundSample = nullptr; // this should exist as long as library exists
		::SafePtr<Door> door;
		::SafePtr<Room> inRoom; // if no owner provided
		Optional<Transform> placement; // in room or relative to owner
		Name socket;

		// if owner and has sound module
		SoundSourcePtr onStartPlayed;
		Name onStopPlay;

		bool hearableOnlyByOwner = false;
		SampleInstance sampleInstance;
		Optional<float> beAt; // to control where should it play now
		bool clearBeAt = false;
		float at = 0.0f; // this is information used when sound should be played in the middle, should not be used to sync distances etc
		Optional<float> autoStopTime;
		Optional<float> autoDropTime; // to allow dropping on stop (without this only sound scene drops it
		bool isPlaying = false;
		bool justStarted = true;
		bool shouldStop = false;
		bool isDropped = false;
		Optional<float> fadeIn;
		Optional<float> fadeOut;
		Optional<float> earlyFadeOut;
		Optional<float> pitchRelatedToFadeBase; // how fade affects pitch final pitch = base + (1.0 - base) * fade
		Optional<Range> distances;
		float hearingDistance = 0.0f;
		Optional<float> hearingDistanceBeyondFirstRoom;

		Array<RefCountObjectPtr<SoundSourceModifier>> modifiers;
	};

};

DECLARE_REGISTERED_TYPE(RefCountObjectPtr<Framework::SoundSource>);
