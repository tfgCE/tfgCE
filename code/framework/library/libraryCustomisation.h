#pragma once

#include "..\..\core\types\name.h"

namespace Framework
{
	struct LibraryCustomisation
	{
	public: // allow easy access to setup
		Name defaultGameplayModuleTypeForActorType;
		Name defaultGameplayModuleTypeForItemType;
		Name defaultGameplayModuleTypeForObjectType;
		Name defaultGameplayModuleTypeForSceneryType;
		Name defaultGameplayModuleTypeForTemporaryObjectType;
	};
};
