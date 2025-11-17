#include "meshGenElementDropMeshNodes.h"

#include "meshGenGenerationContext.h"
#include "meshGenMeshNodeContext.h"
#include "meshGenUtils.h"

#include "..\appearance\meshNode.h"

#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\io\xml.h"

using namespace Framework;
using namespace MeshGeneration;

//

REGISTER_FOR_FAST_CAST(ElementDropMeshNodes);

ElementDropMeshNodes::~ElementDropMeshNodes()
{
}

bool ElementDropMeshNodes::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("meshNode")))
	{
		DropMeshNodePtr fe(new DropMeshNode());
		if (fe->load_from_xml(node))
		{
			dropMeshNodes.push_back(fe);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool ElementDropMeshNodes::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool ElementDropMeshNodes::process(GenerationContext & _context, ElementInstance & _instance) const
{
	bool result = true;
	if (dropMeshNodes.is_empty())
	{
		for_every_ref(mn, _context.access_mesh_nodes())
		{
			mn->drop();
		}
	}
	for_every_ref(dmn, dropMeshNodes)
	{
		for_every_ref(mn, _context.access_mesh_nodes())
		{
			if (mn->is_dropped())
			{
				continue;
			}
			bool drop = false;
			if (dmn->toolSet.is_empty())
			{
				drop = true;
			}
			else
			{
				MeshNodeContext mnContext(_context, _instance, mn);
				WheresMyPoint::Context wmpContext(&mnContext);
				wmpContext.set_random_generator(_instance.access_context().access_random_generator());

				WheresMyPoint::Value value;
				dmn->toolSet.update(value, wmpContext);

				if (value.get_type() != type_id<bool>())
				{
					error_generating_mesh(_instance, TXT("expecting bool, got %S"), RegisteredType::get_name_of(value.get_type()));
					result = false;
				}
				else
				{
					drop = value.get_as<bool>();
				}
			}

			if (drop)
			{
				mn->drop();
			}
		}
	}
	return result;
}

//

bool ElementDropMeshNodes::DropMeshNode::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);
	tags.load_from_xml(_node, TXT("tags"));
	toolSet.load_from_xml(_node->first_child_named(TXT("wheresMyPoint")));
	
	return result;
}

