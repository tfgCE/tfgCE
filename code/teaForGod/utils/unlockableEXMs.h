#pragma once

#include "..\game\persistence.h"

namespace TeaForGodEmperor
{
	struct UnlockableEXMs
	{
		Array<Name> availableEXMs;
		Array<Name> unlockedEXMs;

		void update(bool _simpleRules, Optional<bool> const & _nonPermanentFromLastSetup = NP);

		static int count_unlocked(Name const& _exm);
	};
};

