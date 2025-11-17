#pragma once

#include "..\..\core\system\video\camera.h"

namespace Framework
{
	class Room;
	class IModulesOwner;
	struct PointOfInterestInstance;

	namespace CameraPlacementBasedOn
	{
		enum Type
		{
			PresenceCentre,		// presence centre, default
			OwnerPlacement,		// owner's placement (placement in presence) that might be not aligned with presence centre!
			Eyes,				// eyes stored in presence
		};
	};

	/**
	 *	Camera placeable in world, base for rendering and sound cameras
	 */
	class Camera
	: public ::System::Camera
	{
	public:
		Camera();

		inline void set_owner(IModulesOwner const * _owner) { owner = _owner; }
		inline void set_eye_idx(int _eyeIdx) { eyeIdx = _eyeIdx; }
		inline void set_camera_type(System::CameraType::Type _cameraType) { cameraType = _cameraType; }
		inline void set_fov(DEG_ float _fov, DEG_ Optional<VRFovPort> const& _fovPort = NP) { fov = _fov; fovPort = _fovPort; }
		inline void set_ortho_scale(float _orthoScale) { orthoScale = _orthoScale; }
		inline void set_placement(Room * _inRoom, Transform const & _placement) { inRoom = _inRoom; placement = _placement; owner = nullptr; ownerRelativePlacement = Transform::identity; }
		inline void set_view_origin(VectorInt2 _viewOrigin) { viewOrigin = _viewOrigin; }
		inline void set_view_size(VectorInt2 _viewSize) { viewSize = _viewSize; }
		inline void set_view_aspect_ratio(float _viewAspectRatio) { viewAspectRatio = _viewAspectRatio; }
		inline void set_view_centre_offset(Vector2 _viewCentreOffset) { viewCentreOffset = _viewCentreOffset; }
		inline void set_near_far_plane(float _near, float _far) { planes = Range(_near, _far); }

		void clear_placement();
		//	note:	places in presence centre! use _basedOn to get eyes, ears etc
		//			if using just offset_placement on owner's placement (not its presence centre!) you may end up beyond presence! and miss some important doors
		void set_placement_based_on(IModulesOwner const * _owner, CameraPlacementBasedOn::Type _basedOn = CameraPlacementBasedOn::PresenceCentre);
		void set_placement_based_on(PointOfInterestInstance* _foundPOI);

		// those methods will change placement against current one! note that initial placement is in presence centre!
		void offset_placement(Transform const & _inLocalSpace, bool _keepWithinRoom = false);
		void offset_location(Vector3 const & _inLocalSpace, bool _keepWithinRoom = false);
		void offset_orientation(Quat const & _inLocalOrientationSpace);

		inline Room * get_in_room() const { return inRoom; }
		inline IModulesOwner const * get_owner() const { return owner; }
		inline Transform const & get_owner_relative_placement() const { return ownerRelativePlacement; }
		inline Transform const& get_vr_anchor() const { return vrAnchor; }

		inline void set_placement_only(Transform const & _placement) { placement = _placement; }
		inline void set_owner_relative_placement(Transform const & _ownerRelativePlacement) { ownerRelativePlacement = _ownerRelativePlacement; }

	protected:
		IModulesOwner const * owner = nullptr;
		Transform ownerRelativePlacement = Transform::identity;
		Room * inRoom = nullptr;
		Transform vrAnchor = Transform::identity;
	};
};
