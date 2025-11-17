#include "ssm_glitchStutter.h"

#include "..\..\core\random\random.h"
#include "..\..\framework\sound\soundSource.h"

using namespace TeaForGodEmperor;
using namespace SoundSourceModifiers;

GlitchStutter::GlitchStutter(Framework::SoundSource* _soundSource)
: base(_soundSource)
{
}

void GlitchStutter::advance(float _deltaTime)
{
	if (glitch.is_set())
	{
		bool keep = false;
		if (glitch.get() == Type::Repeat)
		{
			if (repeatsLeft > 0)
			{
				keep = true;
				if (repeatTimeLeft > 0.0f)
				{
					repeatTimeLeft -= _deltaTime;
				}
				else
				{
					repeatTimeLeft = repeatDuration;
					access_sound_source().be_at(repeatAt);
					--repeatsLeft;
					if (repeatsLeft <= 0)
					{
						keep = false;
					}
				}
			}
		}
		if (!keep)
		{
			glitch.clear();
			timeToNextGlitch = Random::get_float(0.2f, 0.9f);
		}
	}
	else
	{
		timeToNextGlitch -= _deltaTime;

		if (timeToNextGlitch < 0.0f)
		{
			if (Random::get_chance(0.8f))
			{
				glitch = Type::Repeat;
				repeatAt = access_sound_source().get_at();
				repeatDuration = max(0.0f, Random::get_float(-0.15f, 0.15f));
				repeatsLeft = repeatDuration > 0.03f ? Random::get_int_from_range(2, 5) : Random::get_int_from_range(4, 12);
				repeatTimeLeft = repeatDuration;
			}
			timeToNextGlitch = Random::get_float(0.2f, 0.9f);
		}
	}

}