#pragma once

#define INCLUDED_MATH_H

// required

#include "..\globalDefinitions.h"
#include "..\types\string.h"
#include "..\types\optional.h"
#include "..\debug\logInfoContext.h"
#include "..\serialisation\serialiserBasics.h"
#include "..\values.h"


// utils

#include "mathUtils.h"


// enums

#include "axis.h"


// external forward declarations

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

class Serialiser;


// forward declarations

struct Vector2;
struct VectorInt2;
struct Vector3;
struct VectorInt3;
struct Vector4;
struct VectorInt4;
struct Segment;
struct Segment2;
struct SegmentSimple;
struct Plane;
struct PlaneSet;
struct Quat;
struct Range;
struct Range2;
struct Range3;
struct RangeInt;
struct Rotator3;
struct RangeRotator3;
struct Matrix33;
struct Matrix44;
struct Transform;
struct Box;
struct ConvexHull;
struct ConvexPolygon;
struct Sphere;
struct Capsule;


namespace MathCompare
{
	enum Type
	{
		Lesser,
		LesserOrEqual,
		Equal,
		GreaterOrEqual,
		Greater,
		NotEqual
	};
};

DECLARE_SERIALISER_FOR(MathCompare::Type);


// headers

#include "vector2.h"
#include "vectorInt2.h"
#include "vector3.h"
#include "vectorInt3.h"
#include "vector4.h"
#include "vectorInt4.h"
#include "range.h"
#include "range2.h"
#include "range3.h"
#include "rangeInt.h"
#include "rangeInt2.h"
#include "rangeInt3.h"
#include "segment.h"
#include "segment2.h"
#include "segmentSimple.h"
#include "plane.h"
#include "planeSet.h"
#include "quat.h"
#include "rotator3.h"
#include "rangeRotator3.h"
#include "matrix33.h"
#include "matrix44.h"
#include "transform.h"
#include "matrixUtils.h"
#include "transformUtils.h"
#include "blendAdvancer.h"
#include "blendUtils.h"
#include "blendCurves.h"
#include "box.h"
#include "convexHull.h"
#include "convexPolygon.h"
#include "sphere.h"
#include "capsule.h"
#include "dateMath.h"
#include "bezierCurve.h"
#include "bezierSurface.h"
#include "curve.h"
#include "simpleMesh.h"
#include "skinnedMesh.h"


// some utils

#include "polygonUtils.h"
#include "..\other\typeConversions.h"


// inlines

#include "vector2.inl"
#include "vectorInt2.inl"
#include "vector3.inl"
#include "vectorInt3.inl"
#include "vector4.inl"
#include "vectorInt4.inl"
#include "segment.inl"
#include "segment2.inl"
#include "plane.inl"
#include "planeSet.inl"
#include "quat.inl"
#include "range.inl"
#include "range2.inl"
#include "range3.inl"
#include "rangeInt.inl"
#include "rangeInt2.inl"
#include "rangeInt3.inl"
#include "rotator3.inl"
#include "rangeRotator3.inl"
#include "matrix33.inl"
#include "matrix44.inl"
#include "transform.inl"
#include "matrixUtils.inl"
#include "blendAdvancer.inl"
#include "blendUtils.inl"
#include "blendCurves.inl"
#include "bezierCurve.inl"
#include "curve.inl"


#include "mathSpecialisations.h"

#include "mathDefaults.h"

#include "typeUtils.h"

// registered types

DECLARE_REGISTERED_TYPE(Transform);
DECLARE_REGISTERED_TYPE(Plane);
DECLARE_REGISTERED_TYPE(Rotator3);
DECLARE_REGISTERED_TYPE(Vector2);
DECLARE_REGISTERED_TYPE(Vector3);
DECLARE_REGISTERED_TYPE(Vector3*);
DECLARE_REGISTERED_TYPE(Vector4);
DECLARE_REGISTERED_TYPE(VectorInt2);
DECLARE_REGISTERED_TYPE(VectorInt3);
DECLARE_REGISTERED_TYPE(VectorInt4);
DECLARE_REGISTERED_TYPE(Range);
DECLARE_REGISTERED_TYPE(Range2);
DECLARE_REGISTERED_TYPE(Range3);
DECLARE_REGISTERED_TYPE(RangeInt);
DECLARE_REGISTERED_TYPE(RangeInt2);
DECLARE_REGISTERED_TYPE(RangeInt3);
DECLARE_REGISTERED_TYPE(RangeRotator3);
DECLARE_REGISTERED_TYPE(Quat);
DECLARE_REGISTERED_TYPE(Matrix33);
DECLARE_REGISTERED_TYPE(Matrix44);
DECLARE_REGISTERED_TYPE(Sphere);
