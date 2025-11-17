#include "navNode.h"

#include "navMesh.h"
#include "navNodeRequestor.h"
#include "navSystem.h"

#include "..\game\game.h"
#include "..\modulesOwner\modulesOwner.h"
#include "..\module\moduleNavElement.h"
#include "..\module\modulePresence.h"
#include "..\world\doorInRoom.h"
#include "..\world\roomRegionVariables.inl"

#include "..\..\core\concurrency\scopedMRSWLock.h"
#include "..\..\core\debug\debugRenderer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace Nav;

//

DEFINE_STATIC_NAME(navFlags);

//

REGISTER_FOR_FAST_CAST(Node);

Node::Node()
{
}

Node::~Node()
{
	an_assert(belongsTo == nullptr);
}

void Node::reset_to_new()
{
	an_assert(belongsTo == nullptr);
	outdate();
	outdated = false;

	flags = Flags::none();

	groupId = NONE;
	openNode = false;
	openMoveToFind = Vector3::zero;

	// clear rest
	irrelevantOnSingleConnection = false;
#ifdef AN_DEVELOPMENT
	unified = false;
#endif
	blockedTemporarilyStop = false;
	placement = Transform::identity;
	onMoveableOwner = false;

	openNode = false;
	openMoveToFind = Vector3::zero;
}

void Node::outdate()
{
	if (outdated)
	{
		return;
	}
	outdated = true;
	for_every(connection, connections)
	{
		connection->outdate();
	}
	placementOwner.clear();
	be_door_in_room(nullptr);
}

void Node::be_door_in_room(DoorInRoom* _doorInRoom)
{
	if (doorInRoom)
	{
		doorInRoom->outdate_nav();
	}
	doorInRoom = _doorInRoom;
	if (doorInRoom)
	{
		doorInRoom->set_nav_group_id(groupId);
	}
}

void Node::place_WS(Transform const & _placement, IModulesOwner* _placementOwner, Optional<bool> const & _onMoveableOwner)
{
	Transform placementLS = _placement;
	if (_placementOwner && _placementOwner->get_presence())
	{
		placementLS = _placementOwner->get_presence()->get_placement_for_nav().to_local(placementLS);
	}
	place_LS(placementLS, _placementOwner, _onMoveableOwner);
}

void Node::place_LS(Transform const & _placement, IModulesOwner* _placementOwner, Optional<bool> const & _onMoveableOwner)
{
	placement = _placement;
	placementOwner = _placementOwner;
	if (_onMoveableOwner.is_set())
	{
		onMoveableOwner = _onMoveableOwner.get();
	}
	else
	{
		onMoveableOwner = false;
		if (_placementOwner)
		{
			if (auto * nav = _placementOwner->get_nav_element())
			{
				onMoveableOwner = nav->is_moveable();
			}
		}
	}
}

bool Node::is_connected_to(Node* _node) const
{
	for_every(connection, connections)
	{
		if (connection->to == _node)
		{
#ifdef AN_DEVELOPMENT
			bool connectedReverse = false;
			for_every(connection, _node->connections)
			{
				if (connection->to == this)
				{
					connectedReverse = true;
				}
			}
			an_assert(connectedReverse);
#endif
			return true;
		}
	}

	return false;
}

Node::Connection const* Node::get_connect_by_shared_data(ConnectionSharedData const* _csd) const
{
	for_every(connection, connections)
	{
		if (connection->data == _csd)
		{
			return connection;
		}
	}
	return nullptr;
}

Node::Connection * Node::access_connect_by_shared_data(ConnectionSharedData const* _csd)
{
	for_every(connection, connections)
	{
		if (connection->data == _csd)
		{
			return connection;
		}
	}
	return nullptr;
}

Node::ConnectionSharedData* Node::connect(Node* _node)
{
	an_assert(Game::get()->get_nav_system()->is_in_writing_mode());
	an_assert(!is_connected_to(_node));

	connections.push_back(Connection(_node));
	_node->connections.push_back(Connection(this));

	ConnectionSharedData* data = new ConnectionSharedData();
	connections.get_last().data = data;
	_node->connections.get_last().data = data;

	an_assert(are_connections_valid());

	return data;
}

void Node::disconnect(Node* _node)
{
	an_assert(Game::get()->get_nav_system()->is_in_writing_mode());

	NodePtr thisNode(this); // so we will stay in memory!

	for_every(connection, _node->connections)
	{
		if (connection->to == this)
		{
			_node->connections.remove_at(for_everys_index(connection));
			break;
		}
	}
	for_every(connection, connections)
	{
		if (connection->to == _node)
		{
			connections.remove_at(for_everys_index(connection));
			break;
		}
	}
}

void Node::unify_as_open_node()
{
	an_assert(false, TXT("maybe it shouldn't be a open node?"));
}

void Node::connect_as_open_node()
{
	an_assert(false, TXT("maybe it shouldn't be a open node?"));
}

Transform Node::get_current_placement() const
{
	Transform result = placement;
	if (auto* imo = placementOwner.get())
	{
		result = imo->get_presence()->get_placement_for_nav().to_world(result);
	}
	return result;
}

void Node::debug_draw()
{
#ifdef AN_DEBUG_RENDERER
	Transform currentPlacement = get_current_placement();

	if (is_outdated())
	{
		debug_push_transform(currentPlacement);
		Vector3 offset(0.1f, 0.0f, 0.1f);
		debug_draw_line(true, Colour::red, Vector3::zero, Vector3( offset.x,  offset.y, offset.z));
		debug_draw_line(true, Colour::red, Vector3::zero, Vector3(-offset.x, -offset.y, offset.z));
		debug_draw_line(true, Colour::red, Vector3::zero, Vector3( offset.y, -offset.x, offset.z));
		debug_draw_line(true, Colour::red, Vector3::zero, Vector3(-offset.y,  offset.x, offset.z));
		debug_pop_transform();
		return;
	}

	debug_push_transform(currentPlacement);
	Vector3 offset(0.1f, 0.0f, 0.2f);
	Colour colour = Colour::green;
	if (get_door_in_room())
	{
		offset.x *= 0.75f;
		offset.z *= 1.50f;
		colour = Colour::cyan;
	}
	if (!is_open_node())
	{
		colour *= 0.8f;
	}
	debug_draw_line(true, colour, Vector3::zero, Vector3( offset.x,  offset.y, offset.z));
	debug_draw_line(true, colour, Vector3::zero, Vector3(-offset.x, -offset.y, offset.z));
	debug_draw_line(true, colour, Vector3::zero, Vector3( offset.y, -offset.x, offset.z));
	debug_draw_line(true, colour, Vector3::zero, Vector3(-offset.y,  offset.x, offset.z));
#ifdef AN_DEVELOPMENT
	debug_draw_text(true, colour, Vector3::zero, NP, true, 0.1f, false, TXT("[%i] %S / %S"), groupId, name.to_char(), flags.to_string().to_char());
#else
	debug_draw_text(true, colour, Vector3::zero, NP, true, 0.1f, false, flags.to_string().to_char());
#endif
	/*
	// this may not work properly after building as we could have switched placements
	if (! openMoveToFind.is_zero())
	{
		debug_draw_arrow(true, Colour::blue, Vector3::zero, openMoveToFind);
	}*/
	debug_pop_transform();

	for_every(connection, connections)
	{
		if (!connection->to->is_outdated())
		{
			Colour connectionColour = Colour::green;
			if (connection->data->is_blocked_temporarily())
			{
				connectionColour = Colour::orange.with_alpha(0.333f);
			}
			Transform otherCurrentPlacement = connection->to->get_current_placement();
			debug_draw_line(true, connectionColour, currentPlacement.get_translation(), otherCurrentPlacement.get_translation());
		}
	}
#endif
}

void Node::auto_assign_group(REF_ int & _nextGroupId)
{
	if (groupId != NONE ||
		is_outdated())
	{
		return;
	}

	auto_assign_group_internal(_nextGroupId);

	++_nextGroupId;
}

void Node::auto_assign_group_internal(int _groupId)
{
	if (groupId != NONE)
	{
		an_assert(groupId == _groupId, TXT("if not, how come, we are connected?"));
		return;
	}

	groupId = _groupId;

	if (doorInRoom)
	{
		doorInRoom->set_nav_group_id(groupId);
	}

	for_every(connection, connections)
	{
		if (!connection->is_outdated())
		{
			connection->to->auto_assign_group_internal(_groupId);
		}
	}
}

void Node::remove_outdated()
{
	an_assert(Game::get() && Game::get()->get_nav_system()->is_in_writing_mode());

	for (int i = 0; i < connections.get_size(); ++i)
	{
		if (connections[i].is_outdated())
		{
			connections.remove_at(i);
			--i;
		}
	}
}

void Node::outdate_if_irrelevant()
{
	if (is_outdated())
	{
		return;
	}

	if (irrelevantOnSingleConnection)
	{
		an_assert(!doorInRoom, TXT("nodes with irrelevantOnSingleConnection should not have door in room associated, investigate why it was said so"));
		if (!doorInRoom &&
			calc_non_outdated_connections_count() == 1)
		{
			outdate();
		}
	}
}

Room* Node::get_room() const
{
	if (auto const * navMesh = get_nav_mesh())
	{
		return navMesh->get_room();
	}
	else
	{
		return nullptr;
	}
}

void Node::remove_requestor(NodeRequestor* _requestor)
{
	Concurrency::ScopedMRSWLockWrite lock(requestorsLock);
	requestors.remove_fast(_requestor);
}

void Node::add_requestor(NodeRequestor* _requestor)
{
	Concurrency::ScopedMRSWLockWrite lock(requestorsLock);
	an_assert(! requestors.does_contain(_requestor));
	requestors.push_back(_requestor);
}

void Node::do_for_all_requestors(std::function<void(IModulesOwner* _owner)> _do) const
{
	Concurrency::ScopedMRSWLockRead lock(requestorsLock);
	for_every_ptr(requestor, requestors)
	{
		if (auto* imo = requestor->get_owner())
		{
			_do(imo);
		}
	}
}

int Node::calc_non_outdated_connections_count() const
{
	int count = 0;
	for_every(connection, connections)
	{
		if (! connection->is_outdated())
		{
			++count;
		}
	}
	return count;
}

void Node::get_unified_values_from(Node const * _other)
{
	if (doorInRoom || _other->doorInRoom)
	{
		// can't make them irrelevant, with this, we could have holes in navmesh between rooms (as door in rooms most likely have always one connection)
		irrelevantOnSingleConnection = false;
	}
	else
	{
		irrelevantOnSingleConnection |= _other->irrelevantOnSingleConnection;
	}
}

void Node::set_flags_from_nav_mesh()
{
	if (auto* room = get_room())
	{
		flags.clear();
		room->on_each_variable_down<String>(NAME(navFlags), [this](String const & _stringFlags)
		{
			flags.apply(_stringFlags);
		});
	}
}