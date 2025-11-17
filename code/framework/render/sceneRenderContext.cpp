#include "sceneRenderContext.h"
#include "renderCamera.h"

#include "renderScene.h"

#include "..\..\core\system\video\videoClipPlaneStackImpl.inl"

//

using namespace Framework;
using namespace Framework::Render;

//

SceneRenderContext::SceneRenderContext(SceneBuildContext const & _sceneBuildContext, Render::Camera const & _camera)
: sceneView(_sceneBuildContext.get_scene_view())
, camera(_camera)
, fovSize(_sceneBuildContext.get_fov_size())
, nearPlane(_sceneBuildContext.get_near_plane())
, maxDoorDist(_sceneBuildContext.get_max_door_dist())
{
	// store view matrix (as inverted placement of camera)
	viewMatrix = camera.get_placement().to_matrix().inverted();
}
