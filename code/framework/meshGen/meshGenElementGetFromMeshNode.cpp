#include "meshGenElementGetFromMeshNode.h"

#include "meshGenGenerationContext.h"
#include "meshGenMeshNodeContext.h"
#include "meshGenUtils.h"

#include "..\appearance\meshNode.h"

#include "..\meshGen\meshGenValueDefImpl.inl"

#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\io\xml.h"

//

using namespace Framework;
using namespace MeshGeneration;

//

REGISTER_FOR_FAST_CAST(ElementGetFromMeshNode);

ElementGetFromMeshNode::ElementGetFromMeshNode()
{
	useStack = false;
}

ElementGetFromMeshNode::~ElementGetFromMeshNode()
{
}

bool ElementGetFromMeshNode::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	name.load_from_xml(_node, TXT("name"));
	tagged.load_from_xml_attribute(_node, TXT("tagged"));
	drop = _node->get_bool_attribute_or_from_child_presence(TXT("drop"), drop);

	for_every(n, _node->all_children())
	{
		if (n->get_name() == TXT("drop"))
		{
			drop = true;
			continue;
		}
		if (n->get_name() == TXT("placement"))
		{
			placementAs = n->get_name_attribute(TXT("as"), placementAs);
			continue;
		}
		if (n->get_name() == TXT("location"))
		{
			locationAs = n->get_name_attribute(TXT("as"), locationAs);
			continue;
		}
		Name locationAs;
		GetParameter gp;
		gp.type = RegisteredType::get_type_id_by_name(n->get_name().to_char());
		gp.name = n->get_name_attribute(TXT("name"));
		gp.storeAs = n->get_name_attribute(TXT("as"), gp.name);
		if (gp.type == NONE)
		{
			error_loading_xml(n, TXT("not recognised type \"%S\""), n->get_name().to_char());
			result = false;
		}
		else if (!gp.name.is_valid())
		{
			error_loading_xml(n, TXT("no name provided for output parameter"));
			result = false;
		}
		else
		{
			if (!gp.storeAs.is_valid())
			{
				gp.storeAs = gp.name;
			}
			getParameters.push_back(gp);
		}
	}

	return result;
}

bool ElementGetFromMeshNode::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool ElementGetFromMeshNode::process(GenerationContext & _context, ElementInstance & _instance) const
{
	bool result = true;
	auto & contextMeshNodes = _context.get_mesh_nodes();
	for_every_ref(meshNode, contextMeshNodes)
	{
		bool matches = true;
		if (matches)
		{
			if (name.is_set() &&
				name.get(_context) != meshNode->name)
			{
				matches = false;
			}
			if (!tagged.is_empty() &&
				!tagged.check(meshNode->tags))
			{
				matches = false;
			}
		}
		if (matches)
		{
			if (placementAs.is_valid())
			{
				_context.set_parameter(placementAs, meshNode->placement);
			}
			if (locationAs.is_valid())
			{
				_context.set_parameter(locationAs, meshNode->placement.get_translation());
			}
			for_every(gp, getParameters)
			{
				if (void const* src = meshNode->variables.get_raw(gp->name, gp->type))
				{
					an_assert(RegisteredType::is_plain_data(gp->type));
					_context.set_parameter(gp->storeAs, gp->type, src);
				}
			}
			if (drop)
			{
				meshNode->drop();
			}
			break;
		}
	}
	return result;
}
