#include "option.h"

#include "..\debug\debug.h"
#include "..\types\string.h"

#define PARSE_OPTION(OPT) if (String::compare_icase(_value, to_char(OPT)) && (_allowedFlags & OPT)) return OPT;
#define OPTION(OPT, LOWER) if (_option == OPT) return LOWER;

Option::Type Option::parse(tchar const * _value, Option::Type _default, int _allowedFlags, OPTIONAL_ OUT_ bool* result)
{
	PARSE_OPTION(On);
	PARSE_OPTION(Off);
	PARSE_OPTION(Allow);
	PARSE_OPTION(Disallow);
	PARSE_OPTION(Block);
	PARSE_OPTION(Available);
	PARSE_OPTION(True);
	PARSE_OPTION(False);
	PARSE_OPTION(Auto);
	PARSE_OPTION(Ask);
	PARSE_OPTION(Unknown);
	PARSE_OPTION(Stop);
	PARSE_OPTION(Run);
	if (tstrlen(_value) != 0)
	{
		error(TXT("option \"%S\" not recognised/allowed"), _value);
		assign_optional_out_param(result, false);
	}
	return _default;
}

tchar const * Option::to_char(Option::Type _option)
{
	OPTION(On, TXT("on"));
	OPTION(Off, TXT("off"));
	OPTION(Allow, TXT("allow"));
	OPTION(Disallow, TXT("disallow"));
	OPTION(Block, TXT("block"));
	OPTION(Available, TXT("available"));
	OPTION(True, TXT("true"));
	OPTION(False, TXT("false"));
	OPTION(Auto, TXT("auto"));
	OPTION(Ask, TXT("ask"));
	OPTION(Unknown, TXT("unknown"));
	OPTION(Stop, TXT("stop"));
	OPTION(Run, TXT("run"));
	return TXT("Off");
}
