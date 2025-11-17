#pragma once

#include "..\..\core\globalDefinitions.h"

namespace Framework
{
	interface_class IModulesOwner;

	namespace GameUtils
	{
		bool is_player(Framework::IModulesOwner const* imo);
		bool is_local_player(Framework::IModulesOwner const* imo);
		bool is_controlled_by_player(Framework::IModulesOwner const* imo);
		bool is_controlled_by_local_player(Framework::IModulesOwner const* imo);
	}
}
