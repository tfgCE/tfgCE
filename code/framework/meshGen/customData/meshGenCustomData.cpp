#include "meshGenCustomData.h"

#include "meshGenSpline.h"
#include "meshGenEdge.h"
#include "..\meshGenElement.h"

#ifdef AN_CLANG
#include "..\..\library\library.h"
DEFINE_STATIC_NAME(zero);
#include "meshGenSplineImpl.inl"
#include "meshGenSplineRefImpl.inl"
#endif

using namespace Framework;
using namespace MeshGeneration;
using namespace CustomData;

//

void CustomData::initialise_static()
{
	REGISTER_CUSTOM_LIBRARY_STORED_DATA(meshGenSpline2, Spline<Vector2>);
	REGISTER_CUSTOM_LIBRARY_STORED_DATA(meshGenSpline3, Spline<Vector3>);
	REGISTER_CUSTOM_LIBRARY_STORED_DATA(meshGenEdge, Edge);
}

void CustomData::close_static()
{
	//
}

//

