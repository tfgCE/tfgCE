#pragma once

#include "..\..\..\..\core\latent\latent.h"

namespace Framework
{
	interface_class IModulesOwner;
	struct PresencePath;
	struct RelativeToPresencePlacement;

	namespace AI
	{
		class Message;
		class MindInstance;
		struct LatentTaskInfoWithParams;
	};
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			namespace Tasks
			{
				bool is_aiming_at_me(Framework::PresencePath const & _pathToEnemy, Framework::IModulesOwner * _me, float _maxOffPath, float _maxAngleOff, float _maxDistance);
				bool is_aiming_at_me(Framework::RelativeToPresencePlacement const & _pathToEnemy, Framework::IModulesOwner * _me, float _maxOffPath, float _maxAngleOff, float _maxDistance);

				LATENT_FUNCTION(evade_aiming_3d);
			};
		};
	};
};
