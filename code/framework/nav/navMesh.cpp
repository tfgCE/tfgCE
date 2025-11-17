#include "navMesh.h"

#include "navNode.h"
#include "navSystem.h"

#include "..\game\game.h"

#include "..\..\core\utils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace Nav;

//

Nav::Mesh::Mesh(Room* _inRoom, Name const & _id)
: id(_id)
, inRoom(_inRoom)
{
}

Nav::Mesh::~Mesh()
{
	clear();
}

void Nav::Mesh::add(Node* _node)
{
	an_assert(Game::get() && Game::get()->get_nav_system()->is_in_writing_mode());
	an_assert(_node->belongsTo == nullptr);
	nodes.push_back(NodePtr(_node));
	_node->belongsTo = this;
}

void Nav::Mesh::remove(Node* _node)
{
	an_assert(Game::get() && Game::get()->get_nav_system()->is_in_writing_mode());
	an_assert(_node->belongsTo == this);
	_node->belongsTo = nullptr;
	if (!_node->is_outdated())
	{
		_node->outdate();
	}
	_node->remove_outdated(); // will remove all connections, effectively disassemble node

#ifdef AN_DEVELOPMENT
	bool wasAmongNodes = false;
#endif
	for_every_ref(node, nodes)
	{
		if (node == _node)
		{
#ifdef AN_DEVELOPMENT
			wasAmongNodes = true;
#endif
			nodes.remove_at(for_everys_index(node));
			break;
		}
	}
#ifdef AN_DEVELOPMENT
	an_assert(wasAmongNodes);
#endif
}

void Nav::Mesh::clear()
{
	scoped_call_stack_info(TXT("Nav::Mesh::clear"));

	an_assert(Game::get() && Game::get()->get_nav_system()->is_in_writing_mode());
	for_every_ref(node, nodes)
	{
		node->outdate();
		node->remove_outdated(); // will remove all connections, effectively disassemble node
		node->belongsTo = nullptr;
	}
	nodes.clear();
}

void Nav::Mesh::unify_open_nodes()
{
	scoped_call_stack_info(TXT("Nav::Mesh::unify_open_nodes"));

	an_assert(Game::get() && Game::get()->get_nav_system()->is_in_writing_mode());

	for_every_ref(node, nodes)
	{
		if (!node->is_outdated() &&
			node->is_open_node())
		{
			node->unify_as_open_node();
		}
	}
}

void Nav::Mesh::connect_open_nodes()
{
	scoped_call_stack_info(TXT("Nav::Mesh::connect_open_nodes"));

	an_assert(Game::get() && Game::get()->get_nav_system()->is_in_writing_mode());

	for_every_ref(node, nodes)
	{
		if (!node->is_outdated() &&
			node->is_open_node())
		{
			node->connect_as_open_node();
		}
	}
}

void Nav::Mesh::outdate()
{
	scoped_call_stack_info(TXT("Nav::Mesh::outdate"));

	inRoom = nullptr;

	for_every_ref(node, nodes)
	{
		node->outdate();
	}
}

void Nav::Mesh::debug_draw()
{
	todo_note(TXT("this may break when nav mesh is modified, just don't use it then?"));

	for_every_ref(node, nodes)
	{
		node->debug_draw();
	}
}

void Nav::Mesh::assign_groups()
{
	scoped_call_stack_info(TXT("Nav::Mesh::assign_groups"));

	an_assert(Game::get() && Game::get()->get_nav_system()->is_in_writing_mode());

	groupCount = 0;

	// reset first
	for_every_ref(node, nodes)
	{
		node->groupId = NONE;
	}

	// assign, we will get group count updated
	for_every_ref(node, nodes)
	{
		node->auto_assign_group(REF_ groupCount);
	}
}

void Nav::Mesh::outdate_irrelevant()
{
	scoped_call_stack_info(TXT("Nav::Mesh::outdate_irrelevant"));

	an_assert(Game::get() && Game::get()->get_nav_system()->is_in_writing_mode());

	while (true)
	{
		bool anythingOutdated = false;

		for (int i = 0; i < nodes.get_size(); ++i)
		{
			if (! nodes[i]->is_outdated())
			{
				nodes[i]->outdate_if_irrelevant();
				if (nodes[i]->is_outdated())
				{
					anythingOutdated = true;
				}
			}
		}

		if (!anythingOutdated)
		{
			break;
		}
	}
}

void Nav::Mesh::remove_outdated()
{
	scoped_call_stack_info(TXT("Nav::Mesh::remove_outdated"));

	an_assert(Game::get() && Game::get()->get_nav_system()->is_in_writing_mode());

	for (int i = 0; i < nodes.get_size(); ++i)
	{
		if (nodes[i]->is_outdated())
		{
			remove(nodes[i].get());
			--i;
		}
		else
		{
			nodes[i]->remove_outdated();
		}
	}
}

PlacementAtNode Nav::Mesh::find_location(Transform const & _placement, OUT_ float * _dist, Optional<int> const & _navMeshKeepGroup) const
{
	float bestDist = 1000.0f;
	Node* best = nullptr;
	Transform bestPlacement = Transform::identity;

	for_every_ref(node, nodes)
	{
		float dist = bestDist;
		Vector3 locationAt = _placement.get_translation();
		if ((! _navMeshKeepGroup.is_set() || node->get_group() == _navMeshKeepGroup.get()) &&
			node->find_location(_placement.get_translation(), OUT_ locationAt, REF_ dist))
		{
			if (!best || dist < bestDist)
			{
				bestDist = dist;
				best = node;
				bestPlacement = _placement;
				bestPlacement.set_translation(locationAt);
			}
		}
	}

	PlacementAtNode result;
	result.node = best;
	if (best)
	{
		result.placementLS = best->get_current_placement().to_local(bestPlacement);
		assign_optional_out_param(_dist, bestDist);
	}
	return result;
}

bool Nav::Mesh::is_transport_route(int _groupId) const
{
	int doorCount = 0;
	for_every_ref(node, nodes)
	{
		if (node->get_group() == _groupId &&
			node->get_door_in_room())
		{
			++doorCount;
			if (doorCount > 2)
			{
				return false;
			}
		}
	}

	return doorCount == 2;
}
