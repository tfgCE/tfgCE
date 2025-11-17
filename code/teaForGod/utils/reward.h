#pragma once

#include "..\game\energy.h"

//

namespace TeaForGodEmperor
{
	struct Reward
	{
		Energy xp = Energy::zero();
		int meritPoints = 0;

		Reward() {}
		Reward(Energy const& _xp, int _meritPoints) : xp(_xp), meritPoints(_meritPoints) {}

		static Reward xp_only(Energy const & _xp) { return Reward(_xp, 0); }
		static Reward merit_points_only(int _mp) { return Reward(Energy::zero(), _mp); }

		Reward no_xp() const { return merit_points_only(meritPoints); }
		Reward no_merit_points() const { return xp_only(xp); }

		bool is_empty() const { return xp.is_zero() && meritPoints == 0; }
		
		bool has_xp() const { return ! xp.is_zero(); }
		bool has_merit_points() const { return meritPoints != 0; }

		String as_string_decimals(int _xpDecimals = NONE) const;
		String as_string(bool _forceXP = false, bool _forceMeritPoints = false) const;
		String as_string_no_xp_char() const;
	};

};
