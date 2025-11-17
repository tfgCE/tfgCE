#pragma once

#include "..\..\framework\module\moduleSound.h"

namespace Concurrency
{
	class JobExecutionContext;
};

namespace TeaForGodEmperor
{
	class ModuleSoundData;
	class SoundSource;
	struct ModuleSoundSample;

	class ModuleSound
	: public Framework::ModuleSound
	{
		FAST_CAST_DECLARE(ModuleSound);
		FAST_CAST_BASE(Framework::ModuleSound);
		FAST_CAST_END();

		typedef Framework::ModuleSound base;
	public:
		static Framework::RegisteredModule<Framework::ModuleSound> & register_itself();

		ModuleSound(Framework::IModulesOwner* _owner);
		virtual ~ModuleSound();

		void stop_talk_sounds(Optional<float> const& _fadeOut);
		bool is_playing_talk_sounds() const;

	protected:
		override_ bool can_play_sound(Framework::ModuleSoundSample const* _sample) const;
		override_ void on_play_sound(Framework::SoundSource*);
	};
};

