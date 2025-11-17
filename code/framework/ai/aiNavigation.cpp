#include "aiNavigation.h"

#include "aiMindInstance.h"

#include "..\game\game.h"
#include "..\modulesOwner\modulesOwner.h"
#include "..\module\moduleCollision.h"
#include "..\module\modulePresence.h"
#include "..\nav\navMesh.h"
#include "..\nav\navNode.h"
#include "..\nav\navPath.h"
#include "..\nav\navSystem.h"
#include "..\nav\tasks\navTask_FindPath.h"
#include "..\nav\tasks\navTask_GetRandomLocation.h"
#include "..\nav\tasks\navTask_GetRandomPath.h"
#include "..\world\doorInRoom.h"
#include "..\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AI;

//

// room tags
DEFINE_STATIC_NAME(notTransportRoute);

//

Navigation::Navigation(MindInstance* _mind)
: mind(_mind)
{
}

Navigation::~Navigation()
{
}

bool Navigation::is_at_transport_route() const
{
	an_assert(mind->get_owner_as_modules_owner());
	if (auto* presence = mind->get_owner_as_modules_owner()->get_presence())
	{
		if (auto* inRoom = presence->get_in_room())
		{
			if (!inRoom->get_tags().get_tag(NAME(notTransportRoute)))
			{
				auto atNode = find_nav_location(inRoom, presence->get_placement());
				if (atNode.is_valid())
				{
					int atGroupId = atNode.node->get_group();

					if (auto* nav = atNode.node->get_nav_mesh())
					{
						return nav->is_transport_route(atGroupId);
					}
				}
			}
		}
	}
	return false;
}

Nav::PlacementAtNode Navigation::find_nav_location() const
{
	an_assert(mind->get_owner_as_modules_owner());
	if (auto * presence = mind->get_owner_as_modules_owner()->get_presence())
	{
		return find_nav_location(presence->get_in_room(), presence->get_placement());
	}
	else
	{
		return Nav::PlacementAtNode::invalid();
	}
}

Nav::PlacementAtNode Navigation::find_nav_location(IModulesOwner* _of, Vector3 const & _relativeLocOS, Optional<int> const & _navMeshKeepGroup) const
{
	if (_of)
	{
		if (auto * presence = _of->get_presence())
		{
			if (auto* room = presence->get_in_room())
			{
				Transform placement = presence->get_placement().to_world(Transform(_relativeLocOS, Quat::identity));
				room->move_through_doors(presence->get_centre_of_presence_transform_WS(), placement, room);
				return find_nav_location(room, placement, _navMeshKeepGroup);
			}
		}
	}
	return Nav::PlacementAtNode::invalid();
}

Nav::PlacementAtNode Navigation::find_nav_location(IModulesOwner* _of, Optional<int> const & _navMeshKeepGroup) const
{
	return find_nav_location(_of, Vector3::zero, _navMeshKeepGroup);
}

Nav::PlacementAtNode Navigation::find_nav_location(Room* _room, Transform const & _placement, Optional<int> const & _navMeshKeepGroup) const
{
	if (_room)
	{
		todo_note(TXT("add support for navmesh id?"));
		return _room->find_nav_location(_placement, nullptr, Name::invalid(), _navMeshKeepGroup);
	}
	else
	{
		return Nav::PlacementAtNode::invalid();
	}
}

Nav::Task* Navigation::find_path_to(IModulesOwner* _to, Nav::PathRequestInfo const & _requestInfo) const
{
	Nav::PathRequestInfo requestInfo = _requestInfo;
	fill(requestInfo);
	Nav::PlacementAtNode currNavLoc = find_nav_location();
	Framework::Nav::Tasks::FindPath* task = Framework::Nav::Tasks::FindPath::new_task(
		currNavLoc,
		find_nav_location(_to, requestInfo.withinSameRoomAndNavigationGroup && currNavLoc.node.is_set()? Optional<int>(currNavLoc.node->get_group()) : NP),
		requestInfo);
	Game::get()->get_nav_system()->add(task);
	return task;
}

Nav::Task* Navigation::find_path_to(Room* _room, Vector3 const & _where, Nav::PathRequestInfo const & _requestInfo) const
{
	Nav::PathRequestInfo requestInfo = _requestInfo;
	fill(requestInfo);
	Nav::PlacementAtNode currNavLoc = find_nav_location();
	Framework::Nav::Tasks::FindPath* task = Framework::Nav::Tasks::FindPath::new_task(
		currNavLoc,
		find_nav_location(_room, Transform(_where, Quat::identity), requestInfo.withinSameRoomAndNavigationGroup && currNavLoc.node.is_set() ? Optional<int>(currNavLoc.node->get_group()) : NP),
		requestInfo);
	Game::get()->get_nav_system()->add(task);
	return task;
}

Nav::Task* Navigation::find_path_to(Nav::PlacementAtNode const & _to, Nav::PathRequestInfo const & _requestInfo) const
{
	Nav::PathRequestInfo requestInfo = _requestInfo;
	fill(requestInfo);
	Nav::PlacementAtNode currNavLoc = find_nav_location();
	Nav::PlacementAtNode to;
	if (requestInfo.withinSameRoomAndNavigationGroup && currNavLoc.node.is_set() && _to.node.is_set() &&
		currNavLoc.node->get_group() != _to.node->get_group())
	{
		to = find_nav_location(_to.get_room(), _to.get_current_placement(), requestInfo.withinSameRoomAndNavigationGroup && currNavLoc.node.is_set() ? Optional<int>(currNavLoc.node->get_group()) : NP);
	}
	Framework::Nav::Tasks::FindPath* task = Framework::Nav::Tasks::FindPath::new_task(
		currNavLoc,
		to.is_valid()? to : _to,
		requestInfo);
	Game::get()->get_nav_system()->add(task);
	return task;
}

Nav::Task* Navigation::find_path_through(DoorInRoom* _dir, Optional<float> const & _distanceIn, Nav::PathRequestInfo const & _requestInfo) const
{
	Nav::PathRequestInfo requestInfo = _requestInfo;
	fill(requestInfo);
	requestInfo.transport_path();
	Nav::PlacementAtNode currNavLoc = find_nav_location();
	Framework::Nav::Tasks::FindPath* task = nullptr;
	if (_dir)
	{
		auto* oDir = _dir->get_door_on_other_side();
		if (oDir && !requestInfo.withinSameRoomAndNavigationGroup)
		{
			float distanceIn = 0.3f;
			if (_distanceIn.is_set())
			{
				distanceIn = _distanceIn.get();
			}
			else if (auto* imo = mind->get_owner_as_modules_owner())
			{
				if (auto* collision = imo->get_collision())
				{
					distanceIn = collision->get_radius_for_gradient();
				}
			}
			task = Framework::Nav::Tasks::FindPath::new_task(
				currNavLoc,
				find_nav_location(oDir->get_in_room(), oDir->get_inbound_matrix().to_transform().to_world(Transform(Vector3::yAxis * distanceIn, Quat::identity)),
					requestInfo.withinSameRoomAndNavigationGroup && currNavLoc.node.is_set() ? Optional<int>(currNavLoc.node->get_group()) : NP),
					requestInfo);
		}
		else
		{
			task = Framework::Nav::Tasks::FindPath::new_task(
				currNavLoc,
				find_nav_location(_dir->get_in_room(), _dir->get_placement(),
					requestInfo.withinSameRoomAndNavigationGroup && currNavLoc.node.is_set() ? Optional<int>(currNavLoc.node->get_group()) : NP),
					requestInfo);
		}
		Game::get()->get_nav_system()->add(task);
	}
	return task;
}

Nav::Task* Navigation::get_random_location(float _distance) const
{
	Framework::Nav::Tasks::GetRandomLocation* task = Framework::Nav::Tasks::GetRandomLocation::new_task(
		find_nav_location(),
		_distance);
	Game::get()->get_nav_system()->add(task);
	return task;
}

Nav::Task* Navigation::get_random_path(float _distance, Optional<Vector3> const & _startInDir, Nav::PathRequestInfo const & _requestInfo) const
{
	Nav::PathRequestInfo requestInfo = _requestInfo;
	fill(requestInfo);
	Framework::Nav::Tasks::GetRandomPath* task = Framework::Nav::Tasks::GetRandomPath::new_task(
		find_nav_location(),
		_distance,
		_startInDir,
		requestInfo);
	Game::get()->get_nav_system()->add(task);
	return task;
}

void Navigation::fill(Nav::PathRequestInfo & _requestInfo) const
{
	if (mind)
	{
		if (!_requestInfo.useNavFlags.is_set() && mind->get_nav_flags().is_empty())
		{
			warn(TXT("you may want to provide navFlags for \"%S\""), mind->get_owner_as_modules_owner()? mind->get_owner_as_modules_owner()->ai_get_name().to_char() : TXT("??"));
		}
		_requestInfo.useNavFlags.set_if_not_set(mind->get_nav_flags());
	}
}
