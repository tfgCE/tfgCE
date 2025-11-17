#pragma once

#include "..\..\..\..\core\latent\latent.h"

#include "..\aiLogic_npcBase.h"

namespace Framework
{
	namespace AI
	{
		class MindInstance;
	};

	struct RelativeToPresencePlacement;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			namespace Tasks
			{
				LATENT_FUNCTION(shoot);

				namespace Functions
				{
					// uses also owner variables:
					//	float	projectileSpeed
					//	float	shootingAccuracyMaxOffAngle			- how much off each projectile goes
					//	bool	shootingForcedAtTarget				- if true, projectile will go right at the target, not using the socket
					//	float	shootingMaxAllowedOffTargetAngle	- (default -1), if 0 or positive, will shoot only if aiming at the target with max this angle off
					//
					// uses also projectile variables:
					//	float	projectileSpeed
					//
					// returns if shot or not

					bool perform_shoot(::Framework::AI::MindInstance* mind, int shootIndex, Framework::RelativeToPresencePlacement const& enemyPlacement,
						Vector3 const& enemyTargetingOffsetOS, Optional<float> const& _projectileSpeed, Optional<bool> const & _ignoreViolenceDisallowed,
						Array<NPCBase::ShotInfo> const* _useShotInfos);

					void trigger_combat_auto_music(Framework::RelativeToPresencePlacement const& enemyPlacement);
				};
			};
		};
	};
};
