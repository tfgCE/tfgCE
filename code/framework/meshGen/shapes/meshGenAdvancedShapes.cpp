#include "meshGenAdvancedShapes.h"

#include "..\meshGenElementShape.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace AdvancedShapes;

void AdvancedShapes::add_shapes()
{
	REGISTER_MESH_GEN_SHAPE(TXT("edges and surfaces"), AdvancedShapes::edges_and_surfaces, AdvancedShapes::create_edges_and_surfaces_data);
	REGISTER_MESH_GEN_SHAPE(TXT("cross section cylindrical"), AdvancedShapes::cross_section_cylindrical, AdvancedShapes::create_cross_section_cylindrical_data);
	REGISTER_MESH_GEN_SHAPE(TXT("cross section along"), AdvancedShapes::cross_section_along, AdvancedShapes::create_cross_section_along_data);
	REGISTER_MESH_GEN_SHAPE(TXT("cross sections"), AdvancedShapes::cross_sections, AdvancedShapes::create_cross_sections_data);
	REGISTER_MESH_GEN_SHAPE(TXT("flat display"), AdvancedShapes::flat_display, AdvancedShapes::create_flat_display_data);
	REGISTER_MESH_GEN_SHAPE(TXT("lightning"), AdvancedShapes::lightning, AdvancedShapes::create_lightning_data);
	REGISTER_MESH_GEN_SHAPE(TXT("desert"), AdvancedShapes::desert, AdvancedShapes::create_desert_data);
	REGISTER_MESH_GEN_SHAPE(TXT("door hole complement"), AdvancedShapes::door_hole_complement, AdvancedShapes::create_door_hole_complement_data);
}

