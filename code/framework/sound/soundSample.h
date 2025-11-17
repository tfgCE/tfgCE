#pragma once

#include "..\..\core\io\xml.h"
#include "..\..\core\sound\playback.h"
#include "..\..\core\sound\sample.h"

#include "..\library\libraryStored.h"

#include "..\text\localisedString.h"

#include "soundSampleTextCache.h"

namespace Random
{
	class Generator;
};

namespace Framework
{
	struct SampleImporter;

	/**
	 *	Sample as wrap for multiple samples.
	 *	There's also info how to play sample, when to stop, how to fade in, fade out.
	 */
	class Sample
	: public LibraryStored
	{
		FAST_CAST_DECLARE(Sample);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		static bool load_multiple_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);

		Sample(Library * _library, LibraryName const & _name);

		::Sound::Sample * get_sample(int _index = 0) { return samples.is_index_valid(_index)? samples[_index] : nullptr; }
		::Sound::Sample const * get_sample(int _index = 0) const { return samples.is_index_valid(_index) ? samples[_index] : nullptr; }

		::Sound::Playback play(Optional<float> const & _fadeIn = NP, Optional<::Sound::PlaybackSetup> const & _playbackSetup = NP, Optional<int> const& _sampleIdx = NP, bool _keepPaused = false) const;

		int get_sample_num() const { return samples.get_size(); }

		bool is_hearable_only_by_owner() const { return hearableByOwnerOnly; }
		float get_hearing_distance() const { return hearingDistance; }
		Optional<float> const & get_hearing_distance_beyond_first_room() const { return hearingDistanceBeyondFirstRoom; }
		Optional<int> const & get_hearing_room_distance_recently_seen_by_player() const { return hearingRoomDistanceRecentlySeenByPlayer; }
		int get_max_concurrent_apparent_sound_sources() const { return maxConcurrentApparentSoundSources; }

		float get_fade_in_time() const { return fadeInTime; }
		float get_fade_out_time() const { return fadeOutTime; }
		float get_early_fade_out_time() const { return earlyFadeOutTime; }

		float get_start_at_pt(Random::Generator* _useRandomGenerator = nullptr) const;

		String const& get_text(Optional<float> const& _at = NP) const;

#ifdef AN_PERFORMANCE_MEASURE
	public:
		String const & get_performance_context_info() const { return performanceContextInfo; }
#endif

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ void load_outside_queue(tchar const* _reason);
		override_ void unload_outside_queue(tchar const* _reason);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	protected:
		~Sample();

	protected:
#ifdef AN_PERFORMANCE_MEASURE
		String performanceContextInfo;
#endif
		RefCountObjectPtr<SampleImporter> onMultipleLoadOutsideQueueSampleImporter; // if we're loading multiple, we want only to import on multiple load outside queue

		Array<::Sound::Sample*> samples;
		LocalisedString text; // text has to be explicitly defined along the sample, we no longer allow adding a custom text somewhere else and linking the two
		SampleTextCache textCache;
		bool hearableByOwnerOnly = false;
		float hearingDistance = 0; // 0 - no limit, above this distance sample is not heard at all
		Optional<float> hearingDistanceBeyondFirstRoom;
		Optional<int> hearingRoomDistanceRecentlySeenByPlayer; // used only when starting to play, do not use it with looped sounds that go longer than a second
		int maxConcurrentApparentSoundSources = 1; // 0 - no limit
		float fadeInTime = 0.0f;
		float fadeOutTime = 0.0f; // to fade out on stop
		float earlyFadeOutTime = 0.0f; // to fade out on sound's end
		Range startAtPt = Range::zero;

		friend struct SampleImporter;

		bool load_params_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
	};
};
