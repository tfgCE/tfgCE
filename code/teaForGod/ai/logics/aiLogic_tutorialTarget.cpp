#include "aiLogic_tutorialTarget.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\world\room.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

REGISTER_FOR_FAST_CAST(TutorialTarget);

TutorialTarget::TutorialTarget(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

TutorialTarget::~TutorialTarget()
{
}

LATENT_FUNCTION(TutorialTarget::execute_logic)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(SafePtr<Framework::Room>, initialRoom);
	LATENT_VAR(Vector3, initialLocationWS);

	LATENT_BEGIN_CODE();

	{
		auto* imo = mind->get_owner_as_modules_owner();
		initialRoom = imo->get_presence()->get_in_room();
		initialLocationWS = imo->get_presence()->get_placement().get_translation();
	}

	while (true)
	{
		{
			auto* imo = mind->get_owner_as_modules_owner();
			if (auto* ai = imo->get_ai())
			{
				if (auto* mind = ai->get_mind())
				{
					if (imo->get_presence()->get_in_room() == initialRoom.get())
					{
						mind->access_locomotion().move_to_3d(initialLocationWS, 0.1f, Framework::MovementParameters().full_speed());
					}
					else
					{
						mind->access_locomotion().stop();
					}
				}
			}
		}
		LATENT_WAIT(Random::get_float(0.2f, 0.8f));
	}

	LATENT_ON_BREAK();
	//
	
	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}
