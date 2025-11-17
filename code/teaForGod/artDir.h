#pragma once

#include "..\core\types\colour.h"
#include "..\core\types\optional.h"

struct Colour;

namespace Meshes
{
	class Mesh3DInstance;
	class Mesh3DInstanceSetInlined;
};

namespace Framework
{
	class DoorInRoom;
	class Game;
	class Room;

	namespace Render
	{
		class DoorInRoomProxy;
		class EnvironmentProxy;
		class SceneBuildContext;
	};
};

namespace TeaForGodEmperor
{
	/**
	 *	Everything related to art dir, mostly util functions
	 */
	class ArtDir
	{
	public:
		static void setup_rendering();

		// build in uniforms
		static void set_global_desaturate(Optional<Colour> const& _amount = NP, Optional<Colour> const & _changeInto = NP); // rgb - which colours should get desaturated, you want to keep red? r=0 g=1 b=1, _changeInto is how much is chagned into a different colour
		static void set_global_tint(Optional<Colour> const& _colour = NP);
		static void set_global_tint_fade(Optional<float> const& _fade = NP);

		static void add_custom_build_in_uniforms();

		static void customise_game(Framework::Game* _game);

		static void environment_proxy__setup(Framework::Render::SceneBuildContext const& _context, Framework::Render::EnvironmentProxy* _environmentProxy, Framework::Render::EnvironmentProxy* _previousEnvironmentProxy, Framework::Room const* _forRoom);
			static void environment_proxy__setup_sky_box_material_from_light(Framework::Render::SceneBuildContext const& _context, Framework::Render::EnvironmentProxy* _environmentProxy, Framework::Render::EnvironmentProxy* _previousEnvironmentProxy, Framework::Room const* _forRoom);
			static void environment_proxy__setup_fog_so_far(Framework::Render::SceneBuildContext const& _context, Framework::Render::EnvironmentProxy* _environmentProxy, Framework::Render::EnvironmentProxy* _previousEnvironmentProxy, Framework::Room const* _forRoom);
			static void environment_proxy__setup_background_anchor(Framework::Render::SceneBuildContext const& _context, Framework::Render::EnvironmentProxy* _environmentProxy, Framework::Render::EnvironmentProxy* _previousEnvironmentProxy, Framework::Room const* _forRoom);
			static void environment_proxy__setup_pilgrimage_rotation(Framework::Render::SceneBuildContext const& _context, Framework::Render::EnvironmentProxy* _environmentProxy, Framework::Render::EnvironmentProxy* _previousEnvironmentProxy, Framework::Room const* _forRoom);
		//
		static void door_in_room_proxy__setup(Framework::Render::SceneBuildContext const& _context, Framework::Render::DoorInRoomProxy* _doorInRoomProxy, Framework::DoorInRoom const* _doorInRoom);
			static void door_in_room_proxy__setup_open_world_directions(Framework::Render::SceneBuildContext const& _context, Framework::Render::DoorInRoomProxy* _doorInRoomProxy, Framework::DoorInRoom const* _doorInRoom);
			static void door_in_room_proxy__setup_lock_indicators(Framework::Render::SceneBuildContext const& _context, Framework::Render::DoorInRoomProxy* _doorInRoomProxy, Framework::DoorInRoom const* _doorInRoom);
		
		static void door_in_room_mesh__setup_open_world_directions(Meshes::Mesh3DInstanceSetInlined* _instanceSetInlined, Meshes::Mesh3DInstance * _instance, Framework::DoorInRoom const* _doorInRoom, Optional<bool> const & _didCameThrough = NP);

	};
};
