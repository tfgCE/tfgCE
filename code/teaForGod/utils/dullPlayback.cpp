#include "dullPlayback.h"

#include "..\..\core\mainConfig.h"
#include "..\..\core\sound\playback.h"

//

//

using namespace TeaForGodEmperor;

//

DullPlayback::DullPlayback(float _dull)
{
	dull = _dull;
	lowPassGain = max(0.1f, 1.0f - 1.3f * dull); // force higher dull value
	useLowPass = MainConfig::global().get_sound().enableOcclusionLowPass;
}

void DullPlayback::handle(Sound::Playback& _playback)
{
	if (useLowPass)
	{
		_playback.set_low_pass_gain(lowPassGain);
	}
}
