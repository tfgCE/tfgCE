#pragma once

#include "..\..\core\containers\arrayStatic.h"
#include "..\..\core\debug\logInfoContext.h"
#include "..\..\core\types\name.h"
#include "..\..\core\types\optional.h"
#include "..\..\core\system\timeStamp.h"

namespace Framework
{
	// work similar to Cooldowns but doesn't have to be advanced and is not affected by pausing/slowing down, useful for visuals

	template<unsigned int Count, typename NameElement = Name>
	struct CooldownsTimeStampBased
	{
		static CooldownsTimeStampBased const & empty() { return s_empty; }

		CooldownsTimeStampBased();

		bool is_available(NameElement const & _name) const;
		void add(NameElement const & _name, Optional<float> const & _cooldown); // adds, may end up with multiple with same name
		void clear(NameElement const & _name); // removes just one
		void set(NameElement const & _name, Optional<float> const & _cooldown); // sets new or existing
		void set_if_longer(NameElement const & _name, Optional<float> const & _cooldown); // sets new or existing
		Optional<float> get(NameElement const & _name) const;

		void log(LogInfoContext & _log) const;

		bool make_sure_theres_place_for_new_one(float _cooldown); // if no place left, removes the lowest one, unless _cooldown is lower

	private:
		static CooldownsTimeStampBased s_empty;

		struct Cooldown
		{
			NameElement name;
			System::TimeStamp freeAtTS;
		};
		ArrayStatic<Cooldown, Count> cooldowns;

		void clean_up();
	};

	#include "cooldownsTimeStampBased.inl"
};
