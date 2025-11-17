//#define SOUND_PLAYBACK_USE_ZERO_LOCATION
//#define SOUND_PLAYBACK_USE_ZERO_VELOCITY

//#define SOUND_NO_DOPPLER

//#define SOUND_NO_REVERBS

//#define SOUND_NO_OCCLUSSION

//#define SOUND_NO_LOW_PASS

// this is disabled as it was adding very low frequency wave immediately without any proper ramping and it was resulting in a sound crackling
#define SOUND_NO_DISTANCE_FILTER

// if this is used, first advancement of sound will be skipped, ie. first frame will be not heard, better to make sure all the sounds start properly faded
//#define SOUND_SKIP_FADE_ON_FIRST_ADVANCEMENT