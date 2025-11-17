#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\core\sound\playback.h"

#include "..\..\framework\library\usedLibraryStored.h"
#include "..\..\framework\general\cooldowns.h"

namespace Framework
{
	class Door;
	class DoorInRoom;
	class Room;
	class Sample;
	class SoundSource;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class EnvironmentPlayer
	{
	public:
		EnvironmentPlayer();
		~EnvironmentPlayer();

		static void load_on_demand_if_required_for(Framework::Room* _room);

		void pause();
		void resume();

		void clear();

		void set_dull(float _dull = 0.0f, float _dullBlendTime = 0.0f);

		void update(Framework::IModulesOwner* _imo, float _deltaTime);

		void update_effects();

	public:
		void log(LogInfoContext& _log) const;

	private:
		mutable Concurrency::SpinLock accessLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.EnvironmentPlayer.accessLock"));

		bool isPaused = false;

		float dull = 0.0f;
		float dullBlendTime = 0.0f;
		float dullTarget = 0.0f;

		Framework::UsedLibraryStored<Framework::Sample> windSample;
		Framework::UsedLibraryStored<Framework::Sample> windBeyondDoorSample;
		Framework::UsedLibraryStored<Framework::Sample> windBlowSample;
		Framework::UsedLibraryStored<Framework::Sample> desertWindSample;
		Framework::UsedLibraryStored<Framework::Sample> desertWindBeyondDoorSample;
		Array<Framework::UsedLibraryStored<Framework::Sample>> customSamples;

		CACHED_ Framework::Room const* checkedForRoom = nullptr;

		Framework::Cooldowns<32> cooldowns;

		struct Playback
		{
			Name id;
			Framework::Sample* sample;
			Sound::Playback playback; // to pause/resume/fade out/log
		};
		List<Playback> enviroEffects;
		List<Playback> enviroEffectsOnEntrace;

		struct DoorPlayback
		{
			Name id;
			SafePtr<Framework::Door> throughDoor;
			Framework::Sample* sample;
			RefCountObjectPtr<Framework::SoundSource> soundSource;
		};
		List<DoorPlayback> doorSounds;

		void play_effect(Name const& _id, Framework::Sample* _sample);
		void stop_effects();
		void play_effect_on_entrance(Name const& _id, Framework::Sample* _sample);

		void play_door_sound(Name const& _id, Framework::Sample* _sample, Framework::Room * _room, Framework::Door* _throughDoor, Transform const & _placementWS);

		void stop_door_sounds();

		Framework::Sample* get_custom_sample(Framework::LibraryName const& _name);
	};
};
