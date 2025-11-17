#include "camera.h"

using namespace ::System;

Camera::Camera()
: placement(Transform::identity)
, fov(90.0f)
{
	viewOrigin = VectorInt2::zero;
	viewSize = VectorInt2::zero; 
	viewAspectRatio = 1.0f;
	viewCentreOffset = Vector2::zero;
	planes.min = 0.02f;
	planes.max = 10000.0f;
#ifdef AN_ASSERT
	hasProjectionMatrixCalculated = false;
#endif
}
