#include "meshGenModifierSelection.h"

//

using namespace Framework;
using namespace MeshGeneration;
using namespace Modifiers;

//

bool Selection::is_empty() const
{
	return boxes.is_empty();
}

bool Selection::is_selected(Vector3 const& _at) const
{
	for_every(box, boxes)
	{
		if (box->is_inside(_at))
		{
			return true;
		}
	}
	return is_empty();
}

bool Selection::load_from_child_node_xml(IO::XML::Node const* _node, tchar const * _childNode)
{
	bool result = true;
	
	boxes.clear();
	for_every(chNode, _node->children_named(_childNode))
	{
		for_every(node, chNode->all_children())
		{
			if (node->get_name() == TXT("box"))
			{
				Box box;
				if (box.load_from_xml(node))
				{
					boxes.push_back(box);
				}
				else
				{
					error_loading_xml(node, TXT("problem loading box"));
					result = false;
				}
			}
		}
	}

	return result;
}
