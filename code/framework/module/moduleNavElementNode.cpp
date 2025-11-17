#include "moduleNavElementNode.h"

#include "modules.h"

#include "..\nav\navMesh.h"
#include "..\nav\nodes\navNode_Point.h"

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(ModuleNavElementNode);

static ModuleNavElement* create_module(IModulesOwner* _owner)
{
	return new ModuleNavElementNode(_owner);
}

RegisteredModule<ModuleNavElement> & ModuleNavElementNode::register_itself()
{
	return Modules::navElement.register_module(String(TXT("node")), create_module);
}

ModuleNavElementNode::ModuleNavElementNode(IModulesOwner* _owner)
: ModuleNavElement(_owner)
{
}

void ModuleNavElementNode::add_to(Nav::Mesh* _navMesh)
{
	base::add_to(_navMesh);

	node = Nav::Nodes::Point::get_one();
	node->be_open_node();
	node->place_WS(get_owner()->get_presence()->get_placement(), get_owner());
	_navMesh->add(node.get());
	node->set_flags_from_nav_mesh();
}
