#pragma once

#include "..\..\..\..\core\types\optional.h"

struct Vector3;

namespace Framework
{
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			namespace Utils
			{
				Vector3 apply_shooting_accuracy(Vector3 const & _velocity, Framework::IModulesOwner* _shooter, Framework::IModulesOwner* _enemy = nullptr, Optional<float> const& _enemyDistance = NP, Optional<float> const& _defDispersionAngle = NP);
			};
		};
	};
};