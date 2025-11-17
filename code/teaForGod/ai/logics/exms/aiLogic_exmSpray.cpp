#include "aiLogic_exmSpray.h"

#include "..\..\..\game\gameDirector.h"
#include "..\..\..\modules\custom\mc_pickup.h"
#include "..\..\..\modules\custom\mc_switchSidesHandler.h"
#include "..\..\..\modules\gameplay\moduleEXM.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\..\utils\collectInTargetCone.h"

#include "..\..\..\..\core\containers\arrayStack.h"

#include "..\..\..\..\framework\appearance\controllers\ac_particlesUtils.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\..\framework\object\temporaryObject.h"

#include "..\..\..\..\core\debug\debugRenderer.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define DEBUG_DRAW_PUSHING

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// temporary object
DEFINE_STATIC_NAME(spray);

// socket
DEFINE_STATIC_NAME_STR(exmSpray, TXT("exm spray"));

//

REGISTER_FOR_FAST_CAST(EXMSpray);

EXMSpray::EXMSpray(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
}

EXMSpray::~EXMSpray()
{
	end_spray();
}

void EXMSpray::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);
}

void EXMSpray::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (pilgrimOwner.get() && pilgrimModule && exmModule && owner.get() && hand != Hand::MAX)
	{
		bool pressedNow = pilgrimModule->is_controls_use_exm_pressed(hand) &&
			pilgrimModule->is_active_exm_equipped(hand, owner.get());
		if (pressedNow && !spawnedSpray.get())
		{
			if (pilgrimModule->get_hand(hand) &&
				exmModule->mark_exm_active(true))
			{
				start_spray();
			}
		}
		else if ((!pressedNow && spawnedSpray.get()) || ! exmModule->is_exm_active()) // could be deactivated due to lack of energy
		{
			end_spray();
			
			exmModule->mark_exm_active(false);
		}
	}
}

bool EXMSpray::start_spray()
{
	end_spray();
	if (pilgrimOwner.get() && pilgrimModule && hand != Hand::MAX)
	{
		if (auto* handIMO = pilgrimModule->get_hand(hand))
		{
			if (auto* imo = owner.get())
			{
				if (auto* to = imo->get_temporary_objects())
				{
					spawnedSpray = to->spawn_attached_to(NAME(spray), handIMO, Framework::ModuleTemporaryObjects::SpawnParams().at_socket(NAME(exmSpray)).at_relative_placement(Transform(Vector3::yAxis * 0.05f, Quat::identity)));
				}
			}
		}
	}
	return spawnedSpray.get() != nullptr;
}

void EXMSpray::end_spray()
{
	if (auto* imo = spawnedSpray.get())
	{
		Framework::ParticlesUtils::desire_to_deactivate(imo);
	}
	spawnedSpray.clear();
}