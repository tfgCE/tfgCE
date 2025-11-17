#pragma once

#include "renderCamera.h"
#include "renderEnums.h"

#include "..\..\core\system\video\videoClipPlaneStack.h"

namespace Framework
{
	namespace Render
	{
		class Scene;
		class SceneBuildContext;
		class SceneRenderContext
		{
		public:
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
			typedef ::System::CustomVideoClipPlaneStack<::System::CustomVideoClipPlaneSet<32>> ExternalSceneViewClipPlaneStack;
#endif

			SceneRenderContext(SceneBuildContext const & _sceneBuildContext, Camera const & _camera);

			SceneView::Type get_scene_view() const { return sceneView; }

			// this is the camera used for building, although it could be changed
			// this is without the offset that might be applied
			Camera const & get_camera() const { return camera; }

			Matrix44 const & get_view_matrix() const { return viewMatrix; }

			Vector2 const & get_fov_size() const { return fovSize; }
			float get_near_plane() const { return nearPlane; }
			float get_max_door_dist() const { return maxDoorDist; }

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
			ExternalSceneViewClipPlaneStack& access_external_scene_view_clip_plane_stack() { return externalSceneViewClipPlaneStack; }
#endif
		private:
			SceneView::Type sceneView;
			Camera camera;
			Matrix44 viewMatrix;
			Vector2 fovSize;
			float nearPlane;
			float maxDoorDist;

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
			ExternalSceneViewClipPlaneStack externalSceneViewClipPlaneStack;
#endif
		};

	};
};
