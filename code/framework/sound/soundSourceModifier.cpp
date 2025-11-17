#include "soundSourceModifier.h"

#include "soundSource.h"

using namespace Framework;

SoundSourceModifier::SoundSourceModifier(SoundSource* _soundSource)
: soundSource(nullptr)
{
	an_assert(_soundSource);
	_soundSource->add_modifier(this);
}

SoundSourceModifier::~SoundSourceModifier()
{
	if (soundSource)
	{
		soundSource->remove_modifier(this);
	}
}

void SoundSourceModifier::added_to(SoundSource* _soundSource)
{
	an_assert(! soundSource);
	soundSource = _soundSource;
}

void SoundSourceModifier::removed_from(SoundSource* _soundSource)
{
	an_assert(soundSource == _soundSource);
	soundSource = nullptr;
}
