#pragma once

#include "..\core\globalDefinitions.h"
#include "..\core\other\simpleVariableStorage.h"
#include "..\core\wheresMyPoint\iWMPOwner.h"

struct Colour;
struct Name;
struct Vector3;

namespace Framework
{
	interface_class IModulesOwner;
	class DoorInRoom;
	class ModuleAppearance;
	class Room;
};

namespace TeaForGodEmperor
{
	class Utils
	{
	public:
		static Vector3 get_light_dir(Framework::IModulesOwner const* _owner);
		static Colour get_light_colour(Framework::IModulesOwner const* _owner);
		static Colour get_light_colour(Framework::Room const* _room);
		static float get_sun(Framework::IModulesOwner const* _owner);
		// will keep values intact if can't find proper ones
		static bool get_emissive_colour(Framework::IModulesOwner const* _owner, REF_ Colour& _colour);
		static bool get_emissive_power(Framework::IModulesOwner const* _owner, REF_ float & _power);
		static bool get_emissive_colour(Framework::Room const* _room, REF_ Colour& _colour);
		static bool get_emissive_power(Framework::Room const* _room, REF_ float & _power);

		static void get_projectile_emissives_from_appearance(Framework::ModuleAppearance const* ap, OUT_ Colour& projectileEmissiveColour, OUT_ Colour& projectileEmissiveBaseColour);
		static void calculate_projectile_emissive_colour_base(Colour& projectileEmissiveColour, Colour& projectileEmissiveBaseColour);

		static void set_emissives_from_room_to(Framework::IModulesOwner* imo, Framework::Room* _overrideRoom = nullptr);
		static void set_emissives_from_room_to(Framework::DoorInRoom* dir, Framework::Room* _overrideRoom = nullptr);
		static void set_emissives_for_room_and_all_inside(Framework::Room* _room);
		static void force_set_emissives_for_room(Framework::Room* _room, Optional<Colour> const& _emissiveColour, Optional<Colour> const& _emissiveBaseColour, Optional<float> const& _emissivePower) { force_blend_emissives_for_room(_room, _emissiveColour, _emissiveBaseColour, _emissivePower, 0.0f); }
		static void force_blend_emissives_for_room(Framework::Room* _room, Optional<Colour> const& _emissiveColour, Optional<Colour> const& _emissiveBaseColour, Optional<float> const & _emissivePower, float _blendAmount);

		template <typename Class>
		static Class get_value_from(Name const& _byName, Class const& _default, SimpleVariableStorage const* _variables, WheresMyPoint::IOwner const* _wmpOwner)
		{
			if (_variables)
			{
				return _variables->get_value<Class>(_byName, _default);
			}
			if (_wmpOwner)
			{
				return _wmpOwner->get_value<Class>(_byName, _default);
			}
			an_assert(false);
			return _default;
		}

		static void split_multi_lines(REF_ List<String> & _list);
	};
};
