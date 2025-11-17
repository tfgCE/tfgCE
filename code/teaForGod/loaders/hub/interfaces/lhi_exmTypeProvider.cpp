#include "lhi_exmTypeProvider.h"

#include "..\..\..\game\energy.h"

//

using namespace Loader;
using namespace HubInterfaces;

//

REGISTER_FOR_FAST_CAST(IEXMTypeProvider);

TeaForGodEmperor::EnergyCoef IEXMTypeProvider::get_exm_coef() const
{
	return TeaForGodEmperor::EnergyCoef::one();
}
