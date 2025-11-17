#pragma once

#include "module.h"

#include "..\collision\againstCollision.h"
#include "..\jobSystem\jobSystemsImmediateJobPerformer.h"

#include "..\..\core\collision\collisionFlags.h"
#include "..\..\core\collision\model.h"
#include "..\..\core\collision\modelInstanceSet.h"
#include "..\..\core\collision\queryPrimitivePoint.h"
#include "..\..\core\collision\queryPrimitiveSegment.h"

namespace Concurrency
{
	class JobExecutionContext;
};

namespace Framework
{
	class CollisionModel;
	class DoorInRoom;
	class ModuleCollisionData;
	class PhysicalMaterial;
	class PresenceLink;

	struct CheckCollisionContext;

	struct CollisionInfo
	{
		SafePtr<Room> collidedInRoom;
		PresenceLink const * presenceLink = nullptr; // will be cleared between valid frames!
		PhysicalMaterial const * material = nullptr;
		Optional<Vector3> collisionLocation;
		Optional<Vector3> collisionNormal;
		SafePtr<Collision::ICollidableObject> collidedWithObject;
		Collision::ICollidableShape const * collidedWithShape = nullptr; // will be cleared between valid frames!
	};

	namespace ShouldIgnoreCollisionWithResult
	{
		enum Type
		{
			Collide = 0,
			Ignore = 1,
			IgnoreButCheck = 2 // ignore collision itself but check if colliding
		};
	};

	struct DetectedCollidable
	{
		Collision::ICollidableObject const * ico = nullptr;
		float distance;

		DetectedCollidable() {}
		DetectedCollidable & for_object(Collision::ICollidableObject const * _ico) { ico = _ico; return *this; }
		DetectedCollidable & at_distance(float _distance) { distance = _distance; return *this; }
		
		template <typename Container>
		static bool does_contain(Container const & _container, Collision::ICollidableObject const* _ico)
		{
			for_every(ob, _container)
			{
				if (ob->ico == _ico)
				{
					return true;
				}
			}

			return false;
		}

		bool operator==(DetectedCollidable const & _other) const { return _other.ico == ico; }
	};

	class ModuleCollision
	: public Module
	{
		FAST_CAST_DECLARE(ModuleCollision);
		FAST_CAST_BASE(Module);
		FAST_CAST_END();

		typedef Module base;
	public:
		static RegisteredModule<ModuleCollision> & register_itself();

		ModuleCollision(IModulesOwner* _owner);
		virtual ~ModuleCollision();

		Framework::PhysicalMaterial* get_movement_collision_material() const { return movementCollisionMaterial.get(); }
		Framework::PhysicalMaterial* get_precise_collision_material() const { return preciseCollisionMaterial.get(); }

		void update_collision_model_and_query();

		bool should_allow_collisions_with_self() const { return allowCollisionsWithSelf; }

		float get_max_radius() const { return max(get_collision_primitive_radius(), max(get_radius_for_gradient(), get_radius_for_detection())); }

		Vector3 const & get_gradient() const { return collisionGradient;  }
		Vector3 const & get_stiff_gradient() const { return collisionStiffGradient;  }
		float get_radius_for_gradient() const { return radiusForGradientOverride.is_set()? radiusForGradientOverride.get() : (radiusForGradient == 0.0f? get_collision_primitive_radius() : radiusForGradient); }
		void set_radius_for_gradient_override(Optional<float> _override = NP) { radiusForGradientOverride = _override; }
		float get_collision_primitive_radius() const { return collisionPrimitiveRadius; }
		Vector3 const & get_collision_primitive_centre_distance() const { return collisionPrimitiveCentreDistance; }
		Vector3 const & get_collision_primitive_centre_offset() const { return collisionPrimitiveCentreOffset; }

		bool calc_closest_point_on_primitive_os(REF_ Vector3& _locOS) const; // false if not using primitive

		float get_radius_for_detection() const { return radiusForDetectionOverride.get(radiusForDetection); }
		void set_radius_for_detection_override(Optional<float> const & _radiusForDetectionOverride = NP) { radiusForDetectionOverride = _radiusForDetectionOverride; update_should_update_collisions(); }

		float calc_distance_from_primitive_os(Vector3 const & _locOS, bool _subtractRadius = true) const;
		
		Name const& find_closest_bone(Vector3 const& _locOS, AgainstCollision::Type _againstCollision) const;

		float get_mass() const { return mass; }

		bool does_enforce_stiff_collisions() const { return enforceStiffCollisions; }

		// after modifying collides with or collision flags (via access or push/pop), remember to call update_collidable_object
		Collision::Flags & access_collides_with_flags() { an_assert(! is_update_collidable_object_required()); return collidesWithFlagsStack.get_last().flags; }
		Collision::Flags & access_detects_flags() { an_assert(!is_update_collidable_object_required()); return detectsFlagsStack.get_last().flags; }
		Collision::Flags & access_collision_flags() { an_assert(!is_update_collidable_object_required()); return collisionFlagsStack.get_last().flags; }
		Collision::Flags const & get_collides_with_flags() const { an_assert(!is_update_collidable_object_required()); return collidesWithFlagsStack.get_last().flags; }
		Collision::Flags const & get_detects_flags() const { an_assert(!is_update_collidable_object_required()); return detectsFlagsStack.get_last().flags; }
		Collision::Flags const & get_collision_flags() const { an_assert(!is_update_collidable_object_required()); return collisionFlagsStack.get_last().flags; }

	public:
		void set_play_collision_sound(bool _playCollisionSound) { playCollisionSound = _playCollisionSound; }

	private:
		void push_collides_with_flags();
		void push_collides_with_flags(Collision::Flags const & _flags);
		void pop_collides_with_flags();
		void push_detects_flags();
		void push_detects_flags(Collision::Flags const & _flags);
		void pop_detects_flags();
		void push_collision_flags();
		void push_collision_flags(Collision::Flags const & _flags);
		void pop_collision_flags();
	public:
		// push at the end of the stack but pop may remove from the middle, it is called "pop" just to have push/pop pair
		void push_collision_flags(Name const & _id, Collision::Flags const & _flags);
		void pop_collision_flags(Name const & _id);
		void push_collides_with_flags(Name const & _id, Collision::Flags const & _flags);
		void pop_collides_with_flags(Name const & _id);
		void push_detects_flags(Name const & _id, Collision::Flags const & _flags);
		void pop_detects_flags(Name const & _id);
		void update_collidable_object();
#ifdef AN_DEVELOPMENT
		bool is_update_collidable_object_required() const { return updateCollidableObjectRequired; }
#else
		bool is_update_collidable_object_required() const { return false; }
#endif

		Collision::Flags const & get_ik_trace_flags() const { return ikTraceFlags; }
		Collision::Flags const & get_ik_trace_reject_flags() const { return ikTraceRejectFlags; }
		Collision::Flags const & get_presence_trace_flags() const { return presenceTraceFlags; }
		Collision::Flags const & get_presence_trace_reject_flags() const { return presenceTraceRejectFlags; }

		void update_bounding_box();
		void update_on_final_pose(Meshes::Pose const * _poseMS);

		Collision::ModelInstanceSet const & get_against_collision(AgainstCollision::Type _againstCollision) const { return _againstCollision == AgainstCollision::Movement ? get_movement_collision() : get_precise_collision(); }

		Collision::ModelInstanceSet const & get_movement_collision() const { return movementCollision; }
		Collision::ModelInstanceSet const & get_precise_collision() const { return preciseCollision.is_empty() ? (movementCollision.is_empty()? fallbackPreciseCollision : movementCollision) : preciseCollision; }

		bool is_nav_obstructor() const { return navObstructor; }
		bool is_nav_obstructor_only_root() const { return navObstructor && navObstructorOnlyRoot; }

	public:
		float get_collision_primitive_height() const;
		void update_collision_primitive_height(float _height); // and update collision primitive

	public: // runtime
		void clear_dont_collide_with() { dontCollideWith.clear(); dontCollideWithUntilNoCollision.clear(); dontCollideWithIfAttachedTo.clear(); }
		void clear_dont_collide_with(Collision::ICollidableObject const * _object) { dontCollideWith.remove_fast(_object); }
		void dont_collide_with(Collision::ICollidableObject const * _object, Optional<float> const & _forTime = NP);
		void dont_collide_with_up_to_top_instigator(IModulesOwner const * _object, Optional<float> const & _forTime = NP); // should be stored only by with who did we collide
		void dont_collide_with_up_to_top_attached_to(IModulesOwner const * _object, Optional<float> const & _forTime = NP); // should be stored only by with who did we collide
		void dont_collide_with_until_no_collision(Collision::ICollidableObject const * _object, Optional<float> const & _forTimeDontRemove = NP);
		void dont_collide_with_until_no_collision_up_to_top_instigator(IModulesOwner const * _object, Optional<float> const & _forTimeDontRemove = NP); // should be stored only by with who did we collide
		void dont_collide_with_if_attached_to(IModulesOwner const * _object, Optional<float> const & _forTimeDontRemove = NP);
		void dont_collide_with_if_attached_to_top(IModulesOwner const * _object, Optional<float> const & _forTimeDontRemove = NP); // will get the top of attached

		ShouldIgnoreCollisionWithResult::Type should_ignore_collision_with(IModulesOwner const * _object) const; // checks attachements and dontCollideWith
		bool should_collide_with(IModulesOwner const * _object) const { return should_ignore_collision_with(_object) == ShouldIgnoreCollisionWithResult::Collide; }

		void add_not_colliding_with_to(CheckCollisionContext & _ccc) const;

		bool should_prevent_pushing_through_collision() const; // checks if flag is set and if collides with such an object
		void prevent_future_current_pushing_through_collision(Optional<float> const & _forTime = NP); // for all pushers, ignore them until we have no collision with them

	public: // public for CheckGradient
		void store_collided_with(CollisionInfo const & _result);
		void store_detected_collision_with(Collision::ICollidableObject const * _ico);
		void store_detected(DetectedCollidable const & _dc);

	public:
		Array<CollisionInfo> const & get_collided_with() const { return collidedWith; }
		bool does_collide_with(Collision::ICollidableObject const * _collidable) const;
		bool is_colliding_with_non_scenery() const { return collidingWithNonScenery; }

		Array<DetectedCollidable> const & get_detected() const { return detected; }

	public:
		bool should_update_collisions() const { return shouldUpdateCollisions || !get_collides_with_flags().is_empty() || !get_detects_flags().is_empty(); }

		void update_collision_skipped();

		// advance - begin
		static void advance__update_collisions(IMMEDIATE_JOB_PARAMS);
		// advance - end

		void debug_draw(int _againstCollision) const;
		void log_collision_models(LogInfoContext& _log, bool _movement = true, bool _precise = true) const;

		// should already know if we're in front of the door or not (doesn't check it)
		bool is_collision_in_radius(DoorInRoom const * _door, Transform const & _placedInRoomWithDoor, float _radius = 0.0f) const; // radius is how much collision radius should be enlarged
	
		static float get_distance_between(ModuleCollision const * _a, Optional<Transform> _aPlacement, ModuleCollision const * _b, Optional<Transform> _bPlacement);

	public: // Module
		override_ void reset();
		override_ void setup_with(ModuleData const * _moduleData);

	protected: friend class ModulePresence;
		// should already know if we're in front of the door or not (doesn't check it)
		void should_consider_for_collision(DoorInRoom const * _door, Transform const & _placedInRoomWithDoor, float _distanceSoFar, REF_ bool & _collision, REF_ bool & _collisionGradient, REF_ bool & _collisionDetection) const;

	protected: friend class ModuleAppearance;
		void clear_movement_collision();
		void clear_precise_collision();

		// these methods clear existing collision
		Collision::ModelInstance* use_movement_collision(Collision::Model const * _model, Transform const & _placement = Transform::identity); // will clear precise collision too!
		Collision::ModelInstance* use_precise_collision(Collision::Model const * _model, Transform const & _placement = Transform::identity);

	protected:
		void update_collisions(float _deltaTime);
		virtual void update_gradient_and_detection();
	
	public:
		struct CheckIfCollidingContext
		{
			Collision::ICollidableObject const* detectedCollisionWith = nullptr;
		};
		virtual bool check_if_colliding(Vector3 const & _addOffsetWS, float _radiusCoef = 1.0f, CheckIfCollidingContext * _context = nullptr) const;
		virtual bool check_if_colliding_stretched(Vector3 const & _endOffsetWS, float _radiusCoef = 1.0f, CheckIfCollidingContext* _context = nullptr) const; // along the offset

	protected:
		ModuleCollisionData const * moduleCollisionData;

		// cached from module collision data, movement collision filled with default if not set
		Framework::UsedLibraryStored<Framework::PhysicalMaterial> movementCollisionMaterial;
		Framework::UsedLibraryStored<Framework::PhysicalMaterial> preciseCollisionMaterial;

		Collision::ICollidableObject* collidableOwner = nullptr;

		int frameDelayOnActualUpdatingCollisions = 1; // this is for specific objects to allow things to kick in properly on restart

		bool navObstructor = false;
		bool navObstructorOnlyRoot = false; // only root bone is used for nav obstruction

		bool allowCollisionsWithSelf = true; // if we would have doors too close we may end up colliding with ourselves - allow or disallow?
		bool storeGradientCollisionInfo = true; // it might be required by lots of things
		bool doEarlyCheckForUpdateGradient = false; // optimisation hack - gradient is updated only if actor of collidesWithFlags is nearby
		bool noMovementCollision = false; // don't use movement collision at all

		// this is to prevent pushing NPCs through walls
		bool cantBePushedThroughCollision = false; // if set to true, will check if during the movement is colliding with "throughCollisionPusher" and if so, if has been pushed through a collision. should be set only for AI
		bool throughCollisionPusher = false; // this should be set for both player and AI

		bool shouldUpdateCollisions = true; // this is to allow preparing move once to clear collision and update gradient once (to zero most likely)

		RUNTIME_ bool playCollisionSound = true; // can be disabled
		bool playCollisionSoundUsingAttachedToSetup = false; // if attached to, will use attached to setup (will check playCollisionSoundThroughMaterial too!)
		bool playCollisionSoundThroughMaterial = false;
		float playCollisionSoundMinSpeed = 0.0f; // won't play anything when below this value
		float playCollisionSoundFullVolumeSpeed = 0.0f; // what speed has to have object to play at full volume
		Name collisionSound; // to play if collided
		float collisionSoundMinSpeed = 0.005f;
		float collisionSoundNoOftenThan = 0.1f; // to avoid flooding

		float timeSinceLastCollisionSound = 1000.0f;

		Meshes::Pose * collisionUpdatedForPoseMS = nullptr;
		bool updateCollisionPrimitiveBasingOnBoundingBox = false;
		bool updateCollisionPrimitiveBasingOnBoundingBoxAsSphere = false;
		Collision::ModelInstanceSet movementCollision;
		Collision::ModelInstanceSet fallbackPreciseCollision; // if no movement collision is set and no precise collision
		Collision::ModelInstanceSet preciseCollision;
		bool usingProvidedMovementCollision = false;
		bool usingProvidedPreciseCollision = false;
		Collision::Model ownMovementCollisionModel;
		Collision::Model ownPreciseCollisionModel; // used only if precise material provided

#ifdef AN_DEVELOPMENT
		bool updateCollidableObjectRequired = false;
#endif
		struct CollisionFlagsStackElement
		{
			Name id;
			Collision::Flags flags;
			CollisionFlagsStackElement() {}
			CollisionFlagsStackElement(Collision::Flags const & _flags) : flags(_flags) {}
			CollisionFlagsStackElement(Name const & _id, Collision::Flags const & _flags) : id(_id), flags(_flags) {}
		};
		Array<CollisionFlagsStackElement> collidesWithFlagsStack; // with what does it collide, what can stop it
		Array<CollisionFlagsStackElement> detectsFlagsStack; // just detection
		Array<CollisionFlagsStackElement> collisionFlagsStack; // what it may stop
		Collision::Flags ikTraceFlags;
		Collision::Flags ikTraceRejectFlags;
		Collision::Flags presenceTraceFlags;
		Collision::Flags presenceTraceRejectFlags;
		struct DontCollideWith
		{
			Collision::ICollidableObject const * object;
			Optional<float> timeLeft; // for dontCollideWith it is time to remove it from the table, for "no collision" until it might be removed

			// this below is to support does_contain of array
			DontCollideWith() {}
			DontCollideWith(Collision::ICollidableObject const * _object, Optional<float> const & _timeLeft = NP) : object(_object), timeLeft(_timeLeft) {}
			bool operator==(DontCollideWith const & _other) const { return object == _other.object; }

			static void add_to(Array<DontCollideWith> & _array, Collision::ICollidableObject const * _object, Optional<float> const & _timeLeft = NP);
		};
		Array<DontCollideWith> dontCollideWith;
		Array<DontCollideWith> dontCollideWithUntilNoCollision; // this is to disallow colliding until we're no longer colliding - then we clear this and continue
		Array<DontCollideWith> dontCollideWithIfAttachedTo; // this is to disallow colliding until we're no longer colliding - then we clear this and continue

		float mass;

		bool enforceStiffCollisions = false; // will add result to stiff gradient (if allowed/handled) (in other object colliding with this one)

		Optional<float> radiusForGradientOverride; // dynamically changed when required
		float radiusForGradient; // in most cases will be same as primitive radius, but sometimes we may want to move objects away from us when we move but allow our movement unless there is huge block on the way, if not provided (0), will be same as collision primitive radius
		Optional<float> radiusForDetectionOverride;
		float radiusForDetection = 0.0f;
		float gradientStepHole; // hole at the bottom to prevent gradient from hitting bottom (useful with movement on floor)
		float collisionPrimitiveRadius;
		Optional<float> preciseCollisionPrimitiveRadius; // if non zero, used for precise primitive collision
		Vector3 collisionPrimitiveCentreDistance; // if non zero - capsule
		Vector3 collisionPrimitiveCentreOffset;
		Vector3 preciseCollisionPrimitiveCentreAdditionalOffset; // this is added to normal offset
		Collision::QueryPrimitivePoint* movementQueryPoint;
		Collision::QueryPrimitiveSegment* movementQuerySegment;
		Collision::QueryPrimitivePoint* movementQuerySegmentCollapsedToPoint;
		float segmentCollapsedToPointDistance = 0.0f; // how much distance between points was sacrificed during collapse
		bool allowCollapsingSegmentToPoint = false; // this is for special cases where segment is not supported but we need some handling

		// those variables are updated during update_gradient_and_detection
		Array<CollisionInfo> collidedWith;
		Array<Collision::ICollidableObject const *> detectedCollisionWith; // this is just to check if we have collided during this frame
		Array<DetectedCollidable> detected; // this is just to check if we have collided during this frame

		bool collidingWithNonScenery = false;

		Vector3 collisionGradient;
		Vector3 collisionStiffGradient; // if something forces stiff collisions, it is stored in both collision gradient and collision stiff gradient

		void update_collision_primitive_basing_on_bounding_box(bool _asSphere);

		void update_movement_query(OPTIONAL_ OUT_ Vector3* _locA = nullptr, OPTIONAL_ OUT_ Vector3* _locB = nullptr);
		void use_movement_query_point(Collision::QueryPrimitivePoint const & _qpp);
		void use_movement_query_segment(Collision::QueryPrimitiveSegment const & _qps);

		template <typename Class>
		void add_primitive_to_own_collision_models(Class const& _primitiveMovement, Class const& _primitivePrecise, bool _usePrimitiveForPreciseCollision);

		void update_should_update_collisions();
	};

};

