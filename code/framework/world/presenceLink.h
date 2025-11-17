#pragma once

#include "..\modulesOwner\modulesOwner.h"
#include "..\..\core\collision\modelInstanceSet.h"
#include "..\..\core\memory\softPooledObjectMT.h"
#include "..\collision\againstCollision.h"
#include "..\collision\checkCollisionContext.h"
#include "..\collision\checkSegmentResult.h"

#ifdef AN_DEVELOPMENT
#ifdef AN_ALLOW_EXTENSIVE_LOGS
//#define DETAILED_PRESENCE_LINKS
//#define VERY_DETAILED_PRESENCE_LINKS
#endif
#endif

namespace Framework
{
	class Room;
	class DoorInRoom;
	class Presence;

	namespace Render
	{
		class PresenceLinkProxy;
	};

	struct PresenceLinkBuildContext
	{
		int presenceLinkListIdx;
		PresenceLinkBuildContext();
	};

	/*
	 *	Building presence links happens at specific time during frame advancement (see World class)
	 *	It should not be removed/build anytime else.
	 */
	class PresenceLink
	: public SoftPooledObjectMT<PresenceLink>
	{
	public:
		static PresenceLink* create(PresenceLinkBuildContext const & _context, ModulePresence* _ownerPresence, PresenceLink* _linkTowardsOwner, Room* _inRoom, DoorInRoom* _throughDoor, Transform const & _transformPlacementToRoom, bool _isActualPresence, bool _validForCollision, bool _validForCollisionGradient, bool _validForCollisionDetection, bool _validForLightSources);
#ifdef AN_DEVELOPMENT
		static void check_if_object_has_its_own_presence_links(IModulesOwner* imo);
#endif

		void release_if_not_used();

		void sync_release_in_room_clear_in_object(); // removes in objects too, SYNC!
		// invalidated links still exist, they just show nothing (ie. no object, they are still linked together but when releasing room they will be gone)
		void invalidate_in_room(); // all in the room, doesn't have to be sync
		void invalidate_in_room_objects_through_door(DoorInRoom* _door); // all in the room, doesn't have to be sync
		void invalidate_object(); // as above, but with objects in mind, doesn't have to be sync
		
		// releasing in room checks if objects wants to be released
		// releasing in object is when we already know that (we don't need to remove from object, it will happen anyway
		void release_for_building_in_room_ignore_for_object(); // should be called from room when all links are released, completely ignores links in objects 
		void release_for_building_in_object_ignore_for_room(); // should be called from object when about to build to release previous ones, completely ignores links in rooms

		IModulesOwner* get_modules_owner() const { return owner; }
		Room* get_in_room() const { return inRoom; }
		DoorInRoom* get_through_door() const { return throughDoor; }

		PresenceLink const * get_next_in_room() const { return nextInRoom; }
		PresenceLink const * get_next_in_object() const { return nextInObject; }

		PresenceLink const * get_link_towards_owner() const { return linkTowardsOwner; }

		Transform const & get_placement_in_room_for_presence_primitive() const { return placementInRoomForPresencePrimitive; }
		Transform const & get_placement_in_room() const { return placementInRoom; }
		Matrix44 const & get_placement_in_room_matrix() const { return placementInRoomMat; }
		Transform const & get_placement_in_room_for_rendering() const { return placementInRoomForRendering; }
		Matrix44 const & get_placement_in_room_for_rendering_matrix() const { return placementInRoomForRenderingMat; }
		Transform const & get_transform_to_room() const { return transformPlacementToRoom; }
		Matrix44 const & get_transform_to_room_matrix() const { return transformPlacementToRoomMat; }
		PlaneSet const & get_clip_planes() const { return clipPlanes; }
		PlaneSet const & get_clip_planes_for_collision() const { return clipPlanesForCollision; }

		Transform transform_to_owners_room_space(Transform const & _placement) const;

		bool is_actual_presence() const { return isActualPresence; }
		bool is_valid_for_collision() const { return validForCollision; }
		bool is_valid_for_collision_gradient() const { return validForCollisionGradient; }
		bool is_valid_for_collision_detection() const { return validForCollisionDetection; }
		bool is_valid_for_light_sources() const { return validForLightSources; }
		bool check_segment_against(AgainstCollision::Type _againstCollision, REF_ Segment & _segment, REF_ CheckSegmentResult & _result, CheckCollisionContext & _context) const;
		static bool check_segment_against(IModulesOwner* imo, Transform const& _placementWS, AgainstCollision::Type _againstCollision, REF_ Segment& _segment, REF_ CheckSegmentResult& _result, CheckCollisionContext& _context, PresenceLink const * _presenceLink = nullptr);
		Range3 const& get_collision_bounding_box() const { return collisionBoundingBox; }

		void log_development_info(LogInfoContext& _log) const;

	protected: friend class SoftPooledObjectMT<PresenceLink>;
		override_ void on_get();
		override_ void on_release();

	protected: friend class SoftPoolMT<PresenceLink>;
		PresenceLink();

	protected: friend class Presence;
		void update_placement_for_rendering_by(Transform const & _placementChange);

	private:
#ifdef AN_DEVELOPMENT
		bool released = false;
		bool invalidated = false;
		IModulesOwner* builtFor = nullptr;
		int debugBuiltPresenceLinksAtCoreFrame = NONE;
#endif
		IModulesOwner* owner = nullptr;
		ModulePresence* ownerPresence;
		Room* inRoom;
		DoorInRoom* throughDoor; // through which door do we get to owner (ie. we entered this room)
		PresenceLink const * linkTowardsOwner; // so we can get through doors to owner
		Transform presenceOffset = Transform::identity;
		float presenceRadius = 0.0f;
		Vector3 presenceCentreDistance = Vector3::zero;
		CACHED_ Transform placementInRoomForPresencePrimitive; // placementInRoom with offset applied
		bool presenceInSingleRoom = false;
		bool isActualPresence; // as below but for presence, is useful when collision (detection most likely) radius is greater than presence radius
		bool validForCollision; // collision reaches door that this presence link was created through
		bool validForCollisionGradient;// as above, but for collision gradient
		bool validForCollisionDetection; // as above, if we have collision detection set
		bool validForLightSources; // light sources may have a different radius
		Transform transformPlacementToRoom; // how presence's placement should be transformed to be in room
		CACHED_ Matrix44 transformPlacementToRoomMat;
		Transform placementInRoom;
		CACHED_ Matrix44 placementInRoomMat;
		Transform placementInRoomForRendering; // in most cases this is the same as placementInRoom, just for VR we want objects to behave like they are in their old place but be visible a bit off (we don't modify actual placement as it could break when going through doors)
		CACHED_ Matrix44 placementInRoomForRenderingMat;
		PlaneSet clipPlanes; // all doors that clip character
		PlaneSet clipPlanesForCollision; // subset of clip planes - only those doors clip that we are inside of their hole

		Range3 collisionBoundingBox; // max of movement and precise

	private: friend class Room;
		PresenceLink* nextInRoom;
		PresenceLink* prevInRoom;

	private: friend class ModulePresence;
		PresenceLink* nextInObject;
		PresenceLink* prevInObject;

		friend class Render::PresenceLinkProxy;

		inline void internal_release_for_building_in_room_ignore_for_object();
		inline void internal_release_for_building_in_object_ignore_for_room();
	};
};

DECLARE_REGISTERED_TYPE(Framework::PresenceLink*);
