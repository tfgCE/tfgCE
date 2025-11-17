#pragma once

#include "..\globalDefinitions.h"

namespace Unicode
{
	void initialise_static();
	void close_static();

	tchar unicode_to_tchar(uint32 _unicode);
	uint32 tchar_to_unicode(tchar _ch);
};
