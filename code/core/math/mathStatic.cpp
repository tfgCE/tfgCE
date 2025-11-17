#include "math.h"

//

static Plane create_garbage_plane() { Plane p; p.normal.x = 1.0f; p.normal.y = 2.0f; p.normal.z = 3.0f; p.anchor.x = 4.0f; p.anchor.y = 5.0f; p.anchor.z = 6.0f; return p; }

//

Range const Range::zero(0.0f, 0.0f);
Range const Range::empty(9999.0f, -9999.0f);
Range const Range::infinite(-INF_FLOAT, INF_FLOAT);

Range2 const Range2::zero(Range::zero, Range::zero);
Range2 const Range2::empty(Range::empty, Range::empty);

Range3 const Range3::zero(Range::zero, Range::zero, Range::zero);
Range3 const Range3::empty(Range::empty, Range::empty, Range::empty);

RangeRotator3 const RangeRotator3::zero(Range::zero, Range::zero, Range::zero);
RangeRotator3 const RangeRotator3::empty(Range::empty, Range::empty, Range::empty);

Sphere const Sphere::zero(Vector3::zero, 0.0f);

Rotator3 const Rotator3::zero(0.0f, 0.0f, 0.0f);
Rotator3 const Rotator3::xAxis( 0.0f, 90.0f, 0.0f );
Rotator3 const Rotator3::yAxis( 0.0f, 0.0f, 0.0f );
Rotator3 const Rotator3::zAxis( 90.0f, 0.0f, 0.0f );

RangeInt const RangeInt::zero(0, 0);
RangeInt const RangeInt::empty(9999, -9999);

RangeInt2 const RangeInt2::zero(RangeInt::zero, RangeInt::zero);
RangeInt2 const RangeInt2::empty(RangeInt::empty, RangeInt::empty);

RangeInt3 const RangeInt3::zero(RangeInt::zero, RangeInt::zero, RangeInt::zero);
RangeInt3 const RangeInt3::empty(RangeInt::empty, RangeInt::empty, RangeInt::empty);

Vector2 const Vector2::zero(0.0f, 0.0f);
Vector2 const Vector2::one(1.0f, 1.0f);
Vector2 const Vector2::half(0.5f, 0.5f);
Vector2 const Vector2::xAxis(1.0f, 0.0f);
Vector2 const Vector2::yAxis(0.0f, 1.0f);

Vector3 const Vector3::zero(0.0f, 0.0f, 0.0f);
Vector3 const Vector3::one(1.0f, 1.0f, 1.0f);
Vector3 const Vector3::half(0.5f, 0.5f, 0.5f);
Vector3 const Vector3::xAxis(1.0f, 0.0f, 0.0f);
Vector3 const Vector3::yAxis(0.0f, 1.0f, 0.0f);
Vector3 const Vector3::zAxis(0.0f, 0.0f, 1.0f);
Vector3 const Vector3::xy(1.0f, 1.0f, 0.0f);
Vector3 const Vector3::xz(1.0f, 0.0f, 1.0f);

Vector4 const Vector4::zero(0.0f, 0.0f, 0.0f, 0.0f);

VectorInt2 const VectorInt2::zero(0, 0);
VectorInt2 const VectorInt2::one(1, 1);

VectorInt3 const VectorInt3::zero(0, 0, 0);
VectorInt3 const VectorInt3::one(1, 1, 1);

VectorInt4 const VectorInt4::zero(0, 0, 0, 0);

Quat const Quat::zero(0.0f, 0.0f, 0.0f, 0.0f);
Quat const Quat::identity(0.0f, 0.0f, 0.0f, 1.0f);

Plane const Plane::identity(Vector3::zAxis, Vector3::zero);
Plane const Plane::forward(Vector3::yAxis, Vector3::zero);
Plane const Plane::zero(Vector3::zero, Vector3::zero);
Plane const Plane::garbage = create_garbage_plane();

Transform const Transform::zero(Vector3(0.0f, 0.0f, 0.0f), Quat(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
Transform const Transform::identity(Vector3(0.0f, 0.0f, 0.0f), Quat(0.0f, 0.0f, 0.0f, 1.0f));
Transform const Transform::do180(Vector3(0.0f, 0.0f, 0.0f), Rotator3(0.0f, 180.0f, 0.0f).to_quat());

Matrix33 const Matrix33::zero( 0.0f, 0.0f, 0.0f,
							   0.0f, 0.0f, 0.0f,
							   0.0f, 0.0f, 0.0f );
Matrix33 const Matrix33::identity( 1.0f, 0.0f, 0.0f,
								   0.0f, 1.0f, 0.0f,
								   0.0f, 0.0f, 1.0f );


Matrix44 const Matrix44::zero( 0.0f, 0.0f, 0.0f, 0.0f,
							   0.0f, 0.0f, 0.0f, 0.0f,
							   0.0f, 0.0f, 0.0f, 0.0f,
							   0.0f, 0.0f, 0.0f, 0.0f );
Matrix44 const Matrix44::identity( 1.0f, 0.0f, 0.0f, 0.0f,
								   0.0f, 1.0f, 0.0f, 0.0f,
								   0.0f, 0.0f, 1.0f, 0.0f,
								   0.0f, 0.0f, 0.0f, 1.0f );
