#include "aiLogic_portableShield.h"

#include "tasks\aiLogicTask_shield.h"
#include "..\..\game\damage.h"
#include "..\..\modules\custom\health\mc_health.h"

#include "..\..\..\framework\ai\aiMindInstance.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

//

REGISTER_FOR_FAST_CAST(PortableShield);

PortableShield::PortableShield(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

PortableShield::~PortableShield()
{
}

LATENT_FUNCTION(PortableShield::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, extraTask);

	LATENT_BEGIN_CODE();

	{
		::Framework::AI::LatentTaskInfoWithParams nextTaskInfo;
		nextTaskInfo.clear();
		nextTaskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::shield));
		extraTask.start_latent_task(mind, nextTaskInfo);
	}

	while (true)
	{
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}
