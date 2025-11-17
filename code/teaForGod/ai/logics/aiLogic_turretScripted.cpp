#include "aiLogic_turretScripted.h"

#include "tasks\aiLogicTask_attackOrIdle.h"
#include "tasks\aiLogicTask_hideDisappear.h"
#include "tasks\aiLogicTask_perception.h"

#include "utils\aiLogicUtil_shootingAccuracy.h"

#include "..\aiCommonVariables.h"
#include "..\aiRayCasts.h"
#include "..\perceptionRequests\aipr_FindAnywhere.h"

#include "..\..\game\gameDirector.h"
#include "..\..\game\gameSettings.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\music\musicPlayer.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiPerceptionRequest.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\nav\navPath.h"
#include "..\..\..\framework\nav\navTask.h"
#include "..\..\..\framework\nav\tasks\navTask_FindPath.h"
#include "..\..\..\framework\nav\navLatentUtils.h"
#include "..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\inRoomPlacement.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\roomUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai message names
DEFINE_STATIC_NAME_STR(aim_aim, TXT("turret scripted; aim"));

// ai message params
DEFINE_STATIC_NAME(aimAtPresenceCentre);
DEFINE_STATIC_NAME(atMS);
DEFINE_STATIC_NAME(atWS);
DEFINE_STATIC_NAME(atIMO);
DEFINE_STATIC_NAME(offsetTS);
DEFINE_STATIC_NAME(dropToWS);
DEFINE_STATIC_NAME(dropToMS);

// vars
DEFINE_STATIC_NAME(turretTargetMS);
DEFINE_STATIC_NAME(turretTargetingSocket);

//

REGISTER_FOR_FAST_CAST(TurretScripted);

TurretScripted::TurretScripted(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	turretTargetMSVarID = NAME(turretTargetMS);
}

TurretScripted::~TurretScripted()
{
}

void TurretScripted::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	Optional<Vector3> locWS;
	if (target.is_active())
	{
		Vector3 locTS = Vector3::zero;
		if (aimAtPresenceCentre)
		{
			if (auto* t = target.get_target())
			{
				locTS = t->get_presence()->get_centre_of_presence_os().get_translation();
			}
		}
		if (offsetTS.is_set())
		{
			locTS += offsetTS.get();
		}
		locWS = target.get_placement_in_owner_room().location_to_world(locTS);
	}

	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		if (locWS.is_set())
		{
			if (auto* a = imo->get_appearance())
			{
				targetMS = a->get_ms_to_ws_transform().location_to_local(locWS.get());
			}
		}
		auto & vars = imo->access_variables();
		if (!turretTargetMSVar.is_valid())
		{
			turretTargetMSVar = vars.find<Vector3>(turretTargetMSVarID);
		}

		turretTargetMSVar.access<Vector3>() = targetMS;
	}
}

LATENT_FUNCTION(TurretScripted::execute_logic)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	LATENT_BEGIN_CODE();

	if (auto* imo = mind->get_owner_as_modules_owner())
	{
		auto * self = fast_cast<TurretScripted>(mind->get_logic());

		self->target.set_owner(imo);

		if (Name const * var = imo->get_variables().get_existing<Name>(NAME(turretTargetingSocket)))
		{
			self->turretTargetingSocket.set_name(*var);

			if (auto* a = imo->get_appearance())
			{
				self->turretTargetingSocket.look_up(a->get_mesh());
			}
		}
	}

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(aim_aim), [mind](Framework::AI::Message const& _message)
			{
				if (auto* self = fast_cast<TurretScripted>(mind->get_logic()))
				{
					bool clearParams = true;
					if (auto* dropToWSParam = _message.get_param(NAME(dropToWS)))
					{
						if (dropToWSParam->get_as<bool>())
						{
							self->target.clear_target_but_follow();
							clearParams = false;
						}
					}
					if (auto* dropToMSParam = _message.get_param(NAME(dropToMS)))
					{
						if (dropToMSParam->get_as<bool>())
						{
							self->target.clear_target();
							clearParams = false;
						}
					}
					if (clearParams)
					{
						self->offsetTS.clear();
						self->aimAtPresenceCentre = false;
					}
					if (auto* atMSParam = _message.get_param(NAME(atMS)))
					{
						self->targetMS = atMSParam->get_as<Vector3>();
						self->target.clear_target();
					}
					if (auto* atWSParam = _message.get_param(NAME(atWS)))
					{
						self->target.set_placement_in_final_room(Transform(atWSParam->get_as<Vector3>(), Quat::identity));
					}
					if (auto* atIMOParam = _message.get_param(NAME(atIMO)))
					{
						self->target.find_path(mind->get_owner_as_modules_owner(), atIMOParam->get_as<SafePtr<Framework::IModulesOwner> >().get());
					}
					if (auto* offsetTSParam = _message.get_param(NAME(offsetTS)))
					{
						self->offsetTS = offsetTSParam->get_as<Vector3>();
					}
					if (auto* aimAtPresenceCentreParam = _message.get_param(NAME(aimAtPresenceCentre)))
					{
						self->aimAtPresenceCentre = aimAtPresenceCentreParam->get_as<bool>();
					}
				}
			}
		);
	}

	while (true)
	{
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//
	
	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}
