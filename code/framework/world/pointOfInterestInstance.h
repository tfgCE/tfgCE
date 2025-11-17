#pragma once

#include <functional>

#include "pointOfInterestTypes.h"

#include "..\nav\navPlacementAtNode.h"
#include "..\sound\soundSource.h"

#include "..\..\core\functionParamsStruct.h"
#include "..\..\core\math\math.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\core\other\simpleVariableStorage.h"

namespace Meshes
{
	class Mesh3DInstance;
};

namespace Framework
{
	struct LibraryLoadingContext;
	class DoorType;
	class Mesh;
	class ObjectType;
	class Room;
	class RoomType;
	class TemporaryObjectType;

	struct PointOfInterest;
	struct PointOfInterestInstance;
	struct SpaceBlockers;

	struct PointOfInterestInstance
	: public RefCountObject
	, public SafeObject<PointOfInterestInstance>
	{
	public:
		PointOfInterestInstance();
		virtual ~PointOfInterestInstance();

		void create_instance(Random::Generator & _rg, PointOfInterest * _poi, Room* _inRoom, IModulesOwner* _owner, int _poiIdx, Transform const & _placementOfOwner, Mesh const * _forMesh, Meshes::Mesh3DInstance* _forMeshInstance, Optional<bool> const & _process = NP);
		void process();
		void advance(Framework::AdvanceContext const & _advanceContext);

		bool does_require_advancement() const { return functionIntervalTimeLeft.is_set(); }

		Nav::PlacementAtNode get_nav_placement(bool _veryLazy = false);

		bool is_cancelled() const { return cancelled; }
		Name const & get_name() { return name; }
		Tags & access_tags() { return tags; }
		Tags const & get_tags() const { return tags; }
		PointOfInterest* get_poi() const { return poi.get(); }
		Room* get_room() const;
		IModulesOwner* get_owner() const;
		Transform calculate_placement() const;

		SimpleVariableStorage & access_parameters() { return parameters; }
		SimpleVariableStorage const & get_parameters() const { return parameters; }

		struct FindPlacementParams
		{
			ADD_FUNCTION_PARAM_DEF(FindPlacementParams, bool, stickToWallInAnyDir, stick_to_wall_in_any_dir, true);
			ADD_FUNCTION_PARAM(FindPlacementParams, float, stickToWallRayCastDistance, stick_to_wall_ray_cast_distance);
		};

		Optional<Transform> find_placement_for(Object* _spawnedObject, Room* _inRoom, Optional<Random::Generator> const & _useRG = NP, Optional<FindPlacementParams> const& _findPlacementParamsWithinMeshNodeBoundary = NP, Optional<FindPlacementParams> const& _findPlacementParamsNoMeshNodeBoundary = NP) const;

	public: // debug
		void debug_draw(bool _setDebugContext = true);

	protected:
		virtual void setup_function_spawn(OUT_ ObjectType * & _objectType, OUT_ TemporaryObjectType * & _temporaryObjectType) {}
		virtual void setup_function_door(Room const * _forInRoom, OUT_ DoorType * & _doorType, OUT_ RoomType * & _roomTypeBehindDoor);
		virtual void process_function(Name const& function);

		Random::Generator& access_random_generator() { return randomGenerator; }

	protected:
		bool cancelled = false;

		::SafePtr<Room> inRoom; // if owner, this is not used
		::SafePtr<IModulesOwner> owner;
		int poiIdx = NONE;
		Random::Generator randomGenerator;
		RefCountObjectPtr<PointOfInterest> poi;
		Name name; // poi name
		Tags tags; // actual tags - may get modified
		int socketIdx = NONE;
		Mesh const * forMesh = nullptr;
		Meshes::Mesh3DInstance* forMeshInstance = nullptr; // it's ok, we're keeping POIs per object that has this instance

		Transform originalPlacement = Transform::identity; // original placement (placementOfOwner of create_instance), used only if no mesh instance and no owner provided
		mutable Transform placement = Transform::identity; // placement with offset
		//
		mutable Concurrency::SpinLock placementLock = Concurrency::SpinLock(TXT("Framework.PointOfInterestInstance.placementLock"));
		mutable volatile int placementFrameId = NONE;

		SimpleVariableStorage parameters;
		SimpleVariableStorage transferableParameters; // some parameters should not go through to following objects (created as we need them)
		RefCountObjectPtr<SoundSource> playedSound;
		Optional<float> functionIntervalTimeLeft; // if set, will be processed again and again
		bool processingInProgress = false;

		Nav::PlacementAtNode navPlacement; // this is cached to know where in navmesh it is
		//
		Concurrency::SpinLock navPlacementLock = Concurrency::SpinLock(TXT("Framework.PointOfInterestInstance.navPlacementLock"));
		volatile int navPlacementFrameId = NONE;

		void update_placement();

		void cancel();

		bool check_space_blockers(Room* _useRoom, Optional<Transform> const& _placement, std::function<void(OUT_ SpaceBlockers & _spaceBlockers)> _get_space_blockers) const;

		Optional<Transform> check_placement(FindPlacementParams const& _findPlacementParams, Transform const& _placement, Optional<float> const& _forwardDistForClamping /* if not set, clamping not used */, Object* _spawnedObject, Room* _inRoom, Vector3 const& _up, Random::Generator const& _useRG, bool _keepForwardAsItIs) const;
	};

};

DECLARE_REGISTERED_TYPE(SafePtr<Framework::PointOfInterestInstance>);
