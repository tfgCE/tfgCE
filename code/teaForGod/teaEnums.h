#pragma once

#include "..\core\globalDefinitions.h"
#include "..\core\types\optional.h"
#include "..\core\types\string.h"

namespace TeaForGodEmperor
{
	// to be used with game_related_system_flags of Framework::IModulesOwner
	namespace ModulesOwnerFlags
	{
		enum Flag
		{
			BlockSpawnManagerAutoCease = 1
		};
	};

	// to be used with game_related_system_flags of Framework::Door
	namespace DoorFlags
	{
		enum Flag
		{
			AppearLockedIfNotOpen = 1
		};
	};

	namespace HitIndicatorType
	{
		enum Type
		{
			Off,
			Full, // reference only, I guess
			StrongerTowardsHitSource, // will turn display red when looking at hit source
			WeakerTowardsHitSource, // some red voxels are visible when looking at hit source but most are be visible when looking away
			SidesOnly, // only sides (as combination of two above
			NotAtHitSource, // almost no red voxels are visible when looking right at hit source, on sides there is plenty
		};

		inline static tchar const * to_char(Type _type)
		{
			if (_type == Off) return TXT("off");
			if (_type == Full) return TXT("full");
			if (_type == StrongerTowardsHitSource) return TXT("stronger at HS");
			if (_type == WeakerTowardsHitSource) return TXT("weaker at HS");
			if (_type == SidesOnly) return TXT("sides only");
			if (_type == NotAtHitSource) return TXT("not at HS");
			return TXT("??");
		}

		inline static Type parse(String const & _type, HitIndicatorType::Type _default = HitIndicatorType::SidesOnly)
		{
			if (_type == TXT("off")) return Off;
			if (_type == TXT("full")) return Full;
			if (_type == TXT("stronger at HS")) return StrongerTowardsHitSource;
			if (_type == TXT("weaker at HS")) return WeakerTowardsHitSource;
			if (_type == TXT("sides only")) return SidesOnly;
			if (_type == TXT("not at HS")) return NotAtHitSource;
			if (_type == TXT("default")) return _default;
			if (_type.is_empty()) return _default;
			error(TXT("hit indicator type \"%S\" not recognised"), _type.to_char());
			return _default;
		}
	};
	
	namespace DoorDirectionsVisible
	{
		enum Type
		{
			Auto,
			Above,
			AtEyeLevel
		};

		inline static tchar const * to_char(Type _type)
		{
			if (_type == Auto) return TXT("auto");
			if (_type == Above) return TXT("above");
			if (_type == AtEyeLevel) return TXT("at eye level");
			return TXT("??");
		}

		inline static Type parse(String const & _type, DoorDirectionsVisible::Type _default = DoorDirectionsVisible::Auto)
		{
			if (_type == TXT("auto")) return Auto;
			if (_type == TXT("above")) return Above;
			if (_type == TXT("at eye level")) return AtEyeLevel;
			if (_type.is_empty()) return _default;
			error(TXT("door directions visible type \"%S\" not recognised"), _type.to_char());
			return _default;
		}
	};

	namespace DoorDirectionObjectiveOverride
	{
		enum Type
		{
			None,
			TowardsEnergyAmmo
		};

		inline Type parse(String const& _text)
		{
			if (_text == TXT("towards energy ammo")) return TowardsEnergyAmmo;
			return None;
		}
	};

	namespace EnergyType
	{
		enum Type
		{
			None = 0,
			Ammo = 1,
			Health = 2,

			MAX,

			First = Ammo,
			Last = Health,
		};

		inline static Type parse(String const & _type, EnergyType::Type _default = EnergyType::None)
		{
			if (_type == TXT("none")) return None;
			if (_type == TXT("ammo")) return Ammo;
			if (_type == TXT("health")) return Health;
			if (_type.is_empty()) return _default;
			error(TXT("energy type \"%S\" not recognised"), _type.to_char());
			return _default;
		}

		inline static tchar const * to_char(EnergyType::Type _et)
		{
			if (_et == None) return TXT("none");
			if (_et == Ammo) return TXT("ammo");
			if (_et == Health) return TXT("health");
			error(TXT("energy type (%i) not handled"), _et);
			return TXT("??");
		}
	};

	namespace DamageType
	{
		enum Type
		{
			Unknown,
			Plasma,
			Ion,
			Corrosion,
			Lightning,
			//
			Physical,
			EnergyQuantumSideEffects,
			Gameplay,					// related to gameplay systems
		};

		inline static Type parse(String const& _type, DamageType::Type _default = DamageType::Unknown)
		{
			if (_type == TXT("plasma")) return Plasma;
			if (_type == TXT("ion")) return Ion;
			if (_type == TXT("lightning")) return Lightning;
			if (_type == TXT("corrosion")) return Corrosion;
			if (_type == TXT("physical")) return Physical;
			if (_type == TXT("energy quantum side effects")) return EnergyQuantumSideEffects;
			if (_type.is_empty()) return _default;
			error(TXT("energy type \"%S\" not recognised"), _type.to_char());
			return _default;
		}
	};

	struct Guidance
	{
		int temp;
		enum Type
		{
			NoGuidance,
			HotCold,
			Strength,
			Directional,
			MAX
		};

		inline static tchar const* to_char(Type _type)
		{
			if (_type == NoGuidance) return TXT("no guidance");
			if (_type == HotCold) return TXT("hot cold");
			if (_type == Strength) return TXT("strength");
			if (_type == Directional) return TXT("directional");
			return TXT("??");
		}

		inline static Type parse(String const& _type, Guidance::Type _default = Guidance::NoGuidance)
		{
			if (_type == TXT("no guidance")) return NoGuidance;
			if (_type == TXT("hot cold")) return HotCold;
			if (_type == TXT("strength")) return Strength;
			if (_type == TXT("directional")) return Directional;
			if (_type.is_empty()) return _default;
			error(TXT("guidance \"%S\" not recognised"), _type.to_char());
			return _default;
		}
	};

	struct PreferredTileSize
	{
		int temp;
		enum Type
		{
			Auto,
			Smallest,
			Medium,
			Large,
			Largest,
			MAX
		};

		inline static tchar const* to_char(Type _type)
		{
			if (_type == Auto) return TXT("auto");
			if (_type == Smallest) return TXT("smallest");
			if (_type == Medium) return TXT("medium");
			if (_type == Large) return TXT("large");
			if (_type == Largest) return TXT("largest");
			return TXT("??");
		}

		inline static Type parse(String const& _type, Type _default = Auto)
		{
			if (_type == TXT("auto")) return Auto;
			if (_type == TXT("smallest")) return Smallest;
			if (_type == TXT("medium")) return Medium;
			if (_type == TXT("large")) return Large;
			if (_type == TXT("largest")) return Largest;
			if (_type.is_empty()) return _default;
			error(TXT("preferred tile size \"%S\" not recognised"), _type.to_char());
			return _default;
		}
	};

	namespace WorldSeparatorReason
	{
		enum Flag
		{
			NotDefined		= 0,
			CellSeparator	= bit(0),
		};
	};

	namespace PlayerVRMovementRelativeTo
	{
		enum Type
		{
			Head,
			LeftHand,
			RightHand
		};

		inline static tchar const* to_char(Type _type)
		{
			if (_type == Head) return TXT("head");
			if (_type == LeftHand) return TXT("left hand");
			if (_type == RightHand) return TXT("right hand");
			return TXT("??");
		}

		inline static Type parse(String const& _type, Type _default = Head)
		{
			if (_type == TXT("head")) return Head;
			if (_type == TXT("left hand")) return LeftHand;
			if (_type == TXT("right hand")) return RightHand;
			error(TXT("vr movement relative to \"%S\" not recognised"), _type.to_char());
			return _default;
		}
	};

	namespace PilgrimageResult
	{
		enum Type
		{
			Unknown,
			Interrupted,
			DemoEnded,
			Ended,
			PilgrimDied,
		};
	};

	namespace GameSlotMode
	{
		enum Type
		{
			Story,
			Missions
		};

		inline static tchar const* to_char(Type _type)
		{
			if (_type == Story) return TXT("story");
			if (_type == Missions) return TXT("missions");
			return TXT("??");
		}

		inline static Type parse(String const& _type, Type _default = Story)
		{
			if (_type == TXT("story")) return Story;
			if (_type == TXT("missions")) return Missions;
			if (!_type.is_empty())
			{
				error(TXT("game slot mode \"%S\" not recognised"), _type.to_char());
			}
			return _default;
		}
	};
};

template <> bool Optional<TeaForGodEmperor::DamageType::Type>::load_from_xml(IO::XML::Node const* _node);
template <> bool Optional<TeaForGodEmperor::DamageType::Type>::load_from_xml(IO::XML::Attribute const* _attr);
