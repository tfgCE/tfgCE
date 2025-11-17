#pragma once

#include "..\..\..\..\core\fastCast.h"

namespace TeaForGodEmperor
{
	class EXMType;
	struct EnergyCoef;
};

namespace Loader
{
	namespace HubInterfaces
	{
		interface_class IEXMTypeProvider
		{
			FAST_CAST_DECLARE(IEXMTypeProvider);
			FAST_CAST_END();

		public:
			virtual ~IEXMTypeProvider() {}

			virtual TeaForGodEmperor::EXMType const* get_exm_type() const = 0;
			virtual TeaForGodEmperor::EnergyCoef get_exm_coef() const;
		};

	};
};
