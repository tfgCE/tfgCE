#include "mesh3dRenderingContext.h"

using namespace Meshes;

Mesh3DRenderingContext Mesh3DRenderingContext::s_none = Mesh3DRenderingContext();

Mesh3DRenderingContext::Mesh3DRenderingContext()
: meshBonesProvider(nullptr)
, shaderProgramBindingContext(nullptr)
{
}
