#include "navPlacementAtNode.h"

#include "navNode.h"

#ifdef AN_LOG_NAV_TASKS
#include "..\..\core\other\parserUtils.h"
#endif

using namespace Framework;
using namespace Nav;

//

PlacementAtNode PlacementAtNode::s_invalid;

PlacementAtNode::PlacementAtNode()
{
}

PlacementAtNode::PlacementAtNode(Node* _node)
: node(_node)
{
}

bool PlacementAtNode::is_valid() const
{
	return node.is_set() && ! node->is_outdated();
}

void PlacementAtNode::clear()
{
	placementLS.clear();
	node.clear();
}

void PlacementAtNode::set_location(Vector3 const& _locWS)
{
	Transform currentPlacement = is_valid() ? node->get_current_placement() : Transform::identity;
	Transform placement = currentPlacement;
	placement.set_translation(_locWS);
	placementLS = currentPlacement.to_local(placement);
}

void PlacementAtNode::set_placement(Optional<Transform> const & _placement)
{
	if (_placement.is_set())
	{
		Transform currentPlacement = is_valid() ? node->get_current_placement() : Transform::identity;
		placementLS = currentPlacement.to_local(_placement.get());
	}
	else
	{
		placementLS.clear();
	}
}

Transform PlacementAtNode::get_current_placement() const
{
	Transform currentPlacement = get_current_node_placement();
	if (placementLS.is_set())
	{
		currentPlacement = currentPlacement.to_world(placementLS.get());
	}
	return currentPlacement;
}

Transform PlacementAtNode::get_current_node_placement() const
{
	Transform currentPlacement = is_valid() ? node->get_current_placement() : Transform::identity;
	return currentPlacement;
}

Room* PlacementAtNode::get_room() const
{
	return is_valid() ? node->get_room() : nullptr;
}

void PlacementAtNode::set(Node* _node)
{
	placementLS.clear();
	node = _node;
}

#ifdef AN_LOG_NAV_TASKS
void PlacementAtNode::log(LogInfoContext& _log) const
{
	if (node.is_set())
	{
#ifdef AN_64
		_log.log(TXT("node %S"), ParserUtils::uint64_to_hex((uint64)node.get()).to_char());
#else
		_log.log(TXT("node %S"), ParserUtils::uint_to_hex((uint)node.get()).to_char());
#endif
	}
	else
	{
		_log.log(TXT("no node"));
	}
	if (placementLS.is_set())
	{
		_log.log(TXT("placementLS: %S"), placementLS.get().get_translation().to_string().to_char());
	}
	else
	{
		_log.log(TXT("no placementLS"));
	}
}
#endif
