#include "wmp_debugDrawArrow.h"

#include "..\meshGen\meshGenMeshNodeContext.h"

#include "..\..\core\debug\debugRenderer.h"

using namespace Framework;

//

void DebugDrawArrow::draw(DebugRendererFrame* _drf, Colour const & _colour, Vector3 const & _a, Vector3 const & _b) const
{
	_drf->add_arrow(true, _colour, _a, _b, (_b - _a).length() * 0.05f);
}
