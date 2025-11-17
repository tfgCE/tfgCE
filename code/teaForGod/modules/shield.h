#pragma once

#include "..\..\core\fastCast.h"
#include "..\game\energy.h"
#include "..\teaEnums.h"

namespace Framework
{
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	interface_class IShield
	{
		FAST_CAST_DECLARE(IShield);
		FAST_CAST_END();

	public:
		virtual Vector3 get_cover_dir_os() const = 0;
	};
};
