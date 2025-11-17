#pragma once

#include "energy.h"

#include "..\..\core\containers\array.h"
#include "..\..\core\types\option.h"
#include "..\..\framework\framework.h"
#include "..\..\framework\game\gameConfig.h"

namespace Framework
{
	class ModuleData;
};

namespace TeaForGodEmperor
{
	namespace AllowScreenShots
	{
		enum Type
		{
			No,
			Small,
			Big,
		};

		inline Type parse(String const & _text)
		{
			if (_text == TXT("small")) return Small;
			if (_text == TXT("big")) return Big;
			if (_text == TXT("true")) return Big;
			return No;
		}

		inline tchar const* to_char(AllowScreenShots::Type _type)
		{
			if (_type == Small) return TXT("small");
			if (_type == Big) return TXT("big");
			return TXT("no");
		}
	};

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	namespace ExternalViewShowBoundary
	{
		enum Type
		{
			No,
			FloorOnly,
			BackWalls,
			AllWalls,

			MAX
		};

		inline Type parse(String const& _text)
		{
			if (_text == TXT("no")) return No;
			if (_text == TXT("floor only")) return FloorOnly;
			if (_text == TXT("back walls")) return BackWalls;
			if (_text == TXT("all walls")) return AllWalls;
			return No;
		}

		inline tchar const* to_char(ExternalViewShowBoundary::Type _type)
		{
			if (_type == FloorOnly) return TXT("floor only");
			if (_type == BackWalls) return TXT("back walls");
			if (_type == AllWalls) return TXT("all walls");
			return TXT("no");
		}
	};
#endif

	struct GameConfig
	: public ::Framework::GameConfig
	{
		FAST_CAST_DECLARE(GameConfig);
		FAST_CAST_BASE(::Framework::GameConfig);
		FAST_CAST_END();

		typedef ::Framework::GameConfig base;
	public:
		struct SecondaryViewInfo
		{
			bool show = false; // option is always there, it might be just not used anywhere
			float mainSize = 0.7f; // how much space main view does take
			bool mainSizeAuto = false; // will fit to have the eye in a proper aspect displayed
			bool pip = false; // picture in picture (if false, will take the rest of the space)
			bool pipSwap = true; // pip is camera/eyes
			float pipSize = 0.3f;
			bool mainZoom = true; // should zoom to fill
			bool secondaryZoom = true; // should zoom to fill
			Vector2 mainAt = Vector2(0.5f, 0.5f); // where it is relatively to the screen
			Vector2 secondaryAt = Vector2(0.0f, 0.0f); // where it is relatively to the screen
		};

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
		struct ExternalViewInfo
		{
			ExternalViewShowBoundary::Type showBoundary = ExternalViewShowBoundary::No;
		};
#endif

	public:
		static void initialise_static();

		GameConfig();

		static Energy apply_health_coef(Energy const& _to, Framework::ModuleData const* _for);
		static Energy apply_health_coef_inv(Energy const& _to, Framework::ModuleData const* _for);
		static Energy apply_loot_energy_quantum_ammo(Energy const & _to);

	public:
		EnergyCoef const& get_npc_character_health_coef() const { return npcCharacterHealthCoef; } // used when initialising
		EnergyCoef const& get_loot_energy_quantum_ammo_coef() const { return lootEnergyQuantumAmmoCoef; } // used when initialising

		float get_gamepad_camera_speed() const { return gamepadCameraSpeed; }
		void set_gamepad_camera_speed(float _gamepadCameraSpeed) { gamepadCameraSpeed = _gamepadCameraSpeed; }

		bool does_show_vr_on_screen_with_info() const { return vrOnScreenWithInfo; }
		void show_vr_on_screen_with_info(bool _vrOnScreenWithInfo) { vrOnScreenWithInfo = _vrOnScreenWithInfo; }

		SecondaryViewInfo const & get_secondary_view_info() const { return secondaryViewInfo; }
		SecondaryViewInfo & access_secondary_view_info() { return secondaryViewInfo; }

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
		ExternalViewInfo const& get_external_view_info() const { return externalViewInfo; }
		ExternalViewInfo& access_external_view_info() { return externalViewInfo; }
#endif

		AllowScreenShots::Type get_allow_vr_screen_shots() const { return allowVRScreenShots; }
		void allow_vr_screen_shots(AllowScreenShots::Type _allowVRScreenShots) { allowVRScreenShots = _allowVRScreenShots; }

		Option::Type get_hub_point_with_whole_hand() const { return hubPointWithWholeHand; }
		void hub_point_with_whole_hand(Option::Type _hubPointWithWholeHand) { hubPointWithWholeHand = _hubPointWithWholeHand; }
		void next_hub_point_with_whole_hand();

		String const& get_player_profile_name() const { return playerProfileName; }
		void set_player_profile_name(String const & _playerProfileName) { playerProfileName = _playerProfileName; }

		int get_subtitles_line_limit() const { return subtitlesLineLimit;  }

	protected: // ::Framework::GameConfig
		override_ bool internal_load_from_xml(IO::XML::Node const * _node);
		override_ void internal_save_to_xml(IO::XML::Node * _container, bool _userSpecific) const;

	protected:
		String playerProfileName;

		float gamepadCameraSpeed = 0.5f;

		bool vrOnScreenWithInfo = false; // info on sides about health/energy/exms
		
		SecondaryViewInfo secondaryViewInfo;

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
		ExternalViewInfo externalViewInfo;
#endif

		int subtitlesLineLimit = 0;

#ifdef AN_DEVELOPMENT
		AllowScreenShots::Type allowVRScreenShots = AllowScreenShots::Big;
#else
		AllowScreenShots::Type allowVRScreenShots = AllowScreenShots::No;
#endif

		Option::Type hubPointWithWholeHand = Option::Auto;

		EnergyCoef npcCharacterHealthCoef = EnergyCoef::one();
		EnergyCoef lootEnergyQuantumAmmoCoef = EnergyCoef::one();
	};

};
