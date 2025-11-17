#pragma once

#include "..\containers\array.h"

struct String;

namespace IO
{
	namespace Base64
	{
		String encode(void const * _content, int _length);
	};
}