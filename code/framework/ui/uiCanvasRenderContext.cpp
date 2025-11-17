#include "uiCanvasRenderContext.h"

//

using namespace Framework;
using namespace UI;

//

CanvasRenderContext::CanvasRenderContext(::System::Video3D* _v3d, Canvas const* _canvas)
: v3d(_v3d)
, canvas(_canvas)
{
}