#pragma once

#include "..\..\..\..\core\functionParamsStruct.h"
#include "..\..\..\..\core\types\name.h"

#include "..\..\..\game\energy.h"

namespace Framework
{
	struct RelativeToPresencePlacement;

	namespace AI
	{
		class MindInstance;
	};
};

namespace TeaForGodEmperor
{
	struct Energy;

	namespace AI
	{
		namespace Actions
		{
			struct LightningStrikeParams
			{
				ADD_FUNCTION_PARAM_DEF(LightningStrikeParams, bool, ignoreViolenceDisallowed, ignore_violence_disallowed, true);
				ADD_FUNCTION_PARAM_DEF(LightningStrikeParams, bool, forceDischarge, force_discharge, true);
				ADD_FUNCTION_PARAM_DEF(LightningStrikeParams, bool, noHitTemporaryObject, no_hit_temporary_object, true);
				ADD_FUNCTION_PARAM(LightningStrikeParams, Name, hitTemporaryObjectID, hit_temporary_object_id);
				ADD_FUNCTION_PARAM(LightningStrikeParams, Name, lightningSpawnID, lightning_spawn_id);
				ADD_FUNCTION_PARAM(LightningStrikeParams, float, angleLimit, with_angle_limit);
				ADD_FUNCTION_PARAM(LightningStrikeParams, float, verticalAngleLimit, with_vertical_angle_limit);
				ADD_FUNCTION_PARAM(LightningStrikeParams, float, extraDistance, with_extra_distance);
				ADD_FUNCTION_PARAM(LightningStrikeParams, Name, fromSocket, from_socket);
				ADD_FUNCTION_PARAM(LightningStrikeParams, Energy, dischargeDamage, discharge_damage);
				ADD_FUNCTION_PARAM(LightningStrikeParams, Energy, dischargeDamageFallback, discharge_damage_fallback);
			};
			// returns true on a successful attack
			bool do_lightning_strike(Framework::AI::MindInstance* mind, Framework::RelativeToPresencePlacement & enemyPlacement, float maxDist, Optional<LightningStrikeParams> const & _lightningStrikeParams = NP);
		};
	};
};