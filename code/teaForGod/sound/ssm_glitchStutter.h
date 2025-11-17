#pragma once

#include "..\..\core\types\optional.h"
#include "..\..\framework\sound\soundSourceModifier.h"

namespace TeaForGodEmperor
{
	namespace SoundSourceModifiers
	{
		class GlitchStutter
		: public Framework::SoundSourceModifier
		{
			typedef Framework::SoundSourceModifier base;

		public:
			GlitchStutter(Framework::SoundSource* _soundSource);

		public: // SoundSourceModifier
			override_ void advance(float _deltaTime);

		private:

			enum Type
			{
				Repeat
			};

			float timeToNextGlitch = 0.0f;
			Optional<Type> glitch;

			int repeatsLeft;
			float repeatDuration;
			float repeatAt;
			float repeatTimeLeft;

		};
	}
};
