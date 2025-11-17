#include "soundSampleInstance.h"

#include "soundSample.h"

#include "..\..\core\random\random.h"

using namespace Framework;

void SampleInstance::set(Sample const * _sample, Random::Generator * _randomGenerator)
{
	if (_sample)
	{
		Random::Generator rgFallback;
		Random::Generator& rg = _randomGenerator ? *_randomGenerator : rgFallback;
		int samplesNum = _sample->get_sample_num();
		if (samplesNum == 0)
		{
			clear();
		}
		else if (samplesNum == 1)
		{
			soundSampleIndex = 0;
			soundSample = _sample->get_sample(soundSampleIndex);
			sample = _sample;
		}
		else
		{
			soundSampleIndex = rg.get_int(samplesNum);
			soundSample = _sample->get_sample(soundSampleIndex);
			sample = _sample;
		}

		sampleVolume = sample && sample->get_sample() ? rg.get(sample->get_sample()->get_volume()) : 1.0f;
		samplePitch = sample && sample->get_sample() ? rg.get(sample->get_sample()->get_pitch()) : 1.0f;
	}
	else
	{
		clear();
	}
}

void SampleInstance::clear()
{
	soundSampleIndex = 0;
	soundSample = nullptr;
	sample = nullptr;
}
