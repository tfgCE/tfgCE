#pragma once

#include "..\globalDefinitions.h"

struct String;

namespace VRMode
{
	enum Type
	{
		Normal,
		ExtraLatency,
		PhaseSync,

		Default = ExtraLatency
	};

	tchar const* to_char(Type _t);
	Type parse(String const& _v);
};
