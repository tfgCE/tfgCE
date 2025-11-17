#include "gse_shoot.h"


#include "..\..\ai\logics\aiLogic_npcBase.h"
#include "..\..\ai\logics\utils\aiLogicUtil_shootingAccuracy.h"
#include "..\..\game\game.h"
#include "..\..\library\library.h"
#include "..\..\modules\moduleAI.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\gameScript\gameScript.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"

#include "..\..\..\core\io\xml.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

// sounds / temporary objects
DEFINE_STATIC_NAME(shoot);
DEFINE_STATIC_NAME(muzzleFlash);

// variables
DEFINE_STATIC_NAME(projectileSpeed);

//

bool Shoot::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	socket.load_from_xml(_node, TXT("socket"));
	socketVar.load_from_xml(_node, TXT("socketVar"));

	justProjectile = _node->get_bool_attribute_or_from_child_presence(TXT("justProjectile"), justProjectile);

	return result;
}

bool Shoot::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Shoot::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
	{
		if (auto* imo = exPtr->get())
		{
			Concurrency::ScopedSpinLock lock(imo->access_lock());

			if (auto* ai = imo->get_ai())
			{
				auto* mind = ai->get_mind();

				Optional<Name> useSocket = socket;
				if (socketVar.is_set())
				{
					if (auto* v = _execution.get_variables().get_existing<Name>(socketVar.get()))
					{
						useSocket = *v;
					}
				}

				AI::Logics::NPCBase::ShotInfo shotInfo(NAME(shoot), NAME(muzzleFlash));
				if (auto* npcBase = fast_cast<AI::Logics::NPCBase>(mind->get_logic()))
				{
					shotInfo = npcBase->get_shot_infos()[Random::get_int(npcBase->get_shot_infos().get_size())];
				}

				if (auto* tos = imo->get_temporary_objects())
				{
					auto* projectile = tos->spawn(shotInfo.projectile, Framework::ModuleTemporaryObjects::SpawnParams().at_socket(useSocket));
					if (projectile)
					{
						// just in any case if we would be shooting from inside of a capsule
						if (auto* collision = projectile->get_collision())
						{
							collision->dont_collide_with_up_to_top_instigator(imo, 0.5f);
						}
						float useProjectileSpeed = 0.0f;
						useProjectileSpeed = projectile->get_variables().get_value<float>(NAME(projectileSpeed), 15.0f);
						useProjectileSpeed = imo->get_variables().get_value<float>(NAME(projectileSpeed), useProjectileSpeed);
						Vector3 velocity = Vector3(0.0f, useProjectileSpeed, 0.0f);
						velocity = AI::Logics::Utils::apply_shooting_accuracy(velocity, imo, nullptr, 10.0f);
						projectile->on_activate_set_relative_velocity(velocity);

						if (!justProjectile)
						{
							tos->spawn_all(shotInfo.muzzleFlash, Framework::ModuleTemporaryObjects::SpawnParams().at_socket(useSocket));

							if (auto* s = imo->get_sound())
							{
								s->play_sound(shotInfo.sound, useSocket);
							}
						}
					}
				}
			}
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
