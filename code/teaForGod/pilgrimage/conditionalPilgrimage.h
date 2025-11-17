#pragma once

#include "..\..\core\tags\tagCondition.h"
#include "..\..\framework\library\usedLibraryStored.h"

namespace TeaForGodEmperor
{
	class Pilgrimage;

	struct ConditionalPilgrimage
	{
		Framework::UsedLibraryStored<Pilgrimage> pilgrimage;
		TagCondition generalProgressRequirement;
		TagCondition gameSlotGeneralProgressRequirement;
		TagCondition profileGeneralProgressRequirement;
	};
};

