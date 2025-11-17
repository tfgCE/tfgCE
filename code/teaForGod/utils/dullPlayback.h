#pragma once

namespace Sound
{
	struct Playback;
};

namespace TeaForGodEmperor
{
	struct DullPlayback
	{
		DullPlayback(float _dull);

		void handle(Sound::Playback& _playback);

	private:
		float dull;
		float lowPassGain;
		bool useLowPass;
	};

};
