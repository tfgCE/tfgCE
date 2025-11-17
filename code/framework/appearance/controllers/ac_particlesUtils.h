#pragma once

#include "..\..\..\core\globalDefinitions.h"

namespace Framework
{
	interface_class IModulesOwner;

	class ParticlesUtils
	{
	public:
		static void desire_to_deactivate(IModulesOwner* _imo, bool _attached = true);
		static void handle_special_case(IModulesOwner* _imo);
	};
};
