#pragma once

#include "..\..\core\fastCast.h"
#include "..\..\core\globalDefinitions.h"

namespace Framework
{
	interface_class IAIMindOwner
	{
		FAST_CAST_DECLARE(IAIMindOwner);
		FAST_CAST_END();
	public:
		IAIMindOwner();
		virtual ~IAIMindOwner() {}
	};
};
