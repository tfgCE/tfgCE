#include "iMesh3d.h"

using namespace ::System;
using namespace Meshes;

REGISTER_FOR_FAST_CAST(IMesh3D);

IMesh3D::IMesh3D()
{
	renderingOrder.meshPtr = (Mesh3DRenderingOrder::pointerAsNumber)this;
}

IMesh3D::~IMesh3D()
{
}