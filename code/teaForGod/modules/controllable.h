#pragma once

#include "..\..\core\fastCast.h"
#include "..\..\core\types\hand.h"

namespace Framework
{
	interface_class IModulesOwner;
	class GameInput;
};

namespace TeaForGodEmperor
{
	interface_class IControllable
	{
		FAST_CAST_DECLARE(IControllable);
		FAST_CAST_END();

	public:
		virtual void pre_process_controls() {}
		virtual void process_controls(Hand::Type _hand, Framework::GameInput const& _controls, float _deltaTime) = 0;
	};
};
