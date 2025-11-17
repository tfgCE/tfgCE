#include "moduleSound.h"

#include "..\ai\aiCommon.h"

#include "..\..\framework\module\modules.h"
#include "..\..\framework\module\moduleSoundData.h"
#include "..\..\framework\module\moduleDataImpl.inl"

//

using namespace TeaForGodEmperor;

//

// variables
DEFINE_STATIC_NAME(voiceless);

//

REGISTER_FOR_FAST_CAST(ModuleSound);

static Framework::ModuleSound* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleSound(_owner);
}

Framework::RegisteredModule<Framework::ModuleSound> & ModuleSound::register_itself()
{
	return Framework::Modules::sound.register_module(String(TXT("sound")), create_module);
}

ModuleSound::ModuleSound(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

ModuleSound::~ModuleSound()
{
}

bool ModuleSound::can_play_sound(Framework::ModuleSoundSample const* _sample) const
{
	if (_sample && _sample->tags.does_match_any_from(AI::Common::sounds_talk()))
	{
		if (get_owner()->get_variables().get_value<bool>(NAME(voiceless), false))
		{
			return false;
		}
	}
	return base::can_play_sound(_sample);
}

void ModuleSound::on_play_sound(Framework::SoundSource* _sound)
{
	base::on_play_sound(_sound);
	if (_sound && _sound->get_tags().does_match_any_from(AI::Common::sounds_talk()))
	{
		stop_sounds_except(AI::Common::sounds_talk(), _sound);
	}
}

void ModuleSound::stop_talk_sounds(Optional<float> const & _fadeOut)
{
	stop_sounds(AI::Common::sounds_talk(), _fadeOut);
}

bool ModuleSound::is_playing_talk_sounds() const
{
	return is_playing(AI::Common::sounds_talk());
}
