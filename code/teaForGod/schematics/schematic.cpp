#include "schematic.h"

#include "schematicLine.h"

#include "..\game\game.h"

#include "..\..\core\concurrency\asynchronousJob.h"
#include "..\..\core\mesh\mesh3d.h"
#include "..\..\core\mesh\builders\mesh3dBuilder_IPU.h"

#include "..\..\framework\library\library.h"

//

using namespace TeaForGodEmperor;

//

DEFINE_STATIC_NAME_STR(materialName, TXT("schematics; material"));
DEFINE_STATIC_NAME_STR(materialForOverlayName, TXT("schematics; material; overlay"));

//

struct Schematic_DeleteMeshes
: public Concurrency::AsynchronousJobData
{
	Meshes::IMesh3D* mesh3d = nullptr;
	Meshes::IMesh3D* mesh3dOutline = nullptr;
	Schematic_DeleteMeshes(Meshes::IMesh3D* _mesh3d, Meshes::IMesh3D* _mesh3dOutline)
	: mesh3d(_mesh3d)
	, mesh3dOutline(_mesh3dOutline)
	{}

	static void perform(Concurrency::JobExecutionContext const* _executionContext, void* _data)
	{
		Schematic_DeleteMeshes* data = (Schematic_DeleteMeshes*)_data;
		delete_and_clear(data->mesh3d);
		delete_and_clear(data->mesh3dOutline);
	}
};

Schematic::Schematic()
{
}

Schematic::~Schematic()
{
	if (mesh3d || mesh3dOutline)
	{
		Game::async_system_job(Game::get_as<Game>(), Schematic_DeleteMeshes::perform, new Schematic_DeleteMeshes(mesh3d, mesh3dOutline));
		mesh3d = nullptr;
		mesh3dOutline = nullptr;
	}
}

void Schematic::add(SchematicElement* _element)
{
	elements.push_back(RefCountObjectPtr<SchematicElement>(_element));
}

void Schematic::cut_outer_lines_with(SchematicLine const & _convexLine)
{
	int orgElementsCount = elements.get_size();
	for(int i = 0; i < orgElementsCount; ++ i)
	{
		auto* e = elements[i].get();
		if (auto* l = fast_cast<SchematicLine>(e))
		{
			l->cut_outer_lines_with(_convexLine, this);
		}
	}
}

void Schematic::cut_inner_lines_with(SchematicLine const& _convexLine)
{
	int orgElementsCount = elements.get_size();
	for (int i = 0; i < orgElementsCount; ++i)
	{
		auto* e = elements[i].get();
		if (auto* l = fast_cast<SchematicLine>(e))
		{
			l->cut_inner_lines_with(_convexLine, this);
		}
	}
}

void Schematic::sort_elements()
{
	sort(elements, SchematicElement::compare_ref);
}

void Schematic::build_mesh()
{
	sort_elements();

	size = Range2::empty;
	for_count(int, outline, 2)
	{
		Meshes::Builders::IPU ipu;

		for_every_ref(e, elements)
		{
			if (!outline || !e->is_full_schematic_only())
			{
				e->grow_size(REF_ size);
				e->build(ipu, outline);
			}
		}

		Array<int8> elementsData;
		Array<int8> vertexData;

		System::IndexFormat indexFormat;
		indexFormat.with_element_size(System::IndexElementSize::FourBytes);
		indexFormat.calculate_stride();

		System::VertexFormat vertexFormat;
		vertexFormat.with_location_3d();
		vertexFormat.with_colour_rgb();
		vertexFormat.calculate_stride_and_offsets();

		// prepare data for vertex and index formats, so further modifications may be applied and mesh can be imported
		ipu.prepare_data(vertexFormat, indexFormat, vertexData, elementsData);

		int vertexCount = vertexData.get_size() / vertexFormat.get_stride();
		int elementsCount = elementsData.get_size() / indexFormat.get_stride();

		auto*& storeInMesh3d = outline ? mesh3dOutline : mesh3d;
		auto& storeInMeshInstance = outline ? meshInstanceOutline : meshInstance;
		auto* storeInMeshInstanceForOverlay = ! outline ? &meshInstanceForOverlay : nullptr;
		storeInMeshInstance.set_mesh(nullptr);
		if (storeInMeshInstanceForOverlay)
		{
			storeInMeshInstanceForOverlay->set_mesh(nullptr);
		}

		delete_and_clear(storeInMesh3d);

		Meshes::Mesh3D* m3d = new Meshes::Mesh3D();

		m3d->load_part_data(vertexData.get_data(), elementsData.get_data(), Meshes::Primitive::Triangle, vertexCount, elementsCount, vertexFormat, indexFormat);
		storeInMesh3d = m3d;

		if (auto* mat = Framework::Library::get_current()->get_global_references().get<Framework::Material>(NAME(materialName)))
		{
			storeInMeshInstance.set_mesh(m3d);
			storeInMeshInstance.set_material(0, mat->get_material());
		}
		if (storeInMeshInstanceForOverlay)
		{
			if (auto* mat = Framework::Library::get_current()->get_global_references().get<Framework::Material>(NAME(materialForOverlayName)))
			{
				storeInMeshInstanceForOverlay->set_mesh(m3d);
				storeInMeshInstanceForOverlay->set_material(0, mat->get_material());
			}
		}
	}
}

bool Schematic::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	for_every(node, _node->all_children())
	{
		RefCountObjectPtr<SchematicElement> e;
		if (node->get_name() == TXT("line"))
		{
			e = new SchematicLine();
		}
		if (e.is_set())
		{
			if (e->load_from_xml(node, _lc))
			{
				elements.push_back(e);
			}
			else
			{
				error_loading_xml(node, TXT("couldn't load element"));
				result = false;
			}
		}
		else
		{
			error_loading_xml(node, TXT("couldn't recognise schematic elemetn"));
			result = false;
		}
	}
	return result;
}

