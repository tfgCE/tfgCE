#include "vrMode.h"

#include "..\types\string.h"

//

tchar const* VRMode::to_char(Type _t)
{
	if (_t == Normal) return TXT("normal");
	if (_t == ExtraLatency) return TXT("extraLatency");
	if (_t == PhaseSync) return TXT("phaseSync");
	return TXT("");
}

VRMode::Type VRMode::parse(String const& _v)
{
	if (_v == to_char(Normal)) return Normal;
	if (_v == to_char(ExtraLatency)) return ExtraLatency;
	if (_v == to_char(PhaseSync)) return PhaseSync;
	return Default; // default
}
