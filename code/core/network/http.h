#pragma once

#include "..\globalDefinitions.h"

struct String;

namespace Network
{
	namespace HTTP
	{
		String get(tchar const* address);
	};
};
