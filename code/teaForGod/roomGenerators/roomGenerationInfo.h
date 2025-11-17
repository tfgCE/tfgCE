#pragma once

#include "..\teaEnums.h"

#include "..\..\core\math\math.h"
#include "..\..\core\types\option.h"
#include "..\..\core\vr\iVR.h"

namespace TeaForGodEmperor
{
	/**
	 *	Struct to hold some useful information used in room generation.
	 *
	 *	Most imporant is tile size.
	 *	Assumptions we have:
	 *		play rect size is at least 2 x 1.5 (as htc vive requires)
	 *		tile count is at least 3 x 2
	 *
	 *	Some of the settings come from player preferences. It is important to set these as well
	 */
	struct RoomGenerationInfo
	{
	public:
		static float const MIN_TILE_SIZE;
		static float const MAX_TILE_SIZE;
		static float const MIN_PLAY_AREA_WIDTH;
		static float const MIN_PLAY_AREA_LENGTH;

		enum TileCountInfo
		{
			Tiny,
			Basic,
			Normal,
			Long,
			Large,
			VeryLarge
		};
	public:
		static RoomGenerationInfo const & get() { an_assert(s_globalInfo);  return *s_globalInfo; }
		static RoomGenerationInfo & access() { an_assert(s_globalInfo);  return *s_globalInfo; }
		static void initialise_static();
		static void close_static();

		RoomGenerationInfo();

		bool load_from_xml(IO::XML::Node const * _node);
		bool save_to_xml(IO::XML::Node * _node) const;

		static Range2 get_door_required_space(Transform const & _doorPlacementVRS, float _tileSize); // will calculate Range2 for two tiles in front and behind door

	public: // config
		void set_preferred_tile_size(PreferredTileSize::Type _preferredTileSize) { preferredTileSize = _preferredTileSize; }
		void set_test_play_area_rect_size(Optional<Vector2> const& _testPlayAreaRectSize = NP) { testPlayAreaRectSize = _testPlayAreaRectSize; testPlayAreaRectSizeLoaded = false; forceTestPlayAreaRectSize = false; }
		void set_test_vr_scaling(Optional<float> const& _testVRScaling = NP) { testVRScaling = _testVRScaling; }
		PreferredTileSize::Type get_preferred_tile_size() const { return preferredTileSize; }
		void set_door_height(float _doorHeight) { requestedDoorHeight = _doorHeight; }
		Tags & access_requested_generation_tags() { return requestedGenerationTags; }
		Tags const & get_requested_generation_tags() const { return requestedGenerationTags; }
		Optional<Tags> const & get_test_generation_tags() const { return testGenerationTags; }
		void set_allow_crouch(Option::Type _allowCrouch) { requestedAllowCrouch = _allowCrouch; }

		void setup_to_use(bool _output = true) { setup_internal(_output); }
		void setup_for_preview() { setup_internal(false, true); }

		bool does_require_setup() const;

		void set_requires_door_regeneration() { requiresDoorRegeneration = true; }

	public: // usage
		void modify_randomly(Random::Generator const & _generator);

		TileCountInfo get_tile_count_info() const;
		tchar const* get_tile_count_info_as_char() const;
		bool is_small() const { return tileCount.y == 2; } // this is enough, it could be wide but such short and it should be still considered small
		bool is_big() const { return min(tileCount.x, tileCount.y) >= 5; } // this is enough, it could be wide but such short and it should be still considered small

		Vector2 const & get_play_area_rect_size() const { return playAreaRectSize; }

		Vector2 const & get_play_area_offset() const { return playAreaOffset; }
		VR::Zone const & get_play_area_zone() const { return playAreaZone; }

		float const & get_tile_size() const { return tileSize; }
		VectorInt2 const & get_tile_count() const { return tileCount; }

		VR::Zone const & get_tile_zone() const { return tileZone; }
		VR::Zone const & get_tiles_zone() const { return tilesZone; }
		Vector2 const & get_first_tile_centre_offset() const { return firstTileCentreOffset; }
		Vector2 const & get_tiles_zone_offset() const { return tilesZoneOffset; }

		float get_door_height() const { return doorHeight; }
		Tags const & get_generation_tags() const { return generationTags; }
		bool does_allow_crouch() const { return allowCrouch; }
		bool does_allow_any_crouch() const { return allowAnyCrouch; }

		void limit_to(Vector2 const & _maxSize, Optional<float> const & _forceTileSize, Random::Generator const & _generator);

		bool was_play_area_enlarged() const { return playAreaEnlarged; }
		bool was_play_area_forced_scaled() const { return playAreaForcedScaled; }

	private:
		static RoomGenerationInfo* s_globalInfo;

		// config
		PreferredTileSize::Type preferredTileSize = PreferredTileSize::Auto;
		Optional<float> forceTileSize;

		Optional<float> testTileSize;
		Optional<float> testDoorHeight;
		Optional<Tags> testGenerationTags;
		Optional<bool> testAllowCrouch;
		Optional<bool> testAllowAnyCrouch;

		bool requiresDoorRegeneration = false;

		bool testVR = false;
		Optional<Vector2> testPlayAreaRectSize;
		bool testPlayAreaRectSizeLoaded = false;
		bool forceTestPlayAreaRectSize = false; // force will be done even if VR is present

		Optional<float> testVRScaling;

		float requestedDoorHeight; // this is to push door height through room generation info, this is not from room generation config but from player preferences
		Option::Type requestedAllowCrouch = Option::Auto; // allow/disallow/auto
		Tags requestedGenerationTags;

		// end of config

		Vector2 playAreaRectSize = Vector2(2.0f, 1.5f); // this is as the game sees it, includes scaling etc.
		VR::Zone playAreaZone;
		bool playAreaEnlarged = false;
		bool playAreaForcedScaled = false;

		float tileSize = MIN_TILE_SIZE; // this works also as max width for doors and space beyond doors required to fit within available vr space
		VectorInt2 tileCount = VectorInt2(3, 2);
		float doorHeight;
		bool allowCrouch; // requires a bit larger tiles
		bool allowAnyCrouch; // general setting if crouch is allowed at all (player decides, settings/preferences)
		Tags generationTags;

		VR::Zone tileZone; // one tile
		VR::Zone tilesZone; // all tiles
		Vector2 playAreaOffset; // -x -y
		Vector2 firstTileCentreOffset; // centre of first tile
		Vector2 tilesZoneOffset; // -x -y of first tile

		void setup_internal(bool _output = true, bool _preview = false); // setup for vr

		void calculate_auto_values();
		void calculate_tile_count_with_random_growing(Random::Generator const & _generator);
	};
};
