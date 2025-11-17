#include "channel.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Sound;

//

#ifdef AN_FMOD
void LoudnessMeter::decouple()
{
	meterDSP = nullptr;
}

void LoudnessMeter::create_and_add_to(FMOD::System* _lowLevelSystem, FMOD::ChannelControl* _channel)
{
	an_assert(!meterDSP);
	meterDSP = nullptr;
	if (_lowLevelSystem->createDSPByType(FMOD_DSP_TYPE_LOUDNESS_METER, &meterDSP) == FMOD_OK)
	{
		if (_channel->addDSP(FMOD_CHANNELCONTROL_DSP_HEAD, meterDSP) == FMOD_OK)
		{
			if (meterDSP->setActive(true) == FMOD_OK)
			{
				// ok
			}
			else
			{
				error(TXT("error activating DSP loudness meter"));
			}
		}
		else
		{
			error(TXT("error attaching DSP loudness meter"));
		}
	}
	else
	{
		error(TXT("could not create DSP loudness meter"));
	}
}
#endif

void LoudnessMeter::update_recent_max_loudness() const
{
	float loudness = get_loudness();
	if (recentMaxLoudnessTS.get_time_since() > 1.0f)
	{
		recentMaxLoudness = -80.0f;
	}
	if (loudness > recentMaxLoudness)
	{
		recentMaxLoudness = loudness;
		recentMaxLoudnessTS.reset();
	}
}

float LoudnessMeter::get_loudness() const
{
	float result = -90.0f;
#ifdef AN_FMOD
	if (meterDSP)
	{
		FMOD_DSP_LOUDNESS_METER_INFO_TYPE loudnessMeterInfoValue;
		void *loudnessMeterInfoPtr = &loudnessMeterInfoValue;
		uint len = sizeof(FMOD_DSP_LOUDNESS_METER_INFO_TYPE);
		if (meterDSP->getParameterData(FMOD_DSP_LOUDNESS_METER_INFO, &loudnessMeterInfoPtr, &len, nullptr, 0) != FMOD_OK) { error(TXT("could not set dsp param")); }
		FMOD_DSP_LOUDNESS_METER_INFO_TYPE* loudnessMeterInfo = plain_cast<FMOD_DSP_LOUDNESS_METER_INFO_TYPE>(loudnessMeterInfoPtr);
		float loudnessDB = loudnessMeterInfo->momentaryloudness;
		result = loudnessDB;
	}

#endif
	return result;
}

Optional<Colour> LoudnessMeter::loudness_to_colour(float _loudness)
{
	if (_loudness >= -90.0f)
	{
		if (_loudness >= -10.0f)
		{
			return Colour::magenta;
		}
		else if (_loudness >= -12.0f)
		{
			return Colour::red;
		}
		else if (_loudness >= -18.0f)
		{
			return Colour::yellow;
		}
		else if (_loudness <= -60.0f)
		{
			return Colour::blue;
		}
		else if (_loudness <= -36.0f)
		{
			return Colour::cyan;
		}
		else
		{
			return Colour::green;
		}
	}
	return NP;
}

//--


float Channel::calculate_final_volume() const
{
	float finalVolume = usedSetVolume * fadeVolume * audioDuckVolume;
	for_every(cv, customVolumes)
	{
		finalVolume *= cv->volume;
	}
	return finalVolume;
}
