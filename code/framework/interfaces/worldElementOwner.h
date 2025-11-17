#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\fastCast.h"

namespace Framework
{
	interface_class IWorldElementOwner
	{
		FAST_CAST_DECLARE(IWorldElementOwner);
		FAST_CAST_END();
	public:
		virtual ~IWorldElementOwner() {}
	};
};
