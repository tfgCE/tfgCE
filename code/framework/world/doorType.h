#pragma once

#include "doorHole.h"
#include "doorVRCorridor.h"

#include "..\appearance\generatedMeshDeterminants.h"
#include "..\library\libraryStored.h"

#include "regionGenerator.h"

namespace Framework
{
	class DoorInRoom;
	class DoorType;
	class RoomPart;
	class Mesh;
	class MeshGenerator;
	class Room;
	class Sample;
	class ShaderProgram;
	struct DoorGeneratedMeshSet;
	struct SpaceBlockers;

	namespace DoorOperation
	{
		// for DoorType only auto makes sense and is default one
		// other options are added to allow door use DoorOperation
		// they might be used by game specific code
		enum Type
		{
			Unclosable, // if unclosable, will always remain open and won't advance
			StayOpen,
			StayClosed,
			StayClosedTemporarily, // is closed for a brief period of time and should open soon (may even appear that is not locked), should be set only with code
			Auto,
			AutoEagerToClose
		};

		Type parse(String const & _string);
		inline tchar const *to_char(Type _type)
		{
			if (_type == Unclosable) return TXT("unclosable");
			if (_type == StayOpen) return TXT("stay open");
			if (_type == StayClosed) return TXT("stay closed");
			if (_type == StayClosedTemporarily) return TXT("stay closed temporarily");
			if (_type == Auto) return TXT("auto");
			if (_type == AutoEagerToClose) return TXT("auto eager to close");
			return TXT("--");
		}

		inline bool is_likely_to_be_open(Type _type)
		{
			return _type != StayClosed;
		}
	};

	struct DoorSounds
	{
	public:
		bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		void load_data_on_demand_if_required();

		Sample* get_open_sound_sample() const { return openSoundSample.get(); }
		Sample* get_opening_sound_sample() const { return openingSoundSample.get(); }
		Sample* get_opened_sound_sample() const { return openedSoundSample.get(); }
		Sample* get_close_sound_sample() const { return closeSoundSample.get(); }
		Sample* get_closing_sound_sample() const { return closingSoundSample.get(); }
		Sample* get_closed_sound_sample() const { return closedSoundSample.get(); }

	private:
		UsedLibraryStored<Sample> openSoundSample; // always play whole
		UsedLibraryStored<Sample> openingSoundSample; // will be interrupted
		UsedLibraryStored<Sample> openedSoundSample; // always play whole
		UsedLibraryStored<Sample> closeSoundSample;
		UsedLibraryStored<Sample> closingSoundSample;
		UsedLibraryStored<Sample> closedSoundSample;
	};

	struct DoorGeneratedMesh
	{
		GeneratedMeshDeterminants forDeterminants;
		UsedLibraryStored<Mesh> mesh;
	private:
		friend struct DoorGeneratedMeshSet;
		void generate_mesh_for(MeshGenerator* _meshGenerator, DoorType const* _doorType, Name const& meshGeneratorRequestedNominalDoorWidthParamName, Name const& meshGeneratorRequestedNominalDoorHeightParamName);
	};

	struct DoorGeneratedMeshSet
	{
		mutable Concurrency::MRSWLock meshesLock;
		List<DoorGeneratedMesh> meshes;

		Mesh* get_mesh_for(GeneratedMeshDeterminants const& _determinants) const;
		Mesh* generate_mesh_for(GeneratedMeshDeterminants const& _determinants, MeshGenerator* _meshGenerator, DoorType const* _doorType, Name const& meshGeneratorRequestedNominalDoorWidthParamName, Name const& meshGeneratorRequestedNominalDoorHeightParamName);
		void generate_all_meshes_for(MeshGenerator* _meshGenerator, DoorType const* _doorType, Name const& meshGeneratorRequestedNominalDoorWidthParamName, Name const& meshGeneratorRequestedNominalDoorHeightParamName);
	};

	struct DoorWingType
	{
		Name id;
		DoorType* doorType = nullptr;
		bool asymmetrical = false; // if asymmetrical, apply side placement will be done to have one side reversed, so both match when combined
		Transform placement; // relative to outbound matrix
		Vector3 slideOffset; // relative to outbound matrix
		Range mapOpenPt = Range(0.0f, 1.0f); // maps open to movement
		Optional<Vector2> nominalDoorSize; // for offset and placement etc
		PlaneSet clipPlanes; // relative to outbound matrix
		ArrayStatic<Name, 6> autoClipPlanes; // use forward dir as plane normal

		DoorWingType();
		bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc, Array<DoorWingType> const & _wings);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		void load_data_on_demand_if_required();

		Mesh* get_mesh_for(DoorInRoom const* _doorInRoom) const;

		DoorSounds const& get_sounds() const { return sounds; }

	private:
		UsedLibraryStored<Mesh> mesh;
		UsedLibraryStored<MeshGenerator> meshGenerator;
		mutable DoorGeneratedMeshSet generatedMeshes;
		Name meshGeneratorRequestedNominalDoorWidthParamName;
		Name meshGeneratorRequestedNominalDoorHeightParamName;

		DoorSounds sounds;

	private: friend class DoorType;
		bool generate_mesh(Library* _library);
	};

	struct GeneralDoorTypeParameters
	{
		Concurrency::MRSWLock lock;
		SimpleVariableStorage parameters;
	};

	/*
	 *	Door type
	 *
	 *	Door hole is always clockwise in inbound matrix
	 *
	 *	As room piece by default it is placed at 0,0,0 facing north (hole will be placed inside that space)
	 *	This means that we look at the hole from inside - from 0,-10,0
	 *	Connectors in rooms should be placed facing direction in which door goes (like they would be going through)
	 *	For example: if we have square room with centre at 0,0,0, if we have:
	 *		door at 2,0,0, yaw should be 90
	 *		door at 0,2,0  yaw should be 0
	 *		door at 0,-2,0 yaw should be 180
	 *	?? But as it is rarely used (except for adding extra meshes) it's placement isn't even required
	 *
	 *	?? Requires room part to be defined if it should be used with room generator
	 *
	 *	Nominal door width (height too)
	 *		To allow adjusting door width to given conditions (vr) doors allow and encourage using mesh generators
	 *		By default mesh generator param is requestedNominalDoorWidth. Mesh generator should handle this parameter to create door properly.
	 *		Door hole is also adjustable, although this happens automatically - user has to provide nominalDoorWidth and hole will be scaled (along x axis)
	 */
	class DoorType
	: public LibraryStored
	{
		FAST_CAST_DECLARE(DoorType);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		static void initialise_static();
		static void close_static();

		DoorType(Library * _library, LibraryName const & _name);

		DoorHole const * get_hole() const { return &hole; }

		bool is_fake_one_way_window() const { return fakeOneWayWindow; }
		
		bool is_relevant_for_movement() const { return relevantForMovement; }
		bool is_relevant_for_vr() const { return relevantForVR; }

		PieceCombiner::Connector<RegionGenerator>* get_as_region_connector() { return &regionConnector; }

		RoomPart const* get_room_part() const { return roomPart.get(); }

		bool does_block_environment_propagation() const { return blocksEnvironmentPropagation; }

		bool can_be_off_floor() const { return canBeOffFloor; }
		
		bool is_nav_connector() const { return isNavConnector; }

		Mesh const* get_frame_mesh_for(DoorInRoom const* _doorInRoom, GeneratedMeshDeterminants const & _determinants) const;
		Transform const & get_frame_mesh_placement() const { return frameMeshPlacement; }
		bool is_frame_as_half() const { return halfFrame; }
		
		SimpleVariableStorage const & get_mesh_generator_parameters() const { return meshGeneratorParameters; }
		GeneratedMeshDeterminants const& get_generated_mesh_determinants() const { return generatedMeshesDeterminants; }

		DoorSounds const& get_sounds() const { return sounds; }

		bool has_wings() const { return ! wings.is_empty(); }
		Array<DoorWingType> const & get_wings() const { return wings; }
		float get_wings_opening_time() const { return wingsOpeningTime; }
		float get_wings_closing_time() const { return wingsClosingTime; }

		bool can_see_through_when_closed() const { return canSeeThroughWhenClosed; }
		bool can_open_in_two_directions() const { return canOpenInTwoDirections; }
		Curve<float> const & get_sound_hearable_when_open() const { return soundHearableWhenOpen; }
		bool does_ignore_sound_distance_beyond_first_room() const { return ignoreSoundDistanceBeyondFirstRoom; }

		bool can_be_closed() const { return canBeClosed; }
		DoorOperation::Type get_operation() const { return operation; }
		Optional<float> get_auto_close_delay() const { return autoCloseDelay; }
		float get_auto_operation_distance() const { return autoOperationDistance; }
		TagCondition const& get_auto_operation_tag_condition() const { return autoOperationTagCondition; }

		bool is_replacable_by_door_type_replacer() const { return replacableByDoorTypeReplacer; }

		float calculate_vr_width() const;
		Range2 calculate_size(Optional<DoorSide::Type> const& _side = NP, Optional<Vector2> const & _scaleDoorSize = NP) const;
		Range2 calculate_compatible_size(Optional<DoorSide::Type> const& _side = NP, Optional<Vector2> const& _scaleDoorSize = NP) const;

		RoomType const* get_door_vr_corridor_room_room_type(Random::Generator& _rg) const { return doorVRCorridor.get_room(_rg); }
		RoomType const* get_door_vr_corridor_elevator_room_type(Random::Generator& _rg) const { return doorVRCorridor.get_elevator(_rg); }
		bool get_door_vr_corridor_priority(Random::Generator& _rg, OUT_ float& _result) const { return doorVRCorridor.get_priority(_rg, OUT_ _result); }

		bool does_have_space_blockers() const { return withSpaceBlockers; }
		bool is_space_blocker_smaller_for_requires_to_be_empty() const { return spaceBlockerSmallerRequiresToBeEmpty; }

		void create_space_blocker(REF_ SpaceBlockers& _spaceBlockers, Optional<Transform> const & _at = NP, Optional<DoorSide::Type> const& _side = NP, Optional<Vector2> const& _scaleDoorSize = NP) const;

		ShaderProgram * get_door_hole_shader_program() const { return doorHoleShaderProgram.get(); }

		void load_data_on_demand_if_required();

	public:
		bool should_use_height_from_width() const { return heightFromWidth; }

	public:
		DoorType* process_for_spawning_through_poi(Room const* _forInRoom, Random::Generator & _rg);

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ void prepare_to_unload();

	public:
		void generate_meshes();
		static void generate_all_meshes();

	protected:
		~DoorType();

	private:
		DoorHole hole;
		Range2 compatibleSize = Range2::empty; // this is similar to hole's points but is used to get common corridor height for two doors as corridors should most likely keep same doors but
											   // in some cases we may have smaller doors at the ends and large in the middle, that's when compatible size comes in handy
		bool fakeOneWayWindow = false; // can't move through it, can only look from A to B, not the other way around
		bool relevantForMovement = true; // if set to false, will be ignored for nav etc. can't move through it etc
		bool relevantForVR = true; // if set to false, vr placement will be ignored
		PieceCombiner::Connector<RegionGenerator> regionConnector; // to make it possible to add door type to generator - although this maybe could be solved differently? maybe region generator should have door slots?
		UsedLibraryStored<Mesh> frameMesh; // frame mesh, placed at outbound which means that if halfFrame is set, mesh visible in room is taken from front of XZ plane (Y negative). for not halfFrame Yneg is side A, Ypos is side B
		UsedLibraryStored<MeshGenerator> frameMeshGenerator; // frame mesh, placed at outbound which means that if halfFrame is set, mesh visible in room is taken from front of XZ plane (Y negative). for not halfFrame Yneg is side A, Ypos is side B
		mutable DoorGeneratedMeshSet generatedFrameMeshes;
		GeneratedMeshDeterminants generatedMeshesDeterminants; // variables by what each generated mesh is identified (and default values)
		SimpleVariableStorage meshGeneratorParameters;
		Name frameMeshGeneratorRequestedNominalDoorWidthParamName; // nominal width, door should be no wider than this (including space around hole)
		Name frameMeshGeneratorRequestedNominalDoorHeightParamName; // nominal height, door should be no wider than this (including space around hole)
		DoorSounds sounds;
		Transform frameMeshPlacement = Transform::identity;
		bool halfFrame = false; // if half (oneSide), it is reversed for both sides - has to be symmetrical, note that for wings you can still use placementA and placementB
		Array<DoorWingType> wings;
		float wingsOpeningTime = 0.0f; // if 0 and there is animation, it is taken from animation
		float wingsClosingTime = 0.0f; // as above or if still 0, taken from opening
		bool canSeeThroughWhenClosed = false;
		bool canOpenInTwoDirections = false;
		Curve<float> soundHearableWhenOpen; // how much sound is hearable when door is closed/open (if 0, sounds do not propagate)
		bool ignoreSoundDistanceBeyondFirstRoom = false; // if this is set to true, distance-wise it is treated as in the same room
		bool withSpaceBlockers = true; // false to have them off
		bool spaceBlockerSmallerRequiresToBeEmpty = false; // if true, will make smaller "requires to be empty", will still stick out a bit
		float spaceBlockerExtraWidth = 0.0f; // will add to the width a bit
		UsedLibraryStored<ShaderProgram> doorHoleShaderProgram;

		bool canBeOffFloor = false; // for windows

		bool isNavConnector = true;
		bool replacableByDoorTypeReplacer = true;
		DoorOperation::Type operation = DoorOperation::Auto;
		bool canBeClosed = false;
		float autoOperationDistance = 0.7f;
		TagCondition autoOperationTagCondition;
		Optional<float> autoCloseDelay;

		DoorVRCorridor doorVRCorridor;

		struct VRDoorWidth
		{
			// (actual nominal / nominal) * width + add
			float nominal = 1.0f; // width in vr space, if zero, not used
			float width = 1.0f; // width and also width in which we define hole
			float add = 0.0f;
		} vrDoorWidth; // used when creating corridors etc
		bool heightFromWidth = false;

		UsedLibraryStored<RoomPart> roomPart;
		RefCountObjectPtr<RoomPart> ownRoomPart;

		// POIs may want to replace certain door types under given circumstances (like if a room is already inaccessible, drop door bars)
		struct ForSpawnedThroughPOI
		{
			float chance = 1.0f;
			TagCondition roomTagged;
			UsedLibraryStored<DoorType> useDoorType;
		};
		Array<ForSpawnedThroughPOI> forSpawnedThroughPOI;

		bool blocksEnvironmentPropagation = false;

		bool generate_mesh(Library* _library);

		friend struct DoorWingType;
		friend struct DoorGeneratedMesh;
		friend struct DoorGeneratedMeshSet;
	};
};
