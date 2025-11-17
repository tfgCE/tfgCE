#pragma once

#include "..\..\framework\module\modulePresence.h"

namespace TeaForGodEmperor
{
	class ModulePresencePilgrim
	: public Framework::ModulePresence
	{
		FAST_CAST_DECLARE(ModulePresencePilgrim);
		FAST_CAST_BASE(Framework::ModulePresence);
		FAST_CAST_END();

		typedef Framework::ModulePresence base;
	public:
		static Framework::RegisteredModule<Framework::ModulePresence> & register_itself();

		ModulePresencePilgrim(Framework::IModulesOwner* _owner);
	};
};

