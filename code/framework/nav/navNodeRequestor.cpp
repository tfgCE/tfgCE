#include "navNodeRequestor.h"

#include "navNode.h"

using namespace Framework;
using namespace Nav;

//

NodeRequestor::NodeRequestor(IModulesOwner* _owner)
: owner(_owner)
{
}

NodeRequestor::~NodeRequestor()
{
	clear();
}

void NodeRequestor::clear()
{
	if (node.is_set() && owner)
	{
		node->remove_requestor(this);
	}
	node.clear();
}

void NodeRequestor::request(Node* _node)
{
	if (node == _node)
	{
		return;
	}
	clear();
	node = _node;
	if (node.is_set() && owner)
	{
		node->add_requestor(this);
	}
}