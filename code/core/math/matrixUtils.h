#pragma once

#include "math.h"

inline float aspect_ratio(SIZE_ Vector2 const & _size);
inline float aspect_ratio(SIZE_ VectorInt2 const & _size);
inline Vector2 maintain_aspect_ratio_keep_x(SIZE_ Vector2 const & _size, SIZE_ Vector2 const & _baseOnAspectRatio);
inline Vector2 maintain_aspect_ratio_keep_y(SIZE_ Vector2 const & _size, SIZE_ Vector2 const & _baseOnAspectRatio);

// calculate smaller resolution with applied aspectRatioCoef
inline Vector2 apply_aspect_ratio_coef(SIZE_ Vector2 const& _size, float _aspectRatioCoef);
inline VectorInt2 apply_aspect_ratio_coef(SIZE_ VectorInt2 const& _size, float _aspectRatioCoef);

// calculate full from smaller that had applied aspectRatioCoef
inline Vector2 get_full_for_aspect_ratio_coef(SIZE_ Vector2 const& _size, float _aspectRatioCoef);
inline VectorInt2 get_full_for_aspect_ratio_coef(SIZE_ VectorInt2 const& _size, float _aspectRatioCoef);

Matrix44 projection_matrix(DEG_ float const _fov, float const _aspectRatio, float const _nearPlane, float const _farPlane);
Matrix44 projection_matrix(float const minX, float const maxX, float const minY, float const maxY, float const _nearPlane, float const _farPlane);
Matrix44 projection_matrix_assymetric_fov(float const leftDegrees, float const rightDegrees, float const upDegrees, float const downDegrees, float const _nearPlane, float const _farPlane);

// define in what space do you want to give values
Matrix44 orthogonal_matrix_for_2d(LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size, float const _nearPlane = 0.0f, float const _farPlane = 1.0f);
Matrix44 orthogonal_matrix(Range2 const & _range, float const _nearPlane = 0.0f, float const _farPlane = 1.0f);

// this is camera view matrix - inverted look at matrix, makes "world" a local space of camera
Matrix44 camera_view_matrix(LOCATION_ Vector3 const & _from, LOCATION_ Vector3 const & _at, NORMAL_ Vector3 const & _cameraUp);

inline Matrix44 rotation_xy_matrix(float _deg);
inline Matrix44 scale_matrix(float _scale);
inline Matrix44 scale_matrix(Vector3 const & _scale);
inline Matrix44 translation_matrix(LOCATION_ Vector3 const & _translation);
inline Matrix44 translation_scale_xy_matrix(LOCATION_ Vector2 const & _translation, Vector2 const & _scale);
// x right  y forward  z up
inline Matrix44 look_at_matrix(LOCATION_ Vector3 const & _from, LOCATION_ Vector3 const & _at, NORMAL_ Vector3 const & _up);
inline Matrix44 look_at_matrix_with_right(LOCATION_ Vector3 const & _from, LOCATION_ Vector3 const & _at, NORMAL_ Vector3 const & _right);
inline Matrix44 look_matrix(LOCATION_ Vector3 const & _from, NORMAL_ Vector3 const & _lookDir, NORMAL_ Vector3 const & _up);
inline Matrix44 look_matrix_no_up(LOCATION_ Vector3 const & _from, NORMAL_ Vector3 const & _lookDir);
inline Matrix44 look_matrix_with_right(LOCATION_ Vector3 const & _from, NORMAL_ Vector3 const & _lookDir, NORMAL_ Vector3 const & _right);
Matrix44 matrix_from_up(LOCATION_ Vector3 const & _from, NORMAL_ Vector3 const & _up);
inline Matrix44 matrix_from_axes_orthonormal_check(NORMAL_ Vector3 const & _xAxis, NORMAL_ Vector3 const & _yAxis, NORMAL_ Vector3 const & _zAxis, LOCATION_ Vector3 const & _location);
inline Matrix44 matrix_from_axes(NORMAL_ Vector3 const & _xAxis, NORMAL_ Vector3 const & _yAxis, NORMAL_ Vector3 const & _zAxis, LOCATION_ Vector3 const & _location);
inline Matrix33 look_matrix33(NORMAL_ Vector3 const & _lookDir, NORMAL_ Vector3 const & _up);
inline Matrix33 up_matrix33(NORMAL_ Vector3 const & _up);
inline Matrix44 matrix_from_up_right(LOCATION_ Vector3 const & _loc, NORMAL_ Vector3 const & _up, NORMAL_ Vector3 const & _right);
inline Matrix44 matrix_from_up_forward(LOCATION_ Vector3 const & _loc, NORMAL_ Vector3 const & _up, NORMAL_ Vector3 const & _forward);

inline Matrix44 turn_matrix_xy_180(Matrix44 const & mat);

Vector3 calculate_flat_forward_from(Transform const& _headTransform);
