#include "meshGenBones.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace Bones;

void Bones::add_bones()
{
	REGISTER_MESH_GEN_BONE(TXT("bone"), Bones::bone_start, Bones::bone_end, Bones::create_bone_data).be_default();
}

