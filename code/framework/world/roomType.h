#pragma once

#include "..\..\core\collision\collisionInfoProvider.h"

#include "doorHole.h"
#include "doorTypeReplacer.h"
#include "doorVRCorridor.h"
#include "predefinedRoomOcclusionCulling.h"
#include "worldSetting.h"

#include "..\library\libraryStored.h"
#include "..\library\libraryStoredReplacer.h"

#include "..\..\core\wheresMyPoint\iWMPTool.h"

#include "regionGenerator.h"
#include "roomGeneratorInfo.h"

namespace Framework
{
	class EnvironmentOverlay;
	class EnvironmentType;
	class Reverb;

	namespace SoftDoorRendering
	{
		enum Type
		{
			None,
			EveryOdd,
		};

		Type parse(String const & _string);
	};

	/**
	 *	?
	 */
	class RoomType
	: public LibraryStored
	{
		FAST_CAST_DECLARE(RoomType);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		struct UseEnvironment
		{
			// will try in given order:
			// 1. from region if provided
			// 2. from first door
			// 3. by type
			// 4. fallback/empty
			Name useFromRegion;
			bool useFromFirstDoor = false;
			UsedLibraryStored<EnvironmentType> useEnvironmentType;
			Name parentEnvironment; // if useEnvironmentType is used, if not provided, no parent will be set
			Transform placement = Transform::identity;
			Name anchorPOI;
			MeshGenParam<Transform> anchor;

			float pulsePeriod = 1.0f;
			UsedLibraryStored<EnvironmentType> pulseEnvironmentType;

			void clear() { *this = UseEnvironment(); }
			bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			bool apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library = nullptr);
		};

		RoomType(Library * _library, LibraryName const & _name);

		WorldSetting const * get_setting() const { return setting.get(); }

		SimpleVariableStorage const & get_required_variables() const { return requiresVariables; }

		DoorTypeReplacer* get_door_type_replacer() const { return doorTypeReplacer.get(); }

		SoftDoorRendering::Type get_soft_door_rendering() const { return softDoorRendering; }
		int get_soft_door_rendering_offset() const { return softDoorRenderingEveryOddOffset; }
		Name const & get_soft_door_rendering_tag() const { return softDoorRenderingTag; }

		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ bool create_from_template(Library* _library, CreateFromTemplate const & _createFromTemplate) const;
		override_ void prepare_to_unload();

		RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>> get_region_piece_for_region_generator(REF_ PieceCombiner::Generator<Framework::RegionGenerator>& _generator, Region* _region, REF_ Random::Generator & _randomGenerator) const;

		PieceCombiner::Piece<RegionGenerator>* get_as_region_piece() { return regionPiece.get(); }
		PieceCombiner::Piece<RegionGenerator> const * get_as_region_piece() const { return regionPiece.get(); }

		bool should_use_fallback_generation() const { return useFallbackGeneration; }
		RoomGeneratorInfo const* get_sole_room_generator_info() const;
		RoomGeneratorInfo const * get_room_generator_info(Random::Generator& _rg) const { return roomGeneratorInfo.get(_rg); }
		RoomType const * get_door_vr_corridor_room_room_type(Random::Generator& _rg) const { return doorVRCorridor.get_room(_rg); }
		RoomType const * get_door_vr_corridor_elevator_room_type(Random::Generator& _rg) const { return doorVRCorridor.get_elevator(_rg); }
		bool get_door_vr_corridor_priority(Random::Generator& _rg, OUT_ float & _result) const { return doorVRCorridor.get_priority(_rg, OUT_ _result); }
		bool get_door_vr_corridor_priority_range(OUT_ Range & _result) const { return doorVRCorridor.get_priority_range(OUT_ _result); }

		WheresMyPoint::ToolSet const& get_wheres_my_point_processor_setup_parameters() const { return wheresMyPointProcessorSetupParameters; }
		WheresMyPoint::ToolSet const& get_wheres_my_point_processor_on_generate() const { return wheresMyPointProcessorOnGenerate; }
		WheresMyPoint::ToolSet const & get_wheres_my_point_processor_pre_generate() const { return wheresMyPointProcessorPreGenerate; }

		UseEnvironment const & get_use_environment() const { return useEnvironment; }
		Array<UsedLibraryStored<EnvironmentOverlay>> const & get_environment_overlays() const { return environmentOverlays; }

		Reverb* get_reverb() const { return useReverb.get(); }

		LibraryStoredReplacer const & get_library_stored_replacer() const { return libraryStoredReplacer; }

		Collision::CollisionInfoProvider const & get_collision_info_provider() const { return collisionInfoProvider; }

		bool should_have_nav_mesh() const { return shouldHaveNavMesh; }
		bool should_be_avoided_to_navigate_into() const { return shouldBeAvoidedToNavigateInto; }

		PredefinedRoomOcclusionCulling const& get_predefined_occlusion_culling() const { return predefinedRoomOcclusionCulling; }

	protected:
		virtual ~RoomType();

	private:
		SimpleVariableStorage requiresVariables;

		// soft door rendering (without setting depth stencil, clearing everything, closing, etc), this is for inbound door
		SoftDoorRendering::Type softDoorRendering;
		int softDoorRenderingEveryOddOffset; // as it is for inbounds, first candidates always have depth = 1 (odd, but we can offset that with this value)
		Name softDoorRenderingTag;

		WheresMyPoint::ToolSet wheresMyPointProcessorSetupParameters; // to allow setup of parameters randomly (!)
		WheresMyPoint::ToolSet wheresMyPointProcessorPreGenerate; // to allow modification of environment etc at the very beginning of generation
		WheresMyPoint::ToolSet wheresMyPointProcessorOnGenerate; // to allow modification of parameters when generating room

		WorldSettingPtr setting;
		CopyWorldSettings copySettings;

		DoorTypeReplacerPtr doorTypeReplacer; // door type replacer that is specific to this room, if room generator provides different one (it might be via region), it is used unless the roomType's has a higher priority

		UseEnvironment useEnvironment;
		Array<UsedLibraryStored<EnvironmentOverlay>> environmentOverlays;

		UsedLibraryStored<Reverb> useReverb;

		LibraryStoredReplacer libraryStoredReplacer;

		RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>> regionPiece; // might be overridden/provided by roomGeneratorInfo (for example the room requires multiple exits and tells how many exits does it have)

		bool useFallbackGeneration;
		RoomGeneratorInfoSet roomGeneratorInfo;
		DoorVRCorridor doorVRCorridor;

		Collision::CollisionInfoProvider collisionInfoProvider;

		bool shouldHaveNavMesh = true;
		bool shouldBeAvoidedToNavigateInto = false;

		PredefinedRoomOcclusionCulling predefinedRoomOcclusionCulling;
	};
};
