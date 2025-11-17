#include "meshGenBasicShapes.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace BasicShapes;

void BasicShapes::add_shapes()
{
	REGISTER_MESH_GEN_SHAPE(TXT("bounding box"), BasicShapes::bounding_box, BasicShapes::create_data);
	REGISTER_MESH_GEN_SHAPE(TXT("capsule"), BasicShapes::capsule, BasicShapes::create_capsule_data);
	REGISTER_MESH_GEN_SHAPE(TXT("cube"), BasicShapes::cube, BasicShapes::create_data);
	REGISTER_MESH_GEN_SHAPE(TXT("sphere"), BasicShapes::sphere, BasicShapes::create_sphere_data);
	REGISTER_MESH_GEN_SHAPE(TXT("cylinder"), BasicShapes::cylinder, BasicShapes::create_data);
	REGISTER_MESH_GEN_SHAPE(TXT("convex hull"), BasicShapes::convex_hull, BasicShapes::create_convex_hull_data);
}

