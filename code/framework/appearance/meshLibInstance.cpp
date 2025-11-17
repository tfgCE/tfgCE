#include "meshLibInstance.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\mesh\mesh3d.h"

#include "..\..\core\debug\debugRenderer.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(Framework::MeshLibInstance);
LIBRARY_STORED_DEFINE_TYPE(MeshLibInstance, meshLibInstance);

MeshLibInstance::MeshLibInstance(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
{
}

MeshLibInstance::~MeshLibInstance()
{
}

void MeshLibInstance::prepare_to_unload()
{
	base::prepare_to_unload();
	mesh.clear();
	materialSetups.clear();
}

#ifdef AN_DEVELOPMENT
void MeshLibInstance::ready_for_reload()
{
	base::ready_for_reload();

	mesh.clear();
	materialSetups.clear();
}
#endif

bool MeshLibInstance::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = LibraryStored::load_from_xml(_node, _lc);

	result &= mesh.load_from_xml(_node, TXT("mesh"), _lc);

	placement.load_from_xml_child_node(_node, TXT("placement"));

	for_every(materialsNode, _node->children_named(TXT("materials")))
	{
		for_every(materialSetupNode, materialsNode->children_named(TXT("material")))
		{
			int idx = materialSetupNode->get_int_attribute(TXT("index"), -1);
			if (idx >= 0)
			{
				memory_leak_suspect;
				materialSetups.set_size(max(materialSetups.get_size(), idx + 1));
				forget_memory_leak_suspect;
				materialSetups[idx].load_from_xml(materialSetupNode, _lc);
			}
		}
	}

	return result;
}

bool MeshLibInstance::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		mesh.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::LoadMaterialIntoMeshLibInstance)
	{
		for_every(materialSetup, materialSetups)
		{
			result &= materialSetup->prepare_for_game(_library, _pfgContext, LibraryPrepareLevel::LoadMaterialIntoMeshLibInstance);
		}
		set_missing_materials(_library);
	}
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::InitialiseMeshLibInstance)
	{
		if (mesh.get())
		{
			mesh->generated_on_demand_if_required(); // to catch inappropriate use

			if (Skeleton const* skeleton = mesh->get_skeleton())
			{
				meshInstance.set_mesh(mesh->get_mesh(), skeleton->get_skeleton());
			}
			else
			{
				meshInstance.set_mesh(mesh->get_mesh());
			}

			MaterialSetup::apply_material_setups(materialSetups, meshInstance, System::MaterialSetting::NotIndividualInstance);

			meshInstance.fill_materials_with_missing();
		}
	}
	return result;
}

void MeshLibInstance::set_missing_materials(Library* _library)
{
	if (auto* m = mesh.get())
	{
		m->generated_on_demand_if_required(); // to catch inappropriate use
		if (auto* mesh = m->get_mesh())
		{
			for (int index = 0, partsNum = mesh->get_parts_num(); index < partsNum; ++index)
			{
				if (!get_material(index))
				{
					set_material(index, Material::get_default(_library));
				}
			}
		}
	}
}

void MeshLibInstance::debug_draw() const
{
	debug_push_transform(placement);
	debug_draw_mesh_bones(false, mesh->get_mesh(), Transform::identity, &meshInstance, &meshInstance);
	debug_pop_transform();
}

void MeshLibInstance::log(LogInfoContext& _context) const
{
	base::log(_context);
}