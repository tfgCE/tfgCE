#pragma once

struct Name;

namespace Framework
{
	namespace RoomLayoutNames
	{
		namespace DoorInRoomTags
		{
			Name const& reconnectable(); // door that can be reconnected and mixed with other door
		};
		
		namespace ObjectTags
		{
			Name const& wall_element_yp();
			Name const& wall_element_xp();
			Name const& wall_element_ym();
			Name const& wall_element_xm();
			Name const& left_corner_element_yp();
			Name const& left_corner_element_xp();
			Name const& left_corner_element_ym();
			Name const& left_corner_element_xm();
		};

		namespace MeshVariables
		{
			// variables
			Name const& room_size(); // size of the room [Vector2]
			Name const& room_height(); // heigh of the room [float]
			Name const& floor_tile_mesh(); // mesh used as a tile for floor mesh generator [LibraryName]
			Name const& wall_length(); // length of the wall for wall mesh generator [float]
			Name const& door_type(); // door used for door hole mesh generator [LibraryName]
			Name const& door_hole_piece_size(); // size of part of the wall for "door in hole wall" [Vector2]
		};

		namespace DoorTypeCustomParameters
		{
			// parameters stored in doorType customParameters
			Name const& door_depth(); // how deep into the wall door is [float]
			Name const& door_hole_piece_length(); // length of part of the wall for "door in hole wall" [float]
			Name const& door_hole_in_wall_mesh_generator(); // mesh generator to be used for door hole in wall, only set it if really needed, otherwise use provided one [LibraryName]
		};
	};
};
