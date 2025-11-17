#include "aiLogic_exmProjectile.h"

#include "..\..\..\game\gameDirector.h"
#include "..\..\..\modules\custom\mc_pickup.h"
#include "..\..\..\modules\custom\mc_switchSidesHandler.h"
#include "..\..\..\modules\gameplay\moduleEXM.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\..\utils\collectInTargetCone.h"

#include "..\..\..\..\core\containers\arrayStack.h"

#include "..\..\..\..\framework\appearance\controllers\ac_particlesUtils.h"
#include "..\..\..\..\framework\module\moduleCollision.h"
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
DEFINE_STATIC_NAME(projectile);

// variables
DEFINE_STATIC_NAME(projectileSpeed);

// socket
DEFINE_STATIC_NAME_STR(exmProjectile, TXT("exm projectile"));

//

REGISTER_FOR_FAST_CAST(EXMProjectile);

EXMProjectile::EXMProjectile(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
}

EXMProjectile::~EXMProjectile()
{
}

void EXMProjectile::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	projectileSpeed = _parameters.get_value(NAME(projectileSpeed), 1.0f);
}

void EXMProjectile::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (pilgrimOwner.get() && pilgrimModule && exmModule && owner.get() && hand != Hand::MAX)
	{
		bool pressedNow = pilgrimModule->has_controls_use_exm_been_pressed(hand) &&
			pilgrimModule->is_active_exm_equipped(hand, owner.get());
		if (pressedNow)
		{
			if (pilgrimModule->get_hand(hand) &&
				exmModule->mark_exm_active_blink())
			{
				throw_projectile();
			}
		}
	}
}

bool EXMProjectile::throw_projectile()
{
	if (pilgrimOwner.get() && pilgrimModule && hand != Hand::MAX)
	{
		if (auto* handIMO = pilgrimModule->get_hand(hand))
		{
			if (auto* imo = owner.get())
			{
				if (auto* to = imo->get_temporary_objects())
				{
					auto* spawnedProjectile = to->spawn(NAME(projectile));

					if (spawnedProjectile)
					{
						spawnedProjectile->on_activate_place_at(handIMO, NAME(exmProjectile), Transform(Vector3::yAxis * 0.1f, Quat::identity));
						spawnedProjectile->on_activate_set_relative_velocity(Vector3::yAxis * projectileSpeed);
						if (auto* c = spawnedProjectile->get_collision())
						{
							c->dont_collide_with_if_attached_to_top(pilgrimOwner.get(), 2.0f);
						}
					}

					return true;
				}
			}
		}
	}
	return false;
}
