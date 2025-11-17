#include "meshGenShapes.h"

#include "..\..\..\framework\meshGen\meshGenElementShape.h"

//

using namespace TeaForGodEmperor;
using namespace MeshGeneration;
using namespace Shapes;

//

void Shapes::add_shapes()
{
	REGISTER_MESH_GEN_SHAPE(TXT("platforms linear"), Shapes::platforms_linear, Shapes::create_platforms_linear_data);
}


