#pragma once

#include "viewFrustum.h"
#include "video3d.h"
#include "..\..\vr\iVR.h"

namespace System
{
	namespace CameraType
	{
		enum Type
		{
			Projection,
			Orthogonal
		};
	};

	class Camera
	{
	public:
		Camera();

		inline int get_eye_idx() const { return eyeIdx; }
		inline CameraType::Type get_camera_type() const { return cameraType; }
		inline Transform const & get_placement() const { return placement; }
		inline DEG_ float get_fov() const { return fov; }
		inline DEG_ Optional<VRFovPort> const& get_fov_port() const { return fovPort; }
		inline float get_ortho_scale() const { return orthoScale; }
		inline VectorInt2 const & get_view_origin() const { return viewOrigin; }
		inline VectorInt2 const & get_view_size() const { return viewSize; }
		inline float get_view_aspect_ratio() const { return viewAspectRatio; }
		inline Vector2 const & get_view_centre_offset() const { return viewCentreOffset; }
		inline Range const & get_near_far_plane() const { return planes; }
		inline Matrix44 const & get_projection_matrix() const { return projectionMatrix; }

#ifdef AN_ASSERT
		bool has_projection_matrix_calculated() const { return hasProjectionMatrixCalculated; }
#endif

	protected:
		int eyeIdx = 0;
		CameraType::Type cameraType = CameraType::Projection; // some values may not apply for certain types
		Transform placement;
		// projection-only
		DEG_ float fov; // Y, vertical
		DEG_ Optional<VRFovPort> fovPort; // if set, will use in favour of fov+viewcentreoffset
		// orthogonal-only
		float orthoScale = 1.0f; // Y, vertical
		// common
		LOCATION_ VectorInt2 viewOrigin; // corner of view in target render buffer
		SIZE_ VectorInt2 viewSize; // where to render stuff
		float viewAspectRatio; // aspect_ratio(viewSize) might be with applied aspectRatioCoef
		LOCATION_ Vector2 viewCentreOffset; // view centre offset in projection (it is -1 to 1)
		Range planes;
		Matrix44 projectionMatrix;
#ifdef AN_ASSERT
		bool hasProjectionMatrixCalculated;
#endif
	};
};
