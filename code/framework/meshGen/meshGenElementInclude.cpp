#include "meshGenElementInclude.h"

#include "meshGenerator.h"
#include "meshGenGenerationContext.h"
#include "meshGenUtils.h"

#include "..\collision\physicalMaterial.h"
#include "..\library\library.h"
#include "..\library\libraryPrepareContext.h"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\io\xml.h"
#include "..\..\core\random\randomUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;

//

DEFINE_STATIC_NAME(probCoef);
DEFINE_STATIC_NAME(probabilityCoef);

//

REGISTER_FOR_FAST_CAST(ElementInclude);

ElementInclude::~ElementInclude()
{
}

bool ElementInclude::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	createMovementCollisionMesh.load_from_xml(_node, TXT("createMovementCollisionMesh"), _lc);
	createMovementCollisionBox.load_from_xml(_node, TXT("createMovementCollisionBox"), _lc);
	createPreciseCollisionMesh.load_from_xml(_node, TXT("createPreciseCollisionMesh"), _lc);
	createPreciseCollisionBox.load_from_xml(_node, TXT("createPreciseCollisionBox"), _lc);
	createPreciseCollisionMeshSkinned.load_from_xml(_node, TXT("createPreciseCollisionMeshSkinned"), _lc);

	createSpaceBlocker.load_from_xml(_node, TXT("createSpaceBlocker"));

	randomNumber.load_from_xml(_node, TXT("randomNumber"));
	{
		IncludeMeshGenerator mg;
		mg.probabilityCoef = _node->get_float_attribute_or_from_child(TXT("probCoef"), mg.probabilityCoef);
		mg.probabilityCoef = _node->get_float_attribute_or_from_child(TXT("probabilityCoef"), mg.probabilityCoef);
		result &= mg.meshGenerator.load_from_xml(_node, TXT("meshGenerator"));
		result &= mg.tagged.load_from_xml_attribute_or_child_node(_node, TXT("tagged"));
		if (mg.meshGenerator.is_set() || ! mg.tagged.is_empty())
		{
			result &= _lc.load_group_into(mg.meshGenerator);
			meshGenerators.clear();
			meshGenerators.push_back(mg);
		}
	}
	for_every(node, _node->children_named(TXT("include")))
	{
		IncludeMeshGenerator mg;
		mg.probabilityCoef = node->get_float_attribute_or_from_child(TXT("probCoef"), mg.probabilityCoef);
		mg.probabilityCoef = node->get_float_attribute_or_from_child(TXT("probabilityCoef"), mg.probabilityCoef);
		result &= mg.meshGenerator.load_from_xml(node, TXT("meshGenerator"));
		result &= mg.tagged.load_from_xml_attribute_or_child_node(node, TXT("tagged"));
		result &= _lc.load_group_into(mg.meshGenerator);
		meshGenerators.push_back(mg);
	}
	
	if (meshGenerators.is_empty())
	{
		error_loading_xml(_node, TXT("no meshGenerators provided to include"));
		result = false;
	}

	return result;
}

bool ElementInclude::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool ElementInclude::process(GenerationContext & _context, ElementInstance & _instance) const
{
	int meshPartsSoFar = _context.get_parts().get_size();

	int rnd = 0;
	if (randomNumber.is_set() &&
		(randomNumber.is_value_set() ||
		(randomNumber.get_value_param().is_set() && _context.has_any_parameter(randomNumber.get_value_param().get()))))
	{
		rnd = RandomUtils::ChooseFromContainer<Array<IncludeMeshGenerator>, IncludeMeshGenerator>::choose(randomNumber.get(_context), meshGenerators, [](IncludeMeshGenerator const & _option) { return _option.probabilityCoef; });
	}
	else
	{
		rnd = RandomUtils::ChooseFromContainer<Array<IncludeMeshGenerator>, IncludeMeshGenerator>::choose(_context.access_random_generator(), meshGenerators, [](IncludeMeshGenerator const & _option) { return _option.probabilityCoef; });
	}
	if (meshGenerators.is_index_valid(rnd))
	{
		auto& chosenIMG = meshGenerators[rnd];
		MeshGenerator* mg = nullptr;
		if (chosenIMG.meshGenerator.is_set())
		{
			Framework::LibraryName mgName = chosenIMG.meshGenerator.get(_context);
			if (mgName.is_valid())
			{
				if ((mg = Library::get_current()->get_mesh_generators().find(mgName)))
				{
					// ok
				}
				else
				{
					error_generating_mesh(_instance, TXT("could not include \"%S\" mesh generator, not found"), mgName.to_string().to_char());
					return false;
				}
			}
		}
		if (!mg && !chosenIMG.tagged.is_empty())
		{
			struct ChooseMG
			{
				float probabilityCoef = 1.0f;
				Framework::MeshGenerator* mg = nullptr;
			};
			Array<ChooseMG> oneOf;
			for_every_ptr(fmg, Library::get_current()->get_mesh_generators().get_tagged(chosenIMG.tagged))
			{
				ChooseMG cmg;
				cmg.mg = fmg;
				cmg.probabilityCoef = fmg->get_tags().get_tag(NAME(probabilityCoef), 1.0f);
				cmg.probabilityCoef = fmg->get_tags().get_tag(NAME(probCoef), cmg.probabilityCoef);
				oneOf.push_back(cmg);
			}
			if (!oneOf.is_empty())
			{
				int idx = RandomUtils::ChooseFromContainer<Array<ChooseMG>, ChooseMG>::choose(_context.access_random_generator(), oneOf, [](ChooseMG const& _option) { return _option.probabilityCoef; });
				mg = oneOf[idx].mg;
			}
		}
		if (mg)
		{
			mg->load_on_demand_if_required();
			if (_context.process(mg->get_main_mesh_generation_element(), &_instance, rnd))
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

				return true;
			}
			else
			{
				error_generating_mesh(_instance, TXT("error processing included \"%S\" mesh generator"), mg->get_name().to_string().to_char());
				return false;
			}
		}
		else
		{
			// none, it's okay
			return true;
		}
	}
	else
	{
		// it's okay if we chosen something that was without mesh generator
		return true;
	}
}

#ifdef AN_DEVELOPMENT
void ElementInclude::for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const
{
	for_every(meshGenerator, meshGenerators)
	{
		// this won't work on variable dependent
		if (meshGenerator->meshGenerator.is_value_set())
		{
			Framework::LibraryName mgName = meshGenerator->meshGenerator.get_value();
			if (mgName.is_valid())
			{
				if (MeshGenerator* mg = Library::get_current()->get_mesh_generators().find(mgName))
				{
					_do(mg);
				}
			}
		}
		if (! meshGenerator->tagged.is_empty())
		{
			for_every_ptr(mg, Library::get_current()->get_mesh_generators().get_tagged(meshGenerator->tagged))
			{
				_do(mg);
			}
		}
	}
}
#endif
