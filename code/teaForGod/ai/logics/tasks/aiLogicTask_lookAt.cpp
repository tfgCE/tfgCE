#include "aiLogicTask_lookAt.h"

#include "..\aiLogic_npcBase.h"

#include "..\..\aiCommon.h"
#include "..\..\aiCommonVariables.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\ai\aiTaskHandle.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\presence\presencePath.h"
#include "..\..\..\..\framework\presence\relativeToPresencePlacement.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

DEFINE_STATIC_NAME(lookAt);
DEFINE_STATIC_NAME(lookAtMS);
DEFINE_STATIC_NAME(lookAtActive);
DEFINE_STATIC_NAME(lookAtOffset);

//

LATENT_FUNCTION(TeaForGodEmperor::AI::Logics::Tasks::look_at)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("look at"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_PARAM_OPTIONAL(int*, lookAtBlock, nullptr);
	LATENT_PARAM_OPTIONAL(::Framework::PresencePath*, presencePath, nullptr);
	LATENT_PARAM_OPTIONAL(::Framework::RelativeToPresencePlacement*, relativeToPresencePlacement, nullptr);
	LATENT_PARAM_OPTIONAL(Vector3*, relativeToPresencePlacementOffsetOS, nullptr);
	LATENT_PARAM_OPTIONAL(Vector3*, locationMS, nullptr);
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(SafePtr<Framework::IModulesOwner>, scriptedLookAt);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::PresencePath, scriptedLookAtPath);

	LATENT_BEGIN_CODE();

	// keep this loop running
	while (true)
	{
		if (auto * imo = mind->get_owner_as_modules_owner())
		{
			// check scripted look at
			{
				if (auto* targetImo = scriptedLookAt.get())
				{
					if (!scriptedLookAtPath.is_active())
					{
						scriptedLookAtPath.find_path(imo, targetImo);
					}
				}
				else
				{
					scriptedLookAtPath.clear_target();
				}
			}
			//
			{
				if (scriptedLookAtPath.is_active())
				{
					imo->access_variables().access<Vector3*>(NAME(lookAtOffset)) = relativeToPresencePlacementOffsetOS;
					imo->access_variables().access<::Framework::PresencePath*>(NAME(lookAt)) = &scriptedLookAtPath;
					imo->access_variables().access<::Framework::RelativeToPresencePlacement*>(NAME(lookAt)) = nullptr;
					imo->access_variables().access<Vector3*>(NAME(lookAtMS)) = nullptr;
					imo->access_variables().access<float>(NAME(lookAtActive)) = !lookAtBlock || !*lookAtBlock ? 1.0f : 0.0f;
				}
				else if (presencePath && presencePath->is_active())
				{
					imo->access_variables().access<Vector3*>(NAME(lookAtOffset)) = relativeToPresencePlacementOffsetOS;
					imo->access_variables().access<::Framework::PresencePath*>(NAME(lookAt)) = presencePath;
					imo->access_variables().access<::Framework::RelativeToPresencePlacement*>(NAME(lookAt)) = nullptr;
					imo->access_variables().access<Vector3*>(NAME(lookAtMS)) = nullptr;
					imo->access_variables().access<float>(NAME(lookAtActive)) = !lookAtBlock || !*lookAtBlock ? 1.0f : 0.0f;
				}
				else if (relativeToPresencePlacement && relativeToPresencePlacement->is_active())
				{
					imo->access_variables().access<Vector3*>(NAME(lookAtOffset)) = relativeToPresencePlacementOffsetOS;
					imo->access_variables().access<::Framework::PresencePath*>(NAME(lookAt)) = nullptr;
					imo->access_variables().access<::Framework::RelativeToPresencePlacement*>(NAME(lookAt)) = relativeToPresencePlacement;
					imo->access_variables().access<Vector3*>(NAME(lookAtMS)) = nullptr;
					imo->access_variables().access<float>(NAME(lookAtActive)) = !lookAtBlock || !*lookAtBlock ? 1.0f : 0.0f;
				}
				else if (locationMS)
				{
					imo->access_variables().access<Vector3*>(NAME(lookAtOffset)) = nullptr;
					imo->access_variables().access<::Framework::PresencePath*>(NAME(lookAt)) = nullptr;
					imo->access_variables().access<::Framework::RelativeToPresencePlacement*>(NAME(lookAt)) = nullptr;
					imo->access_variables().access<Vector3*>(NAME(lookAtMS)) = locationMS;
					imo->access_variables().access<float>(NAME(lookAtActive)) = !lookAtBlock || !*lookAtBlock ? 1.0f : 0.0f;
				}
				else
				{
					imo->access_variables().access<Vector3*>(NAME(lookAtOffset)) = nullptr;
					imo->access_variables().access<::Framework::PresencePath*>(NAME(lookAt)) = nullptr;
					imo->access_variables().access<::Framework::RelativeToPresencePlacement*>(NAME(lookAt)) = nullptr;
					imo->access_variables().access<Vector3*>(NAME(lookAtMS)) = nullptr;
					imo->access_variables().access<float>(NAME(lookAtActive)) = 0.0f;
				}
			}
		}

		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//
	LATENT_ON_END();
	//
	// clean up to make the character look straight
	if (auto * imo = mind->get_owner_as_modules_owner())
	{
		imo->access_variables().access<Vector3*>(NAME(lookAtOffset)) = nullptr;
		imo->access_variables().access<::Framework::RelativeToPresencePlacement*>(NAME(lookAt)) = nullptr;
		imo->access_variables().access<::Framework::PresencePath*>(NAME(lookAt)) = nullptr;
		imo->access_variables().access<Vector3*>(NAME(lookAtMS)) = nullptr;
		imo->access_variables().access<float>(NAME(lookAtActive)) = 0.0f;
	}

	LATENT_END_CODE();
	LATENT_RETURN();
}

//
