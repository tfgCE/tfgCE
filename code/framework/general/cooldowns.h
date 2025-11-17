#pragma once

#include "..\..\core\containers\arrayStatic.h"
#include "..\..\core\debug\logInfoContext.h"
#include "..\..\core\types\name.h"
#include "..\..\core\types\optional.h"

namespace Framework
{
	template<unsigned int Count, typename NameElement = Name>
	struct Cooldowns
	{
		static Cooldowns const & empty() { return s_empty; }

		Cooldowns();

		bool is_available(NameElement const & _name) const;
		void advance(float _deltaTime);
		void add(NameElement const & _name, Optional<float> const & _cooldown); // adds, may end up with multiple with same name
		void clear(NameElement const & _name); // removes just one
		void set(NameElement const & _name, Optional<float> const & _cooldown); // sets new or existing
		void set_if_longer(NameElement const & _name, Optional<float> const & _cooldown); // sets new or existing
		Optional<float> get(NameElement const & _name) const;

		bool does_require_advance() const { return ! cooldowns.is_empty(); }

		void log(LogInfoContext & _log) const;

		bool make_sure_theres_place_for_new_one(float _cooldown); // if no place left, removes the lowest one, unless _cooldown is lower

	private:
		static Cooldowns s_empty;

		struct Cooldown
		{
			NameElement name;
			float left = 0.0f;
		};
		ArrayStatic<Cooldown, Count> cooldowns;
	};

	struct CooldownsSmall
	: public Cooldowns<8, Name>
	{
	public:
		static void log(LogInfoContext & _log, CooldownsSmall const & _cooldowns) { _cooldowns.Cooldowns<8, Name>::log(_log); }
	};

	#include "cooldowns.inl"
};

DECLARE_REGISTERED_TYPE(Framework::CooldownsSmall);

#ifdef AN_CLANG
template<>
Framework::Cooldowns<8> Framework::Cooldowns<8>::s_empty;
#endif

