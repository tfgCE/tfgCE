#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\memory\refCountObjectPtr.h"
#include "..\..\core\sound\playback.h"

namespace Framework
{
	class Sample;
};

namespace TeaForGodEmperor
{
	namespace OverlayInfo
	{
		class System;
		struct Element;
	};

	typedef int SubtitleId;

	class SubtitleSystem
	{
	public:
		static SubtitleSystem* get() { return s_system; }
		static bool withSubtitles;
		static float subtitlesScale;
		static void set_distance_for_subtitles(Optional<float> const& _distanceForSubtitles = NP);
		static void set_background_for_subtitles(Optional<Colour> const& _backgroundForSubtitles = NP);
		static void set_offset_for_subtitiles(Optional<Rotator3> const& _offsetForSubtitles = NP);

		SubtitleSystem();
		~SubtitleSystem();

		void use(OverlayInfo::System* _overlayInfoSystem) { overlayInfoSystem = _overlayInfoSystem; }

		void update(float _deltaTime);

		SubtitleId get(String const & _subtitle) const;
		SubtitleId get(Name const & _subtitleLS) const;
		SubtitleId add(String const & _subtitle, Optional<SubtitleId> const & _continuationOfId = NP, Optional<bool> const & _forced = NP, Optional<Colour> const & _colour = NP, Optional<Colour> const & _backgroundColour = NP);
		SubtitleId add(Name const & _subtitleLS, Optional<SubtitleId> const & _continuationOfId = NP, Optional<bool> const & _forced = NP, Optional<Colour> const & _colour = NP, Optional<Colour> const & _backgroundColour = NP);
		void remove(SubtitleId const& _id, float _delay = 0.0f);
		void remove_if_removed(SubtitleId const& _id, SubtitleId const& _ifRemovedId);
		void remove_all();

		void add_user_log(String const& _line, bool _continuation = false);
		void clear_user_log();
		void trim_user_log(int _length);

		void for_user_log(int _lines, std::function<void(String const&)> _do);
		int get_user_log_ver() const { return userLogVer; }

	public:
		void log(LogInfoContext& _log) const;

	private:
		static SubtitleSystem* s_system;
		
	private:
		// working data
		mutable Concurrency::SpinLock accessLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.SubtitleSystem.accessLock"));
		
		OverlayInfo::System* overlayInfoSystem = nullptr;

		Optional<float> distanceForSubtitles;
		Optional<Colour> backgroundForSubtitles;
		Optional<Rotator3> offsetForSubtitles;

		SubtitleId freeId = 0;

		int lineOffset = 0; // to show consequtive lines below each other
		SubtitleId lineOffsetForId = NONE;

		struct Subtitle
		{
			SubtitleId id = NONE;
			SubtitleId continuationOfId = NONE;
			SubtitleId removeIfRemovedId = NONE;
			bool forced = false;
			Optional<Colour> colour;
			Optional<Colour> backgroundColour;
			Name subtitleLS;
			String subtitle;
			RefCountObjectPtr<OverlayInfo::Element> overlayElement;
			bool lineOffsetRead = false;
			float timeToRemove = 0.0f; // only if positive
		};

		Array<Subtitle> subtitles;

		List<String> userLog;
		int userLogVer = 0;

		float timeSinceUserLogActive = 0.0f;
	};


};
