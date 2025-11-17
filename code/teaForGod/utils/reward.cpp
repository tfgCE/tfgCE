#include "reward.h"

#include "..\..\framework\text\localisedString.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// localised strings
DEFINE_STATIC_NAME_STR(lsCharsExperience, TXT("chars; experience"));
DEFINE_STATIC_NAME_STR(lsCharsMeritPoints, TXT("chars; merit points"));

//

using namespace TeaForGodEmperor;

//

String Reward::as_string_decimals(int _xpDecimals) const
{
	String result;

	if (!xp.is_zero())
	{
		if (!result.is_empty()) result += TXT(" ");
		result += xp.as_string(_xpDecimals) + LOC_STR(NAME(lsCharsExperience));
	}
	if (meritPoints != 0)
	{
		if (!result.is_empty()) result += TXT(" ");
		result += String::printf(TXT("%i"), meritPoints) + LOC_STR(NAME(lsCharsMeritPoints));
	}

	return result;
}

String Reward::as_string(bool _forceXP, bool _forceMeritPoints) const
{
	String result;

	if (_forceXP || !xp.is_zero())
	{
		if (!result.is_empty()) result += TXT(" ");
		result += xp.as_string_auto_decimals() + LOC_STR(NAME(lsCharsExperience));
	}
	if (_forceMeritPoints || meritPoints != 0)
	{
		if (!result.is_empty()) result += TXT(" ");
		result += String::printf(TXT("%i"), meritPoints) + LOC_STR(NAME(lsCharsMeritPoints));
	}

	return result;
}

String Reward::as_string_no_xp_char() const
{
	String result;

	if (!xp.is_zero())
	{
		if (!result.is_empty()) result += TXT(" ");
		result += xp.as_string_auto_decimals();
	}
	if (meritPoints != 0)
	{
		if (!result.is_empty()) result += TXT(" ");
		result += String::printf(TXT("%i"), meritPoints) + LOC_STR(NAME(lsCharsMeritPoints));
	}

	return result;
}
