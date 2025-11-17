#include "meshGenSplineRef.h"

#include "meshGenSpline.h"

#include "../../library/library.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace CustomData;

//

#ifndef AN_CLANG
template struct SplineRef<Vector2>;
template struct SplineRef<Vector3>;
#endif

//

#ifndef AN_CLANG
#include "meshGenSplineRefImpl.inl"
#endif

