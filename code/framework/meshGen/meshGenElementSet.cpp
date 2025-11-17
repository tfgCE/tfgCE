#include "meshGenElementSet.h"

#include "meshGenerator.h"
#include "meshGenGenerationContext.h"

#include "meshGenUtils.h"

#include "..\..\core\io\xml.h"

using namespace Framework;
using namespace MeshGeneration;

//

REGISTER_FOR_FAST_CAST(ElementSet);

ElementSet::~ElementSet()
{
}

bool ElementSet::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	simplifyToBoxInfo.load_from_child_node_xml(_node, _lc);

	createMovementCollisionMesh.load_from_xml(_node, TXT("createMovementCollisionMesh"), _lc);
	createMovementCollisionBox.load_from_xml(_node, TXT("createMovementCollisionBox"), _lc);
	createPreciseCollisionMesh.load_from_xml(_node, TXT("createPreciseCollisionMesh"), _lc);
	createPreciseCollisionBox.load_from_xml(_node, TXT("createPreciseCollisionBox"), _lc);
	createPreciseCollisionMeshSkinned.load_from_xml(_node, TXT("createPreciseCollisionMeshSkinned"), _lc);

	createSpaceBlocker.load_from_xml(_node, TXT("createSpaceBlocker"));

	bool anyElementsPresent = false;
	for_every(elementsNode, _node->children_named(TXT("elements")))
	{
		for_every(node, elementsNode->all_children())
		{
			if (Element* element = Element::create_from_xml(node, _lc))
			{
				if (element->load_from_xml(node, _lc))
				{
					elements.push_back(RefCountObjectPtr<Element>(element));
				}
				else
				{
					error_loading_xml(node, TXT("problem loading one element of set"));
					result = false;
				}
			}
		}
		anyElementsPresent = true;
	}

	if (!anyElementsPresent)
	{
		error_loading_xml(_node, TXT("no elements provided for set"));
		result = false;
	}

	return result;
}

bool ElementSet::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(element, elements)
	{
		result &= element->get()->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

bool ElementSet::process(GenerationContext & _context, ElementInstance & _instance) const
{
	bool result = true;

	int meshPartsSoFar = _context.get_parts().get_size();

	for_every(element, elements)
	{
		_context.access_random_generator().advance_seed(2374, 823794);

		result &= _context.process(element->get(), &_instance, for_everys_index(element));
	}

	simplifyToBoxInfo.process(_context, _instance, meshPartsSoFar, true);

	if (_context.does_require_movement_collision())
	{
		if (createMovementCollisionMesh.create)
		{
			_context.store_movement_collision_parts(create_collision_meshes_from(_context, createMovementCollisionMesh, meshPartsSoFar));
		}
		else if (createMovementCollisionBox.create)
		{
			_context.store_movement_collision_part(create_collision_box_from(_context, createMovementCollisionBox, meshPartsSoFar));
		}
	}

	if (_context.does_require_precise_collision())
	{
		if (createPreciseCollisionMesh.create)
		{
			_context.store_precise_collision_parts(create_collision_meshes_from(_context, createPreciseCollisionMesh, meshPartsSoFar));
		}
		else if (createPreciseCollisionBox.create)
		{
			_context.store_precise_collision_part(create_collision_box_from(_context, createPreciseCollisionBox, meshPartsSoFar));
		}
		else if (createPreciseCollisionMeshSkinned.create)
		{
			_context.store_precise_collision_parts(create_collision_skinned_meshes_from(_context, createPreciseCollisionMeshSkinned, meshPartsSoFar));
		}
	}

	if (_context.does_require_space_blockers())
	{
		if (createSpaceBlocker.create)
		{
			_context.store_space_blocker(create_space_blocker_from(_context, createSpaceBlocker, meshPartsSoFar));
		}
	}

	return result;
}

#ifdef AN_DEVELOPMENT
void ElementSet::for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const
{
	for_every(element, elements)
	{
		element->get()->for_included_mesh_generators(_do);
	}
}
#endif
