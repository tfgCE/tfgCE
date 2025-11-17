#include "aiLogic_doorbot.h"

#include "tasks\aiLogicTask_lookAt.h"

#include "..\aiCommon.h"
#include "..\aiCommonVariables.h"

#include "..\perceptionRequests\aipr_FindAnywhere.h"
#include "..\perceptionRequests\aipr_FindInVisibleRooms.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\nav\navLatentUtils.h"
#include "..\..\..\framework\nav\navPath.h"
#include "..\..\..\framework\nav\navTask.h"
#include "..\..\..\framework\nav\tasks\navTask_FindPath.h"
#include "..\..\..\framework\nav\tasks\navTask_GetRandomLocation.h"
#include "..\..\..\framework\object\scenery.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\roomUtils.h"

#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

DEFINE_STATIC_NAME(evade);
DEFINE_STATIC_NAME(projectileHit);
DEFINE_STATIC_NAME(hit);
DEFINE_STATIC_NAME(inRoom);
DEFINE_STATIC_NAME(inRoomLocation);
DEFINE_STATIC_NAME(inRoomNormal);

DEFINE_STATIC_NAME(follow);
DEFINE_STATIC_NAME(stopAndWait);

// ai message names
DEFINE_STATIC_NAME(dealtDamage);
DEFINE_STATIC_NAME(someoneElseDied);

// ai message params
DEFINE_STATIC_NAME(damageSource);
DEFINE_STATIC_NAME(killer);

// tags
DEFINE_STATIC_NAME(interestedInInterestingDevices);
DEFINE_STATIC_NAME(interestingDevice);

//

namespace WhatToDo
{
	enum Type
	{
		StandStill,
		UsualStuff,
		GreetPilgrim,
		RunAway,
	};
};

//

REGISTER_FOR_FAST_CAST(Doorbot);

Doorbot::Doorbot(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

Doorbot::~Doorbot()
{
}

void Doorbot::advance(float _deltaTime)
{
	set_auto_rare_advance_if_not_visible(Range(0.1f, 0.5f));

	base::advance(_deltaTime);
}

LATENT_FUNCTION(Doorbot::execute_logic)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, lookAtTask);

	LATENT_BEGIN_CODE();

	{	// setup and start look at task
		::Framework::AI::LatentTaskInfoWithParams lookAtTaskInfo;
		lookAtTaskInfo.clear();
		lookAtTaskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::look_at));
		lookAtTask.start_latent_task(mind, lookAtTaskInfo);
	}

	LATENT_WAIT(Random::get_float(0.1f, 0.6f));

	while (true)
	{
		LATENT_WAIT(Random::get_float(0.05f, 0.25f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	lookAtTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}
