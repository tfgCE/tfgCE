#include "aiLogic_heartRoomProjectiles.h"

#include "..\..\game\energy.h"
#include "..\..\game\damage.h"
#include "..\..\game\game.h"
#include "..\..\game\gameDirector.h"

#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\health\mc_health.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai messages
DEFINE_STATIC_NAME_STR(aim_start, TXT("heart room projectiles; start"));
	DEFINE_STATIC_NAME(interval);
	DEFINE_STATIC_NAME(shotCount);
DEFINE_STATIC_NAME_STR(aim_stop, TXT("heart room projectiles; stop"));

// temporary objects
DEFINE_STATIC_NAME(shoot);
DEFINE_STATIC_NAME(muzzleFlash);

// variables 
DEFINE_STATIC_NAME(projectileSpeed);

//

REGISTER_FOR_FAST_CAST(HeartRoomProjectiles);

HeartRoomProjectiles::HeartRoomProjectiles(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const* _logicData)
: base(_mind, _logicData, execute_logic)
{
	heartRoomProjectilesData = fast_cast<HeartRoomProjectilesData>(_logicData);
}

HeartRoomProjectiles::~HeartRoomProjectiles()
{
}

void HeartRoomProjectiles::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);
}

void HeartRoomProjectiles::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (!readyAndRunning)
	{
		return;
	}


	if (active)
	{
		timeToShoot -= _deltaTime;
		if (timeToShoot <= 0.0f)
		{
			timeToShoot = interval;
			if (timeToShoot <= 0.0f)
			{
				active = false;
			}

			auto* imo = get_mind()->get_owner_as_modules_owner();

			if (auto* tos = imo->get_temporary_objects())
			{
				todo_multiplayer_issue(TXT("we just get player here"));
				Transform eyesWS = Transform::identity;
				if (auto* g = Game::get_as<Game>())
				{
					if (auto* pa = g->access_player().get_actor())
					{
						if (auto* presence = pa->get_presence())
						{
							eyesWS = presence->get_placement().to_world(presence->get_eyes_relative_look());
						}
					}
				}

				if (auto* tos = imo->get_temporary_objects())
				{
					if (auto* projectile = tos->spawn(NAME(shoot)))
					{
						// just in any case if we would be shooting from inside of a capsule
						if (auto* collision = projectile->get_collision())
						{
							collision->dont_collide_with_up_to_top_instigator(imo, 0.5f);
						}
						float useProjectileSpeed = projectile->get_variables().get_value<float>(NAME(projectileSpeed), 10.0f);
						useProjectileSpeed = imo->get_variables().get_value<float>(NAME(projectileSpeed), useProjectileSpeed);

						projectile->on_activate_face_towards(eyesWS.get_translation());

						Vector3 velocity = Vector3(0.0f, useProjectileSpeed, 0.0f);
						projectile->on_activate_set_relative_velocity(velocity);

						tos->spawn_all(NAME(muzzleFlash));

						if (auto* s = imo->get_sound())
						{
							s->play_sound(NAME(shoot));
						}
					}
				}
			}

			if (shotsLeft.is_set())
			{
				shotsLeft = shotsLeft.get() - 1;
				if (shotsLeft.get() <= 0)
				{
					active = false;
				}
			}
		}
	}

}

LATENT_FUNCTION(HeartRoomProjectiles::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[heart room beams] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto * self = fast_cast<HeartRoomProjectiles>(logic);

	LATENT_BEGIN_CODE();

	ai_log(self, TXT("heart room beams, hello!"));

	{
		messageHandler.use_with(mind);
		{
			messageHandler.set(NAME(aim_start), [self](Framework::AI::Message const& _message)
				{
					self->active = true;
					self->timeToShoot = 0.0f;
					self->interval = 0.0f; // single without non zero interval
					self->shotsLeft.clear();
					if (auto* p = _message.get_param(NAME(interval)))
					{
						self->interval = p->get_as<float>();
					}
					if (auto* p = _message.get_param(NAME(shotCount)))
					{
						self->shotsLeft = p->get_as<int>();
					}
				}
			);
			messageHandler.set(NAME(aim_stop), [self](Framework::AI::Message const& _message)
				{
					self->active = false;
				}
			);
		}
	}

	LATENT_WAIT(0.1f);

	self->readyAndRunning = true;

	while (true)
	{
		LATENT_WAIT_NO_RARE_ADVANCE(self->rg.get_float(0.1f, 0.2f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//--

REGISTER_FOR_FAST_CAST(HeartRoomProjectilesData);

bool HeartRoomProjectilesData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool HeartRoomProjectilesData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
