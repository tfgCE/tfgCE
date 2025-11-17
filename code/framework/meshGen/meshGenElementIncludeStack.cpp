#include "meshGenElementIncludeStack.h"

#include "meshGenerator.h"
#include "meshGenGenerationContext.h"
#include "meshGenUtils.h"

#include "..\collision\physicalMaterial.h"
#include "..\debug\previewGame.h"
#include "..\game\game.h"
#include "..\library\library.h"
#include "..\library\libraryPrepareContext.h"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\io\xml.h"
#include "..\..\core\random\randomUtils.h"

using namespace Framework;
using namespace MeshGeneration;

//

REGISTER_FOR_FAST_CAST(ElementIncludeStack);

ElementIncludeStack::~ElementIncludeStack()
{
}

bool ElementIncludeStack::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	createMovementCollisionMesh.load_from_xml(_node, TXT("createMovementCollisionMesh"), _lc);
	createMovementCollisionBox.load_from_xml(_node, TXT("createMovementCollisionBox"), _lc);
	createPreciseCollisionMesh.load_from_xml(_node, TXT("createPreciseCollisionMesh"), _lc);
	createPreciseCollisionBox.load_from_xml(_node, TXT("createPreciseCollisionBox"), _lc);
	createPreciseCollisionMeshSkinned.load_from_xml(_node, TXT("createPreciseCollisionMeshSkinned"), _lc);
	
	createSpaceBlocker.load_from_xml(_node, TXT("createSpaceBlocker"));

	stackElement.load_from_xml(_node, TXT("element"));
	allowNoMeshGenerator = _node->get_bool_attribute_or_from_child_presence(TXT("allowNoMeshGenerator"), allowNoMeshGenerator);

#ifdef AN_DEVELOPMENT_OR_PROFILER
	result &= previewMeshGenerator.load_from_xml(_node, TXT("previewMeshGenerator"), _lc);
#endif
	result &= emptyMeshGenerator.load_from_xml(_node, TXT("emptyMeshGenerator"), _lc);

	return result;
}

bool ElementIncludeStack::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

#ifdef AN_DEVELOPMENT_OR_PROFILER
	result &= previewMeshGenerator.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
#endif
	result &= emptyMeshGenerator.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	return result;
}

bool ElementIncludeStack::process(GenerationContext & _context, ElementInstance & _instance) const
{
	int meshPartsSoFar = _context.get_parts().get_size();
	_context.push_include_stack(stackElement.get(_context));
	bool result = true;
	MeshGenerator * mg = nullptr;
	if (!mg)
	{
		mg = _context.run_include_stack_processor();
	}
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (!mg)
	{
		if (fast_cast<PreviewGame>(Game::get()))
		{
			if (previewMeshGenerator.is_set())
			{
				mg = previewMeshGenerator.get();
			}
		}
	}
#endif
	if (!mg)
	{
		if (emptyMeshGenerator.is_set())
		{
			mg = emptyMeshGenerator.get();
		}
	}
	if (mg)
	{
		mg->load_on_demand_if_required();
		if (_context.process(mg->get_main_mesh_generation_element(), &_instance))
		{
			if (_context.does_require_movement_collision())
			{
				if (mg->get_create_movement_collision_mesh().create)
				{
					_context.store_movement_collision_parts(create_collision_meshes_from(_context, mg->get_create_movement_collision_mesh(), meshPartsSoFar));
				}
				else if (mg->get_create_movement_collision_box().create)
				{
					_context.store_movement_collision_part(create_collision_box_from(_context, mg->get_create_movement_collision_box(), meshPartsSoFar));
				}
				else if (createMovementCollisionMesh.create)
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
				if (mg->get_create_precise_collision_mesh().create)
				{
					_context.store_precise_collision_parts(create_collision_meshes_from(_context, mg->get_create_precise_collision_mesh(), meshPartsSoFar));
				}
				else if (mg->get_create_precise_collision_box().create)
				{
					_context.store_precise_collision_part(create_collision_box_from(_context, mg->get_create_precise_collision_box(), meshPartsSoFar));
				}
				else if (createPreciseCollisionMesh.create)
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
				if (mg->get_create_space_blocker().create)
				{
					_context.store_space_blocker(create_space_blocker_from(_context, mg->get_create_space_blocker(), meshPartsSoFar));
				}
				if (createSpaceBlocker.create)
				{
					_context.store_space_blocker(create_space_blocker_from(_context, createSpaceBlocker, meshPartsSoFar));
				}
			}

			result = true;
		}
		else
		{
			error_generating_mesh(_instance, TXT("error processing included \"%S\" mesh generator"), mg->get_name().to_string().to_char());
			result = false;
		}
	}
	else
	{
		if (!allowNoMeshGenerator)
		{
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (fast_cast<PreviewGame>(Game::get()))
			{
				warn_generating_mesh(_instance, TXT("could not find include stack mesh generator for \"%S\""), _context.get_include_stack_as_string().to_char());
			}
			else
#endif
			{
				error_generating_mesh(_instance, TXT("could not find include stack mesh generator for \"%S\""), _context.get_include_stack_as_string().to_char());
				result = false;
			}
		}
	}
	_context.pop_include_stack();
	return result;
}
