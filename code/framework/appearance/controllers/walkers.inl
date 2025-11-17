float GaitLegMovement::get_time_left(GaitPlayback const & _playback) const
{
	return max(0.0f, endsAt - _playback.get_at_time()) / _playback.get_playback_speed();
}

float GaitLegMovement::get_time_left_to_end_placement(GaitPlayback const & _playback) const
{
	an_assert(endPlacementAt >= endsAt);
	return max(0.0f, endPlacementAt - _playback.get_at_time()) / _playback.get_playback_speed();
}
