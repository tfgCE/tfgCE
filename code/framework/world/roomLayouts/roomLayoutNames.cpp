#include "roomLayoutNames.h"

#include "..\..\..\core\types\name.h"

//

DEFINE_STATIC_NAME(reconnectable);

DEFINE_STATIC_NAME(wallElementYP);
DEFINE_STATIC_NAME(wallElementXP);
DEFINE_STATIC_NAME(wallElementYM);
DEFINE_STATIC_NAME(wallElementXM);

DEFINE_STATIC_NAME(leftCornerElementYP);
DEFINE_STATIC_NAME(leftCornerElementXP);
DEFINE_STATIC_NAME(leftCornerElementYM);
DEFINE_STATIC_NAME(leftCornerElementXM);

DEFINE_STATIC_NAME(roomSize);
DEFINE_STATIC_NAME(roomHeight);
DEFINE_STATIC_NAME(floorTileMesh);
DEFINE_STATIC_NAME(wallLength);
DEFINE_STATIC_NAME(doorType);
DEFINE_STATIC_NAME(doorHolePieceSize);

DEFINE_STATIC_NAME(doorDepth);
DEFINE_STATIC_NAME(doorHolePieceLength);
DEFINE_STATIC_NAME(doorHoleInWallMeshGenerator);

//

using namespace Framework;

//

Name const& RoomLayoutNames::DoorInRoomTags::reconnectable() { return NAME(reconnectable); }

Name const& RoomLayoutNames::ObjectTags::wall_element_yp() { return NAME(wallElementYP); }
Name const& RoomLayoutNames::ObjectTags::wall_element_xp() { return NAME(wallElementXP); }
Name const& RoomLayoutNames::ObjectTags::wall_element_ym() { return NAME(wallElementYM); }
Name const& RoomLayoutNames::ObjectTags::wall_element_xm() { return NAME(wallElementXM); }
Name const& RoomLayoutNames::ObjectTags::left_corner_element_yp() { return NAME(leftCornerElementYP); }
Name const& RoomLayoutNames::ObjectTags::left_corner_element_xp() { return NAME(leftCornerElementXP); }
Name const& RoomLayoutNames::ObjectTags::left_corner_element_ym() { return NAME(leftCornerElementYM); }
Name const& RoomLayoutNames::ObjectTags::left_corner_element_xm() { return NAME(leftCornerElementXM); }

Name const& RoomLayoutNames::MeshVariables::room_size() { return NAME(roomSize); }
Name const& RoomLayoutNames::MeshVariables::room_height() { return NAME(roomHeight); }
Name const& RoomLayoutNames::MeshVariables::floor_tile_mesh() { return NAME(floorTileMesh); }
Name const& RoomLayoutNames::MeshVariables::wall_length() { return NAME(wallLength); }
Name const& RoomLayoutNames::MeshVariables::door_type() { return NAME(doorType); }
Name const& RoomLayoutNames::MeshVariables::door_hole_piece_size() { return NAME(doorHolePieceSize); }

Name const& RoomLayoutNames::DoorTypeCustomParameters::door_depth() { return NAME(doorDepth); }
Name const& RoomLayoutNames::DoorTypeCustomParameters::door_hole_piece_length() { return NAME(doorHolePieceLength); }
Name const& RoomLayoutNames::DoorTypeCustomParameters::door_hole_in_wall_mesh_generator() { return NAME(doorHoleInWallMeshGenerator); }
