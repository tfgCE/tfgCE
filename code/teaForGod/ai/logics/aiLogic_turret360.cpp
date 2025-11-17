#include "aiLogic_turret360.h"

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

// variables
DEFINE_STATIC_NAME(projectileSpeed);
DEFINE_STATIC_NAME(shootCount);

//

REGISTER_FOR_FAST_CAST(Turret360);

Turret360::Turret360(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	turret360Data = fast_cast<Turret360Data>(_logicData);
}

Turret360::~Turret360()
{
}

void Turret360::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	shootCount = turret360Data->shootingBurst;
	shootCount = _parameters.get_value<int>(NAME(shootCount), shootCount);
	_parameters.access<int>(NAME(shootCount)) = shootCount;
}

void Turret360::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (extended != extendTarget)
	{
		extended = extendTarget;
		if (!extendedVar.is_valid())
		{
			extendedVar.set_name(turret360Data->extendedVarID);
			extendedVar.look_up<float>(get_mind()->get_owner_as_modules_owner()->access_variables());
		}
		extendedVar.access<float>() = extended;
	}
}

LATENT_FUNCTION(Turret360::execute_logic)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("turret 360 execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(bool, ignoreViolenceDisallowed);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, perceptionTask);
	LATENT_VAR(Random::Generator, rg);
	
	LATENT_VAR(float, timeLeft);
	LATENT_VAR(bool, alerted);
	LATENT_VAR(int, muzzleIdx);
	LATENT_VAR(int, muzzleDir);
	LATENT_VAR(int, muzzleOffsetIdx);
	LATENT_VAR(int, muzzleBurstOffset);

	timeLeft -= LATENT_DELTA_TIME;

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<Turret360>(logic);

	LATENT_BEGIN_CODE();

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::perception));
		perceptionTask.start_latent_task(mind, taskInfo);
	}

	if (auto* a = imo->get_appearance())
	{
		int idx = 0;
		while (true)
		{
			Name socketName = Name(String::printf(TXT("%S%i"), self->turret360Data->muzzleSocketPrefix.to_char(), idx));
			Framework::SocketID socket;
			socket.set_name(socketName);
			socket.look_up(a->get_mesh(), AllowToFail);
			if (socket.is_valid())
			{
				self->muzzleSockets.push_back(socket);
			}
			else
			{
				break;
			}
			++idx;
		}
	}

	while (true)
	{
		LATENT_CLEAR_LOG();

		if (GameDirector::is_violence_disallowed() && !ignoreViolenceDisallowed)
		{
			LATENT_WAIT(1.0f);
			self->extendTarget = 0.0f;
		}
		else
		{
			// wait
			LATENT_LOG(TXT("wait"));
			self->extendTarget = 0.0f;
			if (enemyPlacement.get_target())
			{
				alerted = true;
				timeLeft = rg.get_float(self->turret360Data->alertedInterval) * (1.0f - 0.5f * GameSettings::get().difficulty.aiMean);
			}
			else
			{
				alerted = false;
				timeLeft = rg.get_float(self->turret360Data->idleInterval) * (1.0f - 0.5f * GameSettings::get().difficulty.aiMean);
			}
			while (timeLeft > 0.0f)
			{
				if (!alerted && enemyPlacement.get_target())
				{
					alerted = true;
					timeLeft = min(timeLeft, rg.get_float(self->turret360Data->alertedInterval));
				}
				LATENT_WAIT(0.1f);
			}

			if (!GameDirector::is_violence_disallowed() || ignoreViolenceDisallowed)
			{
				// extend
				LATENT_LOG(TXT("extend"));
				self->extendTarget = 1.0f;

				// pre fire wait
				LATENT_LOG(TXT("pre fire wait"));
				LATENT_WAIT(max(/* hard coded extend time */ 0.2f, self->turret360Data->preFireWait * (1.0f - GameSettings::get().difficulty.aiMean)));

				// fire
				LATENT_LOG(TXT("fire!"));
				if (!GameDirector::is_violence_disallowed() || ignoreViolenceDisallowed)
				{
					muzzleIdx = 0;
					muzzleDir = rg.get_bool() ? 1 : -1;
					muzzleOffsetIdx = rg.get_int(self->muzzleSockets.get_size());
					muzzleBurstOffset = max(1, self->muzzleSockets.get_size() / max(1, self->turret360Data->shootingBurst));
					while (muzzleIdx < muzzleBurstOffset)
					{
						LATENT_LOG(TXT("fire idx %i"), muzzleIdx);
						if (auto* tos = imo->get_temporary_objects())
						{
							for_count(int, iBurst, max(1, self->turret360Data->shootingBurst))
							{
								int useMuzzleIdx = muzzleIdx + iBurst * muzzleBurstOffset;
								if (useMuzzleIdx < self->muzzleSockets.get_size())
								{
									LATENT_LOG(TXT("fire from muzzle %i"), useMuzzleIdx);
									Framework::ModuleTemporaryObjects::SpawnParams atSocket;
									atSocket.at_socket(self->muzzleSockets[mod(useMuzzleIdx * muzzleDir + muzzleOffsetIdx, self->muzzleSockets.get_size())].get_name());
									auto* projectile = tos->spawn(self->turret360Data->projectileTOID, atSocket);
									if (projectile)
									{
										// just in any case if we would be shooting from inside of a capsule
										if (auto* collision = projectile->get_collision())
										{
											collision->dont_collide_with_up_to_top_instigator(imo, 0.5f);
										}
										float useProjectileSpeed = projectile->get_variables().get_value<float>(NAME(projectileSpeed), 15.0f);
										useProjectileSpeed = imo->get_variables().get_value<float>(NAME(projectileSpeed), useProjectileSpeed);

										Vector3 velocity = Vector3(0.0f, useProjectileSpeed, 0.0f);
										velocity = Utils::apply_shooting_accuracy(velocity, imo);
										projectile->on_activate_set_relative_velocity(velocity);

										tos->spawn_all(self->turret360Data->muzzleFlashTOID, atSocket);

										if (auto* s = imo->get_sound())
										{
											s->play_sound(self->turret360Data->shootSoundID);
										}
									}
								}
							}
						}
						++muzzleIdx;
						LATENT_WAIT(self->turret360Data->shootingInterval);
					}
				}

				// post fire wait
				LATENT_LOG(TXT("post fire wait"));
				LATENT_WAIT(self->turret360Data->postFireWait * (1.0f - GameSettings::get().difficulty.aiMean));
			}

			// retract
			LATENT_LOG(TXT("retract"));
			self->extendTarget = 0.0f;
		}
	}

	LATENT_ON_BREAK();
	//
	
	LATENT_ON_END();
	//
	perceptionTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(Turret360Data);

Turret360Data::Turret360Data()
{
}

Turret360Data::~Turret360Data()
{
}

bool Turret360Data::load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	XML_LOAD_NAME_ATTR(_node, muzzleSocketPrefix);
	XML_LOAD_NAME_ATTR(_node, projectileTOID);
	XML_LOAD_NAME_ATTR(_node, muzzleFlashTOID);
	XML_LOAD_NAME_ATTR(_node, shootSoundID);

	XML_LOAD(_node, idleInterval);
	XML_LOAD(_node, alertedInterval);

	XML_LOAD_FLOAT_ATTR(_node, preFireWait);
	XML_LOAD_FLOAT_ATTR(_node, postFireWait);

	XML_LOAD_INT_ATTR(_node, shootingBurst);
	XML_LOAD_FLOAT_ATTR(_node, shootingInterval);

	XML_LOAD_NAME_ATTR(_node, extendedVarID);

	return result;
}

bool Turret360Data::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
