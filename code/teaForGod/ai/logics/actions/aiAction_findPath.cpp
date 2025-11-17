#include "aiAction_findPath.h"

#include "..\aiLogic_npcBase.h"

#include "..\..\aiCommonVariables.h"
#include "..\..\aiRayCasts.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\nav\navTask.h"
#include "..\..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\..\framework\world\presenceLink.h"

#include "..\..\..\..\core\collision\iCollidableShape.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

bool Actions::find_path(::Latent::Frame& _frame, Framework::AI::MindInstance* mind, ::Framework::Nav::TaskPtr & pathTask, SafePtr<Framework::IModulesOwner> & enemy,
	Framework::RelativeToPresencePlacement & enemyPlacement, bool perceptionSightImpaired,
	std::function<void(REF_ Vector3 & goToLoc)> _on_try_to_approach)
{
	if (pathTask.is_set())
	{
		pathTask->cancel();
		pathTask.clear();
	}

	if (!enemyPlacement.is_active())
	{
		return false;
	}

	auto* imo = mind->get_owner_as_modules_owner();
	auto* npcBase = fast_cast<NPCBase>(mind->get_logic());

	auto& locomotion = mind->access_locomotion();
	if (locomotion.is_done_with_move() || locomotion.has_finished_move(1.0f))
	{
		if (enemyPlacement.get_target())
		{
			// sees enemy
			LATENT_LOG(TXT("I see enemy"));
		}
		else
		{
			// follow enemy
			LATENT_LOG(TXT("not seeing enemy"));
		}

		Framework::Nav::PlacementAtNode ourNavLoc = mind->access_navigation().find_nav_location();
		Framework::Nav::PlacementAtNode toEnemyNavLoc = Framework::Nav::PlacementAtNode::invalid();
		
		if (enemyPlacement.get_through_doors().is_empty())
		{
			// same room
			LATENT_LOG(TXT("same room - check actual location"));
			toEnemyNavLoc = mind->access_navigation().find_nav_location(enemyPlacement.get_in_final_room(), enemyPlacement.get_placement_in_final_room());
		}
		else
		{
			// get door through which enemy is available
			if (auto* dir = enemyPlacement.get_through_doors().get_first().get())
			{
				if (auto* nav = dir->get_nav_door_node().get())
				{
					LATENT_LOG(TXT("different room, consider first door from the chain/path"));
					toEnemyNavLoc.set(nav);
				}
				else
				{
					LATENT_LOG(TXT("different room, get close to first door although can't get through"));
					Transform dirPlacement = dir->get_placement();
					Transform inFront(Vector3::yAxis * -0.3f, Quat::identity);
					toEnemyNavLoc = mind->access_navigation().find_nav_location(dir->get_in_room(), dirPlacement.to_world(inFront));
				}
			}
		}

		if (ourNavLoc.is_valid() && toEnemyNavLoc.is_valid())
		{
			LATENT_LOG(TXT("we know where enemy is on navmesh"));
			Framework::Room* goToRoom = imo->get_presence()->get_in_room();
			Framework::DoorInRoom* goThroughDoor = nullptr;
			Vector3 goToLoc = toEnemyNavLoc.get_current_placement().get_translation();
			Vector3 startLoc = imo->get_presence()->get_placement().get_translation();

			bool tryToApproach = false;

			// check if door-through or enemy belong to the same nav group that we are and decide whether to follow to enemy or stay where we are
			// if there's door-through, toEnemyNavLoc is chosen the door it leads through, that's why we end up here with the same group
			if (ourNavLoc.node.get()->get_group() == toEnemyNavLoc.node.get()->get_group())
			{
				LATENT_LOG(TXT("same nav group"));
				// same group
				if (enemyPlacement.get_through_doors().is_empty())
				{
					an_assert(imo->get_appearance());
					if (enemyPlacement.get_target() ||
						(!perceptionSightImpaired &&
							!check_clear_ray_cast(CastInfo(), imo->get_presence()->get_placement().to_world(imo->get_appearance()->calculate_socket_os(npcBase->get_perception_socket_index())).get_translation(), imo,
								enemy.get(),
								enemyPlacement.get_in_final_room(), enemyPlacement.get_placement_in_owner_room())))
					{
						// is within same room we see enemy or we do not see enemy's location
						LATENT_LOG(enemyPlacement.get_target() ? TXT("approach enemy!") : TXT("approach to see if enemy is there"));
						tryToApproach = true;
						goToRoom = enemyPlacement.get_in_final_room();
						goToLoc = enemyPlacement.get_placement_in_final_room().get_translation();
					}
					else
					{
						LATENT_LOG(TXT("get closer to enemy"));
						// keep "goTo"s
						if ((goToLoc - startLoc).length() < 1.5f)
						{
							// lost
							return false;
						}
					}
				}
				else
				{
					if (enemyPlacement.get_target())
					{
						// we see enemy, approach him wherever he is
						LATENT_LOG(TXT("we see enemy, go to the enemy's room"));
						tryToApproach = true;
						goToRoom = enemyPlacement.get_in_final_room();
						goToLoc = enemyPlacement.get_placement_in_final_room().get_translation();
						startLoc = enemyPlacement.location_from_owner_to_target(startLoc);
					}
					else
					{
						LATENT_LOG(TXT("we do not see enemy, go to the door, on the other side"));
						// we don't see enemy and enemy is in different room, just go to the door and see what will happen
						goThroughDoor = enemyPlacement.get_through_doors().get_first().get();
					}
				}
			}
			else
			{
				LATENT_LOG(TXT("different nav group, try to approach if possible"));
				// remain here, within this group, try to approach enemy
				tryToApproach = true;
				// keep "goTo"s
				todo_note(TXT("check chances if maybe we should try to find a way to the enemy, break this function then and request following enemy"));
			}

			Framework::Nav::PathRequestInfo pathRequestInfo(imo);
			pathRequestInfo.with_dev_info(TXT("aiAction_findPath"));
			if (!Mobility::may_leave_room_when_attacking(imo, imo->get_presence()->get_in_room()))
			{
				pathRequestInfo.within_same_room_and_navigation_group();
			}

			if (goThroughDoor)
			{
				LATENT_LOG(TXT("find path through door"));
				pathTask = mind->access_navigation().find_path_through(goThroughDoor, 0.5f, pathRequestInfo);
			}
			else
			{
				LATENT_LOG(TXT("find path to loc"));
				if (tryToApproach && _on_try_to_approach)
				{
					_on_try_to_approach(REF_ goToLoc);
				}
				if (!enemyPlacement.get_target() &&
					goToRoom == imo->get_presence()->get_in_room() &&
					imo->get_presence()->calculate_flat_distance_for_nav(goToLoc) < 0.3f)
				{
					LATENT_LOG(TXT("we're almost at the point but there's no enemy, maybe we should drop this idea?"));
					return false;
				}
				pathTask = mind->access_navigation().find_path_to(goToRoom, goToLoc, pathRequestInfo);
			}
		}
	}
	return true;
}
