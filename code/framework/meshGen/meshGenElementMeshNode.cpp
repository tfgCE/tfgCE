#include "meshGenElementMeshNode.h"

#include "meshGenGenerationContext.h"
#include "meshGenUtils.h"

#include "..\appearance\meshNode.h"

#include "..\..\core\io\xml.h"

using namespace Framework;
using namespace MeshGeneration;

//

REGISTER_FOR_FAST_CAST(ElementMeshNode);

ElementMeshNode::ElementMeshNode()
{
	updateWMPPhase = UpdateWMPPostProcess; // first add mesh node as wmp may alter mesh node variables (meshNodeStore) but we also depend on copying vars that were read empty
}

ElementMeshNode::~ElementMeshNode()
{
}

bool ElementMeshNode::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	meshNodeName.load_from_xml(_node, TXT("name"));
	meshNodeName.load_from_xml(_node, TXT("id")); // deprecated?
	meshNodeTags.load_from_xml_attribute_or_child_node(_node, TXT("tags"));
	
	result &= meshNodeVariables.load_from_xml_child_node(_node, TXT("variables"));

	return result;
}

bool ElementMeshNode::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool ElementMeshNode::process(GenerationContext & _context, ElementInstance & _instance) const
{
	MeshNodePtr meshNode(new MeshNode());
	meshNode->name = meshNodeName.is_set()? meshNodeName.get(_context) : Name::invalid();
	meshNode->tags = meshNodeTags;
	if (!meshNodeVariables.is_empty())
	{
		meshNode->variables = meshNodeVariables;
	}
	_context.access_mesh_nodes().push_back(meshNode);
	return true;
}

bool ElementMeshNode::post_process(GenerationContext & _context, ElementInstance & _instance) const
{
	if (!_context.access_mesh_nodes().is_empty())
	{
		if (auto* meshNode = _context.access_mesh_nodes().get_last().get())
		{
			meshNode->variables.do_for_every([meshNode,&_context](SimpleVariableInfo& _info)
			{
				// override_ only if has default/initial value
				if (_info.was_read_empty())
				{
					if (_context.has_parameter(_info.get_name(), _info.type_id()))
					{
						_context.get_parameter(_info.get_name(), _info.type_id(), _info.access_raw());
						meshNode->variables.mark_read_empty(_info.get_name(), _info.type_id(), false);
					}
					else
					{
						meshNode->variables.invalidate(_info.get_name(), _info.type_id());
					}
				}
			});
		}
	}
	return true;
}
