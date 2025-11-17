#pragma once

#include "module.h"

#include "..\ai\aiMessageTemplate.h"
#include "..\sound\soundSources.h"

namespace Concurrency
{
	class JobExecutionContext;
};

namespace Framework
{
	class ModulePresence;
	class ModuleSoundData;
	class SoundSource;
	struct ModuleSoundSample;

	class ModuleSound
	: public Module
	{
		FAST_CAST_DECLARE(ModuleSound);
		FAST_CAST_BASE(Module);
		FAST_CAST_END();

		typedef Module base;
	public:
		static RegisteredModule<ModuleSound> & register_itself();

		ModuleSound(IModulesOwner* _owner);
		virtual ~ModuleSound();

		SoundSources const & get_sounds() const { return sounds; }
		SoundSources & access_sounds() { return sounds; }

		bool does_handle_sound(Name const & _name) const; // even if silent!
		SoundSource* play_sound(Sample* _sample, Optional<Name> const & _socket = NP, Transform const & _relativePlacement = Transform::identity, SoundPlayParams const & _params = NP, bool _playDetached = false);
		SoundSource* play_sound(Name const & _name, Optional<Name> const & _socket = NP, Transform const & _relativePlacement = Transform::identity, SoundPlayParams const & _params = NP);
		SoundSource* play_sound_using(Name const & _name, ModuleSoundData const * _usingData, Optional<Name> const & _socket = NP, Transform const & _relativePlacement = Transform::identity, SoundPlayParams const & _params = NP);
		SoundSource* play_sound_in_room(Name const & _name, Optional<Name> const & _socket = NP, Transform const & _relativePlacement = Transform::identity, SoundPlayParams const & _params = NP);
		SoundSource* play_sound_in_room(Name const & _name, Room const * _room, Transform const & _placement, SoundPlayParams const & _params = NP);
		void stop_sound(Sample* _sample, Optional<float> const & _fadeOut = NP);
		void stop_sound(Name const & _name, Optional<float> const & _fadeOut = NP);
		void stop_looped_sounds(Optional<float> const & _fadeOut = NP);
		void stop_sounds(Tags const & _tags, Optional<float> const & _fadeOut = NP);
		void stop_sounds_except(Tags const & _tags, SoundSource* _exceptSound, Optional<float> const & _fadeOut = NP);

		bool is_playing(Name const& _name) const;
		bool is_playing(Tags const& _tags) const;

	public:
		void clear_play_params_overrides();
		void clear_play_params_overrides(Name const & _sound);
		void clear_play_params_overrides(Framework::LibraryName const & _sample);
		void set_play_params_overrides(Name const & _sound, SoundPlayParams const & _params);
		void set_play_params_overrides(Framework::LibraryName const & _sample, SoundPlayParams const & _params);

	public:	// Module
		override_ void reset();
		override_ void setup_with(ModuleData const * _moduleData);
		override_ void initialise();
		override_ void activate();

	private: friend class ModulePresence;
		void play_auto_play();

	protected:
		virtual bool can_play_sound(ModuleSoundSample const *) const { return true; }
		virtual void on_play_sound(SoundSource*) {}

	private: friend interface_class IModulesOwner;
		void advance_sounds(float _deltaTime);
		AVOID_CALLING_ bool does_require_advance_sounds() const;

	private:
		ModuleSoundData const * moduleSoundData = nullptr;
		
		Random::Generator rg;

		bool autoPlayPlayed = false;

		Optional<float> generalSoundVolume;

		SoundSources sounds;
		Cooldowns<32> soundCooldowns;
		AI::MessageTemplateCooldowns aiMessageCooldowns;

		struct SoundPlayParamsOverride 
		{
			Framework::LibraryName sample;
			Name sound;
			SoundPlayParams params;
		};
		Array<SoundPlayParamsOverride> playParamsOverrides;

		SoundSource* just_play_sound(Name const & _name, ModuleSoundData const* _usingData, OUT_ ModuleSoundSample const * & _soundSample, SoundPlayParams const & _params = NP);
		void on_attach_placed_sound(ModuleSoundData const* _usingData, SoundSource*, ModuleSoundSample const * soundSample, SoundPlayParams const & _params = NP);

		bool should_sample_be_heard(Sample const * _soundSample, SoundPlayParams const& _params) const;
	};
};

