#include "aiLogicTask_shoot.h"

#include "aiLogicTask_perception.h"

#include "..\aiLogic_npcBase.h"

#include "..\utils\aiLogicUtil_shootingAccuracy.h"

#include "..\..\aiCommonVariables.h"

#include "..\..\..\game\gameDirector.h"

#include "..\..\..\music\musicPlayer.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\ai\aiTaskHandle.h"
#include "..\..\..\..\framework\game\gameUtils.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\..\framework\module\moduleSound.h"
#include "..\..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// sounds / temporary objects
DEFINE_STATIC_NAME(shoot);
DEFINE_STATIC_NAME(muzzleFlash);

// var
DEFINE_STATIC_NAME(projectileSpeed);
DEFINE_STATIC_NAME(shootingForcedAtTarget);
DEFINE_STATIC_NAME(shootingMaxAllowedOffTargetAngle);
DEFINE_STATIC_NAME(shootingMaxAllowedOffTargetVerticalAngle); // requires shootingMaxAllowedOffTargetAngle to be set too!

//

void TeaForGodEmperor::AI::Logics::Tasks::Functions::trigger_combat_auto_music(Framework::RelativeToPresencePlacement const& enemyPlacement)
{
	if (enemyPlacement.get_target() &&
		Framework::GameUtils::is_controlled_by_player(enemyPlacement.get_target()))
	{
		if (enemyPlacement.get_straight_length() > 10.0f)
		{
			MusicPlayer::request_combat_auto_bump_low();
		}
		else
		{
			MusicPlayer::request_combat_auto();
		}
	}
}

bool TeaForGodEmperor::AI::Logics::Tasks::Functions::perform_shoot(::Framework::AI::MindInstance* mind, int shootIndex,
	Framework::RelativeToPresencePlacement const & enemyPlacement, Vector3 const & enemyTargetingOffsetOS,
	Optional<float> const & _projectileSpeed, Optional<bool> const& _ignoreViolenceDisallowed,
	Array<NPCBase::ShotInfo> const* _useShotInfos)
{
	if (GameDirector::is_violence_disallowed() && !_ignoreViolenceDisallowed.get(false))
	{
		return false;
	}

	if (shootIndex == 0)
	{
		Functions::trigger_combat_auto_music(enemyPlacement);
	}

	auto* imo = mind->get_owner_as_modules_owner();

	auto* npcBase = fast_cast<NPCBase>(mind->get_logic());

	NPCBase::ShotInfo shotInfo(NAME(shoot), NAME(muzzleFlash));
	if (_useShotInfos)
	{
		shotInfo = (*_useShotInfos)[mod(shootIndex, _useShotInfos->get_size())];
	}
	else if (npcBase)
	{
		shotInfo = npcBase->get_shot_infos()[mod(shootIndex, npcBase->get_shot_infos().get_size())];
	}

	//ai_log(mind->get_logic(), TXT("!@# shoot %i"), shootIndex);
	if (shootIndex == 0)
	{
		//ai_log(mind->get_logic(), TXT("!@# first shoot"));
		// issue only when shooting at the actual enemy
		if (auto* enemy = enemyPlacement.get_target())
		{
			//ai_log(mind->get_logic(), TXT("!@# there is enemy"));
			AI::Logics::issue_perception_alert_ai_message(imo, enemy, enemyPlacement);
		}
	}

	bool shot = false;
	if (auto* tos = imo->get_temporary_objects())
	{
		bool shootingForcedAtTarget = imo->get_variables().get_value<bool>(NAME(shootingForcedAtTarget), false);
		float shootingMaxAllowedOffTargetAngle = imo->get_variables().get_value<float>(NAME(shootingMaxAllowedOffTargetAngle), -1.0f);
		float shootingMaxAllowedOffTargetVerticalAngle = imo->get_variables().get_value<float>(NAME(shootingMaxAllowedOffTargetVerticalAngle), -1.0f);
		Optional<Transform> projectileWS;
		if (shootingForcedAtTarget || shootingMaxAllowedOffTargetAngle >= 0.0f)
			// no shootingMaxAllowedOffTargetVerticalAngle as requires shootingMaxAllowedOffTargetAngle
		{
			Vector3 enemyLocationWS = enemyPlacement.get_placement_in_owner_room().location_to_world(enemyTargetingOffsetOS);

			if (auto* pi = tos->get_info_for(shotInfo.projectile))
			{
				Name useSocket = shotInfo.socket.get(pi->socket);
				if (useSocket.is_valid())
				{
					if (auto* ap = imo->get_appearance())
					{
						Transform socketOS = ap->calculate_socket_os(useSocket);
						projectileWS = imo->get_presence()->get_placement().to_world(socketOS);
					}
				}
				else
				{
					todo_important(TXT("implement other cases"));
					error(TXT("not shooting as has no projectile placement"));
					return false;
				}

				if (shootingMaxAllowedOffTargetAngle >= 0.0f && projectileWS.is_set())
				{
					if (!enemyPlacement.is_active())
					{
						return false;
					}
					enemyLocationWS = enemyPlacement.get_placement_in_owner_room().location_to_world(enemyTargetingOffsetOS);
					if (npcBase)
					{
						enemyLocationWS = npcBase->access_predict_target_placement().predict(enemyLocationWS, imo, enemyPlacement);
					}
					
					struct ClosestPointOnEnemy
					{
						static Vector3 find(Transform const& projectileWS, Vector3 const& enemyLocationWS, Framework::RelativeToPresencePlacement const& enemyPlacement)
						{
							Vector3 enemyLowWS = enemyPlacement.get_placement_in_owner_room().get_translation();
							Segment enemySeg(enemyLowWS, enemyLocationWS);
							float t = enemySeg.get_closest_t(Segment(projectileWS.get_translation(), projectileWS.location_to_world(Vector3::yAxis * 1000.0f)));
							return enemySeg.get_at_t(t);
						}
					};
					if (shootingMaxAllowedOffTargetVerticalAngle >= 0.0f)
					{
						// check if we can aim right at the target
						Vector3 enemyLocationLS = projectileWS.get().location_to_local(enemyLocationWS);
						Rotator3 atEnemyRotationLS = enemyLocationLS.to_rotator();
						if (abs(atEnemyRotationLS.yaw) > shootingMaxAllowedOffTargetAngle ||
							abs(atEnemyRotationLS.pitch) > shootingMaxAllowedOffTargetVerticalAngle)
						{
							// no, try to aim at the closest point
							// find closest point at the enemy (from ground)
							enemyLocationWS = ClosestPointOnEnemy::find(projectileWS.get(), enemyLocationWS, enemyPlacement);
							Vector3 enemyLocationLS = projectileWS.get().location_to_local(enemyLocationWS);
							Rotator3 atEnemyRotationLS = enemyLocationLS.to_rotator();
							if (abs(atEnemyRotationLS.yaw) > shootingMaxAllowedOffTargetAngle ||
								abs(atEnemyRotationLS.pitch) > shootingMaxAllowedOffTargetVerticalAngle)
							{
								return false;
							}
						}
					}
					else
					{
						float atTargetThreshold = cos_deg(shootingMaxAllowedOffTargetAngle);
						float actualAtTarget = Vector3::dot(projectileWS.get().get_axis(Axis::Forward), (enemyLocationWS - projectileWS.get().get_translation()).normal());
						if (actualAtTarget < atTargetThreshold)
						{
							enemyLocationWS = ClosestPointOnEnemy::find(projectileWS.get(), enemyLocationWS, enemyPlacement);
							actualAtTarget = Vector3::dot(projectileWS.get().get_axis(Axis::Forward), (enemyLocationWS - projectileWS.get().get_translation()).normal());
							if (actualAtTarget < atTargetThreshold)
							{
								return false;
							}
						}
					}
				}
			}

			if (shootingForcedAtTarget)
			{
				if (projectileWS.is_set())
				{
					projectileWS = look_at_matrix(projectileWS.get().get_translation(), enemyLocationWS, projectileWS.get().get_axis(Axis::Up)).to_transform();
				}
				else
				{
					error(TXT("not shooting as has no projectile placement"));
					return false;
				}
			}
		}
		Framework::ModuleTemporaryObjects::SpawnParams tempSP;
		tempSP.at_socket(shotInfo.socket);
		auto* projectile = tos->spawn(shotInfo.projectile, tempSP);
		if (projectile)
		{
			if (projectileWS.is_set())
			{
				Framework::Room* room = imo->get_presence()->get_in_room();
				Transform pWS = projectileWS.get();
				room->move_through_doors(imo->get_presence()->get_centre_of_presence_transform_WS(), pWS, room);
				projectile->on_activate_place_in(room, pWS);
			}

			// just in any case if we would be shooting from inside of a capsule
			if (auto* collision = projectile->get_collision())
			{
				collision->dont_collide_with_up_to_top_instigator(imo, 0.5f);
			}
			float useProjectileSpeed = _projectileSpeed.get(0.0f);
			if (useProjectileSpeed == 0.0f)
			{
				useProjectileSpeed = projectile->get_variables().get_value<float>(NAME(projectileSpeed), 15.0f);
				useProjectileSpeed = imo->get_variables().get_value<float>(NAME(projectileSpeed), useProjectileSpeed);
			}
			if (npcBase)
			{
				npcBase->access_predict_target_placement().set_projectile_speed(useProjectileSpeed);
			}
			Vector3 velocity = Vector3(0.0f, useProjectileSpeed, 0.0f);
			velocity = Utils::apply_shooting_accuracy(velocity, imo, enemyPlacement.get_target(), enemyPlacement.calculate_string_pulled_distance());
			projectile->on_activate_set_relative_velocity(velocity);
			shot = true;
		}
		if (shot)
		{
			tos->spawn_all(shotInfo.muzzleFlash, tempSP);
		}
	}
	if (shot)
	{
		if (auto* s = imo->get_sound())
		{
			s->play_sound(shotInfo.sound, shotInfo.socket);
		}
	}

	return shot;
}


LATENT_FUNCTION(TeaForGodEmperor::AI::Logics::Tasks::shoot)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("shoot"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_PARAM_OPTIONAL(int, shootCount, 1);
	LATENT_PARAM_OPTIONAL(float, shootingInterval, 0.3f);
	LATENT_PARAM_OPTIONAL(float, projectileSpeed, 0.0f); // if zero/not provided will use further one
	LATENT_PARAM_OPTIONAL(Array<NPCBase::ShotInfo> const *, useShotInfos, nullptr); // if we want to use something else
	LATENT_PARAM_OPTIONAL(bool, ignoreAbilityToShoot, false); // ignores if unable to shoot
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(bool, unableToShoot);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(Vector3, enemyTargetingOffsetOS);
	LATENT_COMMON_VAR(bool, ignoreViolenceDisallowed);
	LATENT_COMMON_VAR(bool, confused);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(int, shootIndex);
	LATENT_VAR(bool, shootNow);

	auto* npcBase = fast_cast<NPCBase>(mind->get_logic());

	LATENT_BEGIN_CODE();

	shootNow = true;
	if (npcBase && npcBase->get_max_attack_distance().is_set())
	{
		auto* imo = mind->get_owner_as_modules_owner();
		if (enemyPlacement.calculate_string_pulled_distance(imo->get_presence()->get_centre_of_presence_os(), Transform(enemyTargetingOffsetOS, Quat::identity)) > npcBase->get_max_attack_distance().get())
		{
			shootNow = false;
		}
	}

	if (shootNow)
	{
		for (shootIndex = 0;
			 shootIndex < shootCount && ! confused;
			 ++shootIndex)
		{
			if (ignoreAbilityToShoot || !unableToShoot)
			{
				Functions::perform_shoot(mind, shootIndex, enemyPlacement, enemyTargetingOffsetOS, projectileSpeed, ignoreViolenceDisallowed, useShotInfos);
				LATENT_WAIT(shootingInterval);
			}
		}
	}

	LATENT_ON_BREAK();
	//
	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

