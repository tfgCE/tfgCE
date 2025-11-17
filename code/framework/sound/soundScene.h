#pragma once

#include "soundCamera.h"

#include "..\presence\presencePath.h"
#include "..\sound\soundSource.h"

#include "..\..\core\memory\softPooledObject.h"
#include "..\..\core\sound\playback.h"
#include "..\..\core\sound\reverbInstance.h"

namespace Framework
{
	class DoorInRoom;
	class ModulePresence;
	class SoundSources;
	class Reverb;

	namespace Sound
	{
		class Camera;
		class Context;

		struct ScanRoomInfo
		{
			ScanRoomInfo& with_concurrent_rooms_limit(Optional<int> const & _o) { concurrentRoomsLimit = _o; return *this; }
			ScanRoomInfo& with_depth_limit(Optional<int> const & _o) { depthLimit = _o; return *this; }
			ScanRoomInfo& with_max_distance(Optional<float> const & _o) { maxDistance = _o; return *this; }
			ScanRoomInfo& with_max_distance_beyond_first_room(Optional<float> const & _o) { maxDistanceBeyondFirstRoom = _o; return *this; }
			ScanRoomInfo& with_propagation_dir(bool _withPropagationDir = true) { withPropagationDir = _withPropagationDir; return *this; }

			Optional<int> concurrentRoomsLimit;
			Optional<int> depthLimit;
			Optional<float> maxDistance;
			Optional<float> maxDistanceBeyondFirstRoom;
			bool withPropagationDir = false;
		};

		struct ScannedRoom
		{
			Room* room;
			Transform relativeRoomPlacement = Transform::identity; // relative to start
			ScannedRoom* enteredFrom;
			DoorInRoom* enteredThroughDoor;
			Vector3 entrancePoint; // to make it easier to approximate distance
			Vector3 propagationDir; // so we see in what direction does the sound go
			float soundLevel; // doors will oclude it
			float distance; // this is not actual distance, this is distance used to sort which rooms should be checked
			float distanceBeyondFirstRoom; // this is not actual distance, this is distance used to sort which rooms should be checked
			int depth;
			bool scanned;

			ScannedRoom() {}
			ScannedRoom(Room* _room, ScannedRoom* _enteredFrom, Transform const& _relativeRoomPlacement, DoorInRoom* _enteredThroughDoor, Vector3 const & _propagationDir, float _distance, float _distanceBeyondFirstRoom, int _depth);
			ScannedRoom(Room* _room, Vector3 const & _location);

			static void find(REF_ ArrayStack<ScannedRoom> & _rooms, Room* _startRoom, Vector3 const & _startLoc, ScanRoomInfo & _context);
		private:
			static void scan_room(ScannedRoom & _room, REF_ ArrayStack<ScannedRoom> & _roomsToScan, ScanRoomInfo & _context);
		};

		namespace PlayingSceneSoundState
		{
			enum Type
			{
				NotUsed, // not being played
				InUse, // played, active
				FadingOut // still requires updating, although it is now just fading out (it is sound being stopped, not scene fading), during scene fade out we do not update sound location atm
			};
		};

		struct PlayingSceneSound
		{
			PlayingSceneSoundState::Type state = PlayingSceneSoundState::NotUsed; // because we don't want to move things around, we may have holes in sound array
			bool shouldBeHeard = false; // should this sound be heard?

			// locations are relative to camera (!)
			Vector3 stringPulledLocation;
			int soundSamplePriority;
			float stringDistance; // along string
			float stringDistancePt; // rel to hearing distance
			float stringDistanceBeyondFirstRoom; // as above but without first room
			Optional<float> occlusionLevel;
			CACHED_ Vector3 apparentSoundLocation;
			CACHED_ Optional<Vector3> apparentVelocity;
			// store separately prev and current only for apparent velocity purposes
			CACHED_ float apparentVelocityPrevStringDistance;
			CACHED_ float apparentVelocityStringDistance;
			CACHED_ float apparentVelocityDeltaTime;

			// scene fade, how much 
			float currentSceneFade = 0.0f;
			float targetSceneFade = 0.0f;

			PresencePath path; // this is for reference to know which sound it is, it is from owner to sound source
			Room const * roomRef = nullptr; // if path is invalid (2d sound) we compare roomRef, if not set, we assume it is 2d
			RefCountObjectPtr<SoundSource> soundSource;
			::Sound::Playback playback;

			void update_apparent_sound_location();
		};

		struct SceneSoundReverb
		{
			::Sound::ReverbInstance reverbInstance;
			Reverb* reverb = nullptr;
			float target = 0.0f;
		};

		struct SceneUpdateContext
		{
			float dull = 0.0f; // to use low pass
		};

		/**
		 *	Sound scene is kept between frames and updated
		 *	This is something that manages sound sources, should they play, how should they play etc.
		 */
		class Scene
		{
		public:
			Scene();
			~Scene();

			void update(float _deltaTime, ::Framework::Sound::Camera const & _camera, ModulePresence* _owner, Optional<SceneUpdateContext> const & _context = NP);
			void update(float _deltaTime, SoundSources & _soundSources, Optional<SceneUpdateContext> const& _context = NP); // just rely on sound sources

			void clear();

			Camera const & get_camera() const { return camera; }

			void pause();
			void resume();
			bool is_paused() const { return isPaused; }

			void log(LogInfoContext & _log, bool _skipNotUsed = false, Optional<Colour> const & _defaultColour = NP) const;

			void add_extra_playback(::Sound::Playback const& _playback, Name const& _id = Name::invalid(), Optional<float> const& _fadeOutTime = NP); // it's a good thing to register extra sounds (played in the world) in a scene so we can pause them all
			void stop_extra_playback(Name const & _id, Optional<float> const & _fadeOutTime = NP); // it's a good thing to register extra sounds (played in the world) in a scene so we can pause them all

		private:
			static int const MAX_ROOM_TO_SCAN = 64;

			Camera camera;
			ModulePresence * owner = nullptr; // owner should remain the same, if we switch owner, all sounds go out of the window
			SafePtr<IModulesOwner> anyOwnerIMO; // this is required to have presence paths working properly, there should be any imo to work as a point of reference
			Array<PlayingSceneSound> sounds;
			int soundsLimit; // constructor sets it properly
			Array<SceneSoundReverb> reverbs;
			bool isPaused = false;
			Concurrency::SpinLock soundsOpLock = Concurrency::SpinLock(TXT("Framework.Sound.Scene.soundsOpLock")); // operation performed on sounds

			struct ExtraPlayback
			{
				Name id;
				::Sound::Playback playback;
				float fadeOutTime = 0.2f;
			};
			Array<ExtraPlayback> extraPlaybacks;

			void clear_internal();

			void find_sounds_to_play(float _deltaTime, ::Framework::Sound::Camera const & _camera);
			void add_sounds_from(float _deltaTime, ScannedRoom & _room, ::Framework::Sound::Camera const & _camera);
			void add_sounds_from(float _deltaTime, SoundSources & _soundSources);

			// this is main worker for adding sounds
			void add_sounds_from(float _deltaTime, SoundSources & _soundSources, ScannedRoom * _room, ::Framework::Sound::Camera const * _camera, bool _ownerOnly); // if _camera is not set, _room and _ownerOnly are ignored

			void update_playback(float _deltaTime, Optional<SceneUpdateContext> const& _context);

			void update_extra_playbacks(Optional<SceneUpdateContext> const& _context);

			PlayingSceneSound* get_available_sound(int _priority, float _distancePt);

		};

	};
};
