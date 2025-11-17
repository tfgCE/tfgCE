#pragma once

#include "..\casts.h"
#include "..\globalDefinitions.h"

inline int compare_int(void const* _a, void const* _b)
{
	int a = *plain_cast<int>(_a);
	int b = *plain_cast<int>(_b);
	if (a < b) return A_BEFORE_B;
	if (a > b) return B_BEFORE_A;
	return A_AS_B;
}
