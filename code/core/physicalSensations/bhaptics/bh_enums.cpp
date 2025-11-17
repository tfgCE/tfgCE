#include "bh_enums.h"

//

using namespace an_bhaptics;

//

#define TO_CHAR(what) if (_value == what) return TXT(#what);
#define PARSE(what) if (_value == TXT(#what)) return what;

//

tchar const* PositionType::to_char(Type _value)
{
	TO_CHAR(Left);
	TO_CHAR(Right);
	TO_CHAR(Head);
	TO_CHAR(VestFront);
	TO_CHAR(VestBack);
	TO_CHAR(Vest);
	TO_CHAR(HandL);
	TO_CHAR(HandR);
	TO_CHAR(FootL);
	TO_CHAR(FootR);
	TO_CHAR(ForearmL);
	TO_CHAR(ForearmR);

	error(TXT("unknown bhaptics positiontype (to_char %i)"), _value);
	return TXT("Vest");
}

PositionType::Type PositionType::parse(String const& _value, Type _default)
{
	PARSE(Left);
	PARSE(Right);
	PARSE(Head);
	PARSE(VestFront);
	PARSE(VestBack);
	PARSE(Vest);
	PARSE(HandL);
	PARSE(HandR);
	PARSE(FootL);
	PARSE(FootR);
	PARSE(ForearmL);
	PARSE(ForearmR);

	error(TXT("unknown bhaptics positiontype (parse %S)"), _value.to_char());
	return Vest;
}
