#pragma once

#include "musicDefinition.h"
#include "musicTrack.h"

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\sound\playback.h"

#include "..\..\framework\general\cooldowns.h"

namespace Framework
{
	class Room;
	class Sample;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class PilgrimageInstance;

	/**
	 *	Accidental are sound effects etc that build up to musical soundscape
	 *	Incidental are all explicitly requested music tracks to be played
	 */
	class MusicPlayer
	{
	public:
		static MusicPlayer* get() { return s_player; }

		MusicPlayer(bool _additional = false); // if additional, won't be accessible through "get"
		~MusicPlayer();

		void set_dull(float _dull = 0.0f, float _dullBlendTime = 0.0f);

		void update(float _deltaTime);
		void update(Framework::IModulesOwner* _imo, float _deltaTime);

		void reset();

		void pause();
		void resume();
		
		void play(Framework::Sample* _music, Optional<float> const & _fadeTime = NP);
		void stop(float _fadeOut = 0.0f); // stop, player has died, etc
		void fade_out(float _fadeOut = 2.0f); // allows bumpers to play till the end

	public:
		// requests
		static void block_requests(bool _block = true);
		static void request_none();
		static void request_ambient();
		static void request_incidental(Optional<Name> const & _incidentalMusicTrack = NP, Optional<bool> const & _incidentalAsOverlay = NP); // use with music track, otherwise will try to read from the room, may fail
		static void request_music_track(Optional<Name> const& _musicTrack = NP);
		static void request_combat_auto(Optional<float> const& _highIntensityTime = NP, Optional<float> const& _lowIntensityTime = NP, Optional<float> const& _delay = NP);
		static void request_combat_auto_indicate_presence(Optional<float> const& _lowIntensityTime = NP, Optional<float> const& _delay = NP);
		static void request_combat_auto_bump_high(Optional<float> const& _highIntensityTime = NP);
		static void request_combat_auto_bump_low(Optional<float> const& _lowIntensityTime = NP);
		static void request_combat_auto_stop();
		static void request_combat_auto_calm_down();

		static bool is_playing_combat();
		static Name const & get_current_track();

	public:
		void log(LogInfoContext& _log, Optional<Colour> const& _defaultColour = NP) const;

	private:
		static MusicPlayer* s_player;
		
	private:
		// requests
		Concurrency::SpinLock requestLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.MusicPlayer.requestLock"));
		
		bool requestsBlocked = false;

		struct AutoCombat
		{
			float highIntensityTimeLeft = 0.0f;
			float lowIntensityTimeLeft = 0.0f;
			float delay = 0.0f;
		} autoCombat;

		MusicAutoCombatDefinition autoCombatDefinition; // overriden by what MusicDefinition gives us

		MusicTrack::Type requestedTrackType = MusicTrack::Ambient;
		Optional<Name> requestedMusicTrack; // will override what's to be found in room
		Optional<Name> requestedIncidentalMusicTrack; // only for incidental
		bool requestedIncidentalOverlay = false; // play as overlay
		int requestedIncidentalMusicTrackIdx = 0;
		int currentIncidentalMusicTrackIdx = 0;

		float dull = 0.0f;
		float dullBlendTime = 0.0f;
		float dullTarget = 0.0f;

	private:
		mutable Concurrency::SpinLock accessLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.MusicPlayer.accessLock"));

		// working data
		CACHED_ Framework::Room const* checkedForRoom = nullptr;
		CACHED_ Optional<Name> trackForCheckedRoom;

		bool isPaused = false;
		
		float trackLockedFor = 0.0f;
		Name currentTrackId;
		MusicTrack const* currentTrack = nullptr;
		MusicDefinition const* currentMusicDefinition = nullptr; // for reference only
		MusicTrack::Type currentTrackType = MusicTrack::Ambient;
		Optional<MusicTrack::Type> currentOverlayTrackType;
		bool currentHighIntensity = false;
		Optional<float> fadeOutTimeForNonLooped;

		struct ChangeTrackPlayback
		{
			Framework::Sample const* requestedSample = nullptr;
			float fadeTime = 0.5f;
			Optional<float> fadeInTime; // for fading out, fadeTime is used
			Optional<Range> startAt;
			Optional<Range> startPt;
			Optional<float> fadeOut;
			Optional<float> fadeOutTimeForNonLooped;
			Optional<int> syncDir;
			bool queue = false;
			bool syncPt = false;
			float beatSync = 0.0f;
			Framework::Sample const* playBumperSample = nullptr;
		};
		ChangeTrackPlayback queuedTrackPlayback;
		Sound::Playback currentTrackPlayback;
		Optional<float> currentTrackPlaybackAt;
		List<Sound::Playback> previousPlaybacks; // to pause/resume
		Framework::Sample const* currentSamplePlayed = nullptr; // as a reference, whether we need to change or we're good with what is played
		Sound::Playback currentTrackPlaybackOverlay; // overlay for high intensity
		Framework::Sample const* currentSamplePlayedOverlay = nullptr; // as a reference, whether we need to change or we're good with what is played
		Sound::Playback currentTrackPlaybackIncidentalOverlay; // overlay for incidental (non looped!)
		Framework::Sample const* currentSamplePlayedIncidentalOverlay = nullptr; // as a reference, whether we need to change or we're good with what is played

		Framework::Cooldowns<32, Framework::LibraryName> bumperCooldowns;
		List<Sound::Playback> bumperPlaybacks;
#ifdef AN_DEVELOPMENT
		Framework::Sample const* currentBumperSamplePlayed = nullptr; // info
#endif

		List<Sound::Playback> decoupledPlaybacks; // keep updating to let them die

		float accidentalTimeLeft = 0.0f;
		Framework::Cooldowns<32, Name> accidentalCooldowns;
		List<Sound::Playback> accidentalPlaybacks;

		void decouple_all_and_clear();

		void handle_requested_sample(MusicDefinition const* md, Framework::Sample const* requestedSample);
		void handle_requested_sample_overlay(MusicDefinition const* md, Framework::Sample const* requestedSampleOverlay);
		void handle_requested_sample_incidental_overlay(MusicDefinition const* md, Framework::Sample const* requestedSampleIncidentalOverlay);

		Optional<float> calculate_sync_pt_for_beat_sync(Framework::Sample const* requestedSample, float beatSync, Optional<int> const & _syncDir) const;

		void change_current_track(ChangeTrackPlayback const& cpt, tchar const* _info, REF_ Framework::Sample const*& _currentSamplePlayed, REF_ Sound::Playback& _currentTrackPlayback);
		void fill_change_playback_track(REF_ ChangeTrackPlayback& cpt, MusicDefinition const* md, Framework::Sample const* requestedSample);

		PilgrimageInstance* get_pilgrimage_instance(Framework::Room* inRoom);
	};


};
