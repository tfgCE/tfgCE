#include "soundPlayParams.h"

using namespace Framework;

bool SoundPlayParams::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	volume.load_from_xml(_node, TXT("volume"));
	pitch.load_from_xml(_node, TXT("pitch"));
	fadeIn.load_from_xml(_node, TXT("fadeIn"));
	fadeOut.load_from_xml(_node, TXT("fadeOut"));
	earlyFadeOut.load_from_xml(_node, TXT("earlyFadeOut"));
	pitchRelatedToFadeBase.load_from_xml(_node, TXT("pitchRelatedToFadeBase"));
	hearingDistance.load_from_xml(_node, TXT("hearingDistance"));
	if (_node->has_attribute(TXT("distanceBeyondFirstRoom")))
	{
		warn_loading_xml(_node, TXT("\"distanceBeyondFirstRoom\" is deprecated, use \"hearingDistanceBeyondFirstRoom\""));
	}
	distances.load_from_xml(_node, TXT("distances"));
	hearingDistanceBeyondFirstRoom.load_from_xml(_node, TXT("hearingDistanceBeyondFirstRoom"));
	hearingRoomDistanceRecentlySeenByPlayer.load_from_xml(_node, TXT("hearingRoomDistanceRecentlySeenByPlayer"));
	hearableOnlyByOwner.load_from_xml(_node, TXT("hearableOnlyByOwner"));

	return result;
}

void SoundPlayParams::fill_with(SoundPlayParams const & _other)
{
	volume.set_if_not_set(_other.volume);
	pitch.set_if_not_set(_other.pitch);
	fadeIn.set_if_not_set(_other.fadeIn);
	fadeOut.set_if_not_set(_other.fadeOut);
	earlyFadeOut.set_if_not_set(_other.earlyFadeOut);
	pitchRelatedToFadeBase.set_if_not_set(_other.pitchRelatedToFadeBase);
	distances.set_if_not_set(_other.distances);
	hearingDistance.set_if_not_set(_other.hearingDistance);
	hearingDistanceBeyondFirstRoom.set_if_not_set(_other.hearingDistanceBeyondFirstRoom);
	hearingRoomDistanceRecentlySeenByPlayer.set_if_not_set(_other.hearingRoomDistanceRecentlySeenByPlayer);
	hearableOnlyByOwner.set_if_not_set(_other.hearableOnlyByOwner);
}
