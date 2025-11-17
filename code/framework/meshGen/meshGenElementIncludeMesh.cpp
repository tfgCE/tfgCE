#include "meshGenElementIncludeMesh.h"

#include "meshGenerator.h"
#include "meshGenGenerationContext.h"
#include "meshGenUtils.h"

#include "..\collision\physicalMaterial.h"
#include "..\library\library.h"
#include "..\library\libraryPrepareContext.h"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\collision\model.h"
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

REGISTER_FOR_FAST_CAST(ElementIncludeMesh);

ElementIncludeMesh::~ElementIncludeMesh()
{
}

bool ElementIncludeMesh::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
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
		result &= mg.mesh.load_from_xml(_node, TXT("mesh"));
		result &= mg.tagged.load_from_xml_attribute_or_child_node(_node, TXT("tagged"));
		if (mg.mesh.is_set() || ! mg.tagged.is_empty())
		{
			result &= _lc.load_group_into(mg.mesh);
			meshes.clear();
			meshes.push_back(mg);
		}
	}
	for_every(node, _node->children_named(TXT("include")))
	{
		IncludeMeshGenerator mg;
		mg.probabilityCoef = node->get_float_attribute_or_from_child(TXT("probCoef"), mg.probabilityCoef);
		mg.probabilityCoef = node->get_float_attribute_or_from_child(TXT("probabilityCoef"), mg.probabilityCoef);
		result &= mg.mesh.load_from_xml(node, TXT("mesh"));
		result &= mg.tagged.load_from_xml_attribute_or_child_node(node, TXT("tagged"));
		result &= _lc.load_group_into(mg.mesh);
		meshes.push_back(mg);
	}
	
	if (meshes.is_empty())
	{
		error_loading_xml(_node, TXT("no meshes provided to include"));
		result = false;
	}

	return result;
}

bool ElementIncludeMesh::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool ElementIncludeMesh::process(GenerationContext & _context, ElementInstance & _instance) const
{
	int meshPartsSoFar = _context.get_parts().get_size();

	int rnd = 0;
	if (randomNumber.is_set() &&
		(randomNumber.is_value_set() ||
		(randomNumber.get_value_param().is_set() && _context.has_any_parameter(randomNumber.get_value_param().get()))))
	{
		rnd = RandomUtils::ChooseFromContainer<Array<IncludeMeshGenerator>, IncludeMeshGenerator>::choose(randomNumber.get(_context), meshes, [](IncludeMeshGenerator const & _option) { return _option.probabilityCoef; });
	}
	else
	{
		rnd = RandomUtils::ChooseFromContainer<Array<IncludeMeshGenerator>, IncludeMeshGenerator>::choose(_context.access_random_generator(), meshes, [](IncludeMeshGenerator const & _option) { return _option.probabilityCoef; });
	}
	if (meshes.is_index_valid(rnd))
	{
		auto& chosenIM = meshes[rnd];
		Mesh* m = nullptr;
		if (chosenIM.mesh.is_set())
		{
			Framework::LibraryName meshName = chosenIM.mesh.get(_context);
			if (meshName.is_valid())
			{
				if ((m = Library::get_current()->get_meshes().find(meshName)))
				{
					// ok
				}
				else
				{
					error_generating_mesh(_instance, TXT("could not include \"%S\" mesh not found"), meshName.to_string().to_char());
					return false;
				}
			}
		}
		if (!m && !chosenIM.tagged.is_empty())
		{
			struct ChooseMG
			{
				float probabilityCoef = 1.0f;
				Framework::Mesh* m = nullptr;
			};
			Array<ChooseMG> oneOf;
			for_every_ptr(fm, Library::get_current()->get_meshes_static().get_tagged(chosenIM.tagged))
			{
				ChooseMG cmg;
				cmg.m = fm;
				cmg.probabilityCoef = fm->get_tags().get_tag(NAME(probabilityCoef), 1.0f);
				cmg.probabilityCoef = fm->get_tags().get_tag(NAME(probCoef), cmg.probabilityCoef);
				oneOf.push_back(cmg);
			}
			for_every_ptr(fm, Library::get_current()->get_meshes_skinned().get_tagged(chosenIM.tagged))
			{
				ChooseMG cmg;
				cmg.m = fm;
				cmg.probabilityCoef = fm->get_tags().get_tag(NAME(probabilityCoef), 1.0f);
				cmg.probabilityCoef = fm->get_tags().get_tag(NAME(probCoef), cmg.probabilityCoef);
				oneOf.push_back(cmg);
			}
			if (!oneOf.is_empty())
			{
				int idx = RandomUtils::ChooseFromContainer<Array<ChooseMG>, ChooseMG>::choose(_context.access_random_generator(), oneOf, [](ChooseMG const& _option) { return _option.probabilityCoef; });
				m = oneOf[idx].m;
			}
		}
		if (m)
		{
			m->load_on_demand_if_required();
			if (auto* mesh = m->get_mesh())
			{
				if (_context.get_vertex_format().has_skinning_data())
				{
					todo_implement(TXT("not supported yet, we would need to copy bones etc"));
				}
				for_count(int, partIdx, mesh->get_parts_num())
				{
					if (auto* part = mesh->get_part(partIdx))
					{
						auto* newPart = part->create_copy();

						newPart->convert_vertex_data_to(_context.get_vertex_format());

						_context.store_part(newPart, partIdx); // material idx is the same as part idx
					}
				}
			}
			if (auto* c = m->get_movement_collision_model())
			{
				if (auto* cm = c->get_model())
				{
					_context.store_movement_collision_part(cm->create_copy());
				}
			}
			if (auto* c = m->get_precise_collision_model())
			{
				if (auto* cm = c->get_model())
				{
					_context.store_precise_collision_part(cm->create_copy());
				}
			}
			todo_note(TXT("add sockets, mesh nodes, pois, space blockers, controllers, etc"));
			{
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

				return true;
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
