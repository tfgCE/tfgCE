#include "meshGenModifiers.h"

//

using namespace Framework;
using namespace MeshGeneration;
using namespace Modifiers;

//

void Modifiers::add_modifiers()
{
	REGISTER_MESH_GEN_MODIFIER(anyMeshData, Element::UpdateWMPPreProcess, Modifiers::any_mesh_data, Modifiers::create_any_mesh_data_data);
	REGISTER_MESH_GEN_MODIFIER(convexifyMovementCollision, Element::UpdateWMPPreProcess, Modifiers::convexify_movement_collision, Modifiers::create_convexify_movement_collision_data);
	REGISTER_MESH_GEN_MODIFIER(cut, Element::UpdateWMPPreProcess, Modifiers::cut, Modifiers::create_cut_data);
	REGISTER_MESH_GEN_MODIFIER(dropMesh, Element::UpdateWMPPreProcess, Modifiers::drop_mesh, Modifiers::create_drop_mesh_data);
	REGISTER_MESH_GEN_MODIFIER(dropSpaceBlocker, Element::UpdateWMPPreProcess, Modifiers::drop_space_blocker, Modifiers::create_drop_space_blocker_data);
	REGISTER_MESH_GEN_MODIFIER(extract, Element::UpdateWMPDirectly, Modifiers::extract, Modifiers::create_extract_data);
	REGISTER_MESH_GEN_MODIFIER(invertNormals, Element::UpdateWMPPreProcess, Modifiers::invert_normals, Modifiers::create_invert_normals_data);
	REGISTER_MESH_GEN_MODIFIER(kaleidoscope, Element::UpdateWMPPreProcess, Modifiers::kaleidoscope, Modifiers::create_kaleidoscope_data);
	REGISTER_MESH_GEN_MODIFIER(makeMirror, Element::UpdateWMPPreProcess, Modifiers::make_mirror, Modifiers::create_mirror_data); // make_mirror shares data (and source file too!)
	REGISTER_MESH_GEN_MODIFIER(matchPlacement, Element::UpdateWMPPreProcess, Modifiers::match_placement, Modifiers::create_match_placement_data);
	REGISTER_MESH_GEN_MODIFIER(mirror, Element::UpdateWMPPreProcess, Modifiers::mirror, Modifiers::create_mirror_data);
	REGISTER_MESH_GEN_MODIFIER(scale, Element::UpdateWMPPreProcess, Modifiers::scale, Modifiers::create_scale_data);
	REGISTER_MESH_GEN_MODIFIER(scalePerVertex, Element::UpdateWMPPreProcess, Modifiers::scale_per_vertex, Modifiers::create_scale_per_vertex_data);
	REGISTER_MESH_GEN_MODIFIER(setCustomData, Element::UpdateWMPPreProcess, Modifiers::set_custom_data, Modifiers::create_set_custom_data_data);
	REGISTER_MESH_GEN_MODIFIER(setVAlong, Element::UpdateWMPPreProcess, Modifiers::set_v_along, Modifiers::create_set_v_along_data);
	REGISTER_MESH_GEN_MODIFIER(skeleton, Element::UpdateWMPDirectly, Modifiers::extract, Modifiers::create_extract_data);
	REGISTER_MESH_GEN_MODIFIER(skew, Element::UpdateWMPPreProcess, Modifiers::skew, Modifiers::create_skew_data);
	REGISTER_MESH_GEN_MODIFIER(skinAlong, Element::UpdateWMPPreProcess, Modifiers::skin_along, Modifiers::create_skin_along_data);
	REGISTER_MESH_GEN_MODIFIER(skinAlongChain, Element::UpdateWMPPreProcess, Modifiers::skin_along_chain, Modifiers::create_skin_along_chain_data);
	REGISTER_MESH_GEN_MODIFIER(skinRange, Element::UpdateWMPPreProcess, Modifiers::skin_range, Modifiers::create_skin_range_data);
	REGISTER_MESH_GEN_MODIFIER(splitMovementCollision, Element::UpdateWMPPreProcess, Modifiers::split_movement_collision, Modifiers::create_split_movement_collision_data);
	REGISTER_MESH_GEN_MODIFIER(simplifyToBox, Element::UpdateWMPPreProcess, Modifiers::simplify_to_box, Modifiers::create_simplify_to_box_data);
	REGISTER_MESH_GEN_MODIFIER(validateSpaceBlocker, Element::UpdateWMPPreProcess, Modifiers::validate_space_blocker, Modifiers::create_validate_space_blocker_data);
}

