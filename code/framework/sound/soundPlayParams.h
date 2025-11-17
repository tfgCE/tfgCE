#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\types\optional.h"

namespace Framework
{
	struct LibraryLoadingContext;

	struct SoundPlayParams
	{
	public:
		Optional<Range> volume;
		Optional<Range> pitch;
		Optional<float> fadeIn;
		Optional<float> fadeOut;
		Optional<float> earlyFadeOut;
		Optional<float> pitchRelatedToFadeBase;
		Optional<Range> distances;
		Optional<float> hearingDistance;
		Optional<float> hearingDistanceBeyondFirstRoom;
		Optional<int> hearingRoomDistanceRecentlySeenByPlayer;
		Optional<bool> hearableOnlyByOwner;

	public:
		SoundPlayParams() {}
		SoundPlayParams(NotProvided const & _np) {}

		void fill_with(SoundPlayParams const & _other);

		SoundPlayParams& with_fade_in(float _fadeIn) { fadeIn = _fadeIn; return *this; }
		SoundPlayParams& with_fade_out(float _fadeOut) { fadeOut = _fadeOut; return *this; }
		SoundPlayParams& with_early_fade_out(float _earlyFadeOut) { earlyFadeOut = _earlyFadeOut; return *this; }

	public:
		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
	};
}