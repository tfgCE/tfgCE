#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\sound\playback.h"

#include "..\..\framework\sound\soundSampleTextCache.h"

#include "subtitleSystem.h"

namespace Framework
{
	class Sample;
};

namespace TeaForGodEmperor
{
	class SubtitleSystem;

	class VoiceoverSystem
	{
	public:
		static VoiceoverSystem* get() { return s_system; }

		VoiceoverSystem(bool _additional = false); // if additional, won't be accessible through "get"
		~VoiceoverSystem();

		void use(SubtitleSystem* _subtitleSystem) { subtitleSystem = _subtitleSystem; }

		void update(float _deltaTime);

		void reset();

		void pause();
		void resume();
		
		bool is_speaking(Name const& _actor, Framework::Sample* _line = nullptr) const;
		bool is_speaking(Name const& _actor, String const & _lineText) const;
		float get_time_to_end_speaking(Name const& _actor, Framework::Sample* _line = nullptr) const;
		bool is_anyone_speaking() const;
		bool is_any_subtitle_active() const;

		void speak(Name const& _actor, Optional<int> const & _audioDuck, Framework::Sample* _line, bool _autoSubtitles = true, Optional<float> const & _volume = NP);
		void speak(Name const& _actor, Optional<int> const & _audioDuck, String const & _line, Optional<float> const& _volume = NP);
		void silence(Name const & _actor, Optional<float> const & _fadeOut = NP); // only particular one
		void silence(Optional<float> const & _fadeOut = NP, Optional<Name> const & _exceptOfActor = NP); // all
		void silence(Optional<float> const & _fadeOut, Array<Name> const & _exceptOfActors);

	public:
		void log(LogInfoContext& _log) const;

	private:
		static VoiceoverSystem* s_system;
		
		SubtitleSystem* subtitleSystem = nullptr;

	private:
		// working data
		mutable Concurrency::SpinLock accessLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.VoiceoverSystem.accessLock"));
		
		bool isPaused = false;
		int audioDuckCurrent = 0;

		struct Playback
		{
			Framework::Sample* line;
			String lineText;
			Framework::SampleTextCache lineTextCache;
			String subtitleLine;
			bool clearSubtitles = false;
			float at = 0.0f; // if no line
			float timeLeft = 0.0f; // if no line
			Sound::Playback playback;
			SubtitleId subtitleId = NONE;
			int audioDuck = 0;
			bool autoSubtitles = true;
		};
		struct Actor
		{
			Name actor;
			List<Playback> playbacks; // to pause/resume/fade out/log
		};

		List<Actor> actors;

		void speak(Name const& _actor, Framework::Sample* _line, String const & _lineText, int _audioDuck, bool _autoSubtitles, Optional<float> const& _volume = NP);

		void silence(Playback & _p, Optional<float> const& _fadeOut);
		void remove_subtitle(Playback & _p, float _delay = 0.0f);
		void remove_subtitle(Playback& _p, SubtitleId const& _ifRemovedId);

		void update_audio_duck();
	};


};
