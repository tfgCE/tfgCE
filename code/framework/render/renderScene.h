#pragma once

#include "renderCamera.h"
#include "renderEnums.h"
#include "roomProxy.h"
#include "sceneBuildContext.h"
#include "..\display\displayTypes.h"
#include "..\..\core\memory\softPooledObject.h"

namespace System
{
	struct MaterialInstance;
	class RenderTarget;
	struct VideoConfig;
};

namespace Framework
{
	namespace Render
	{
		class Camera;
		class Context;
		class SceneBuildContext;

		struct AllowVRInputOnRender
		{
			bool allowRotation = true;
			bool allowTranslation = true;

			bool is_any_allowed() const { return allowRotation || allowTranslation; }

			AllowVRInputOnRender(bool _allow = true) : allowRotation(_allow), allowTranslation(_allow) {}
			AllowVRInputOnRender(bool _allowRotation, bool _allowTranslation) : allowRotation(_allowRotation), allowTranslation(_allowTranslation) {}
			static AllowVRInputOnRender rotation_only() { return AllowVRInputOnRender(true, false); }

			AllowVRInputOnRender& maybe_change_to_rotation_only(bool _change) { if (_change) { allowRotation = true; allowTranslation = false; } return *this; }
		};

		/**
		 *	It's rebuild every frame and contains whatever is on scene
		 *
		 *	Important thing about VR
		 *		scene is built at some random point, it is VERY important that when it is being built,
		 *		camera should be based on latest render_view from vr (check advance_vr_important)
		 *		because we store render_view as reference for actual rendering
		 *		when rendering, we use new render_view from vr to update view
		 *
		 *		Flow is:
		 *			pre_advance
		 *				during which we update vr controls
		 *					this is when we get latest render_view and update vr important appearance controllers and camera ( renderView[n] )
		 *			build render scenes
		 *				using latest renderView[n]
		 *			wait for previous frame to be displayed
		 *			render latest scenes
		 *				before we start, we update camera with latest vr camera location ( renderView[n+1] )
		 *				this may alter scene a bit
		 */
		class Scene
		: public RefCountObject 
		, public SoftPooledObject<Scene>
		{
		public:
			typedef std::function<void(Scene* _scene, Render::Context & _context)> OnSubRender;

		public:
			static Scene* build(Camera const & _camera, Optional<SceneBuildRequest> const & _buildRequest = NP, std::function<void(SceneBuildContext& _context)> _alterContext = nullptr, Optional<SceneView::Type> _sceneView = NP, Optional<SceneMode::Type> _sceneMode = NP, Optional<float> _vrScale = NP, Optional<AllowVRInputOnRender> _allowVRInputOnRender = NP);

			void render(REF_ Render::Context & _context, ::System::RenderTarget* _nonVRRenderTarget); // for vr will auto bind render targets

			int get_vr_eye_idx() const { return vrEyeIdx;  }

			Camera const & get_camera() const { return camera; }
			Camera const & get_render_camera() const { return renderCamera; }
			Camera & access_render_camera() { return renderCamera; }
			SceneBuildContext const & get_scene_building_context() const { return sceneBuildContext; }
			Transform const& get_vr_anchor() const { return vrAnchor; }

			void add_extra(Meshes::Mesh3DInstance const * _mesh, Transform const & _placement, bool _background = false); // try to avoid adding extra and use objects in world whenever possible, rendered against vr anchor
			void prepare_extras();

			void set_background_colour(Colour const & _backgroundColour) { backgroundColour = _backgroundColour; }
			void set_background_material(::System::MaterialInstance const* _materialInstance) { backgroundMaterialInstance = _materialInstance; }

			void add_display(Display* _display);
			void combine_displays_from(Scene* _scene);
			static void combine_displays(Array<RefCountObjectPtr<Framework::Render::Scene> > & _scenes);

			void set_on_pre_render(OnSubRender _on_pre_render) { on_pre_render = _on_pre_render; }
			void set_on_post_render(OnSubRender _on_post_render) { on_post_render = _on_post_render; }
			bool is_on_post_render_set() const { return on_post_render != nullptr; }

#ifdef RENDER_SCENE_WITH_ON_SHOULD_ADD_TO_RENDER_SCENE
			void set_on_should_add_to_render_scene(OnShouldAddToRenderScene _on_should_add_to_render_scene) { on_should_add_to_render_scene = _on_should_add_to_render_scene; }
			OnShouldAddToRenderScene get_on_should_add_to_render_scene() const { return on_should_add_to_render_scene; }
#endif
#ifdef RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
			void set_on_prepare_to_render(OnPrepareToRenderOnRenderScene _on_prepare_to_render) { on_prepare_to_render = _on_prepare_to_render; }
			OnPrepareToRenderOnRenderScene get_on_prepare_to_render() const { return on_prepare_to_render; }
#endif

			void log(LogInfoContext& _log) const;

		protected: // SoftPooledObject
			friend class SoftPooledObject<Scene>;
			override_ void on_get();
			override_ void on_release();

		protected: // RefCountObject
			override_ void destroy_ref_count_object() { release(); }

		private:
			SceneView::Type view = SceneView::FromCamera;

			Camera camera; // main camera
			Camera renderCamera; // adjusted for rendering, should be used by post processes, also can be changed between build and render
			Colour backgroundColour;
			::System::MaterialInstance const * backgroundMaterialInstance = nullptr;
			bool vr;
			int vrEyeIdx;
			Transform viewOffset; // offsets applied to camera during rendering or for eye clip planes

			OnSubRender on_pre_render = nullptr;
			OnSubRender on_post_render = nullptr;

#ifdef RENDER_SCENE_WITH_ON_SHOULD_ADD_TO_RENDER_SCENE
			OnShouldAddToRenderScene on_should_add_to_render_scene = nullptr;
#endif
#ifdef RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
			OnPrepareToRenderOnRenderScene on_prepare_to_render = nullptr;
#endif

			Optional<Transform> preparedForVRView;
			AllowVRInputOnRender allowVRInputOnRender = AllowVRInputOnRender(true);

			SceneBuildContext sceneBuildContext;
			RoomProxy* room = nullptr; // rendered room
			Transform vrAnchor = Transform::identity; // for room rendered

			Transform foregroundCameraOffset = Transform::identity; // used if we swapped rooms
			PresenceLinkProxy* foregroundPresenceLinks = nullptr; // in room, as it is
			PresenceLinkProxy* foregroundOwnerLocalPresenceLinks = nullptr; // in room, but is placed in owner's space
			PresenceLinkProxy* foregroundCameraLocalPresenceLinks = nullptr; // always in camera space
			PresenceLinkProxy* foregroundCameraLocalOwnerOrientationPresenceLinks = nullptr; // in camera space but orientated as owner

			Meshes::Mesh3DRenderingBuffer foregroundPresenceLinksRenderingBuffer; // all foreground objects

			// rendered against vr anchor
			Meshes::Mesh3DInstanceSetInlined additionalMeshes;
			Meshes::Mesh3DInstanceSetInlined additionalBackgroundMeshes;
			Meshes::Mesh3DRenderingBuffer additionalMeshesRenderingBuffer;
			Meshes::Mesh3DRenderingBuffer additionalBackgroundMeshesRenderingBuffer;
#ifdef AN_DEVELOPMENT
			bool additionalMeshesPrepared;
#endif

			Array<DisplayPtr> displays; // they may happen to appear in few scenes, this should be resolved (combine_displays) and only first render scene should render them

			friend class SceneBuildContext;
		};

		typedef RefCountObjectPtr<Scene> ScenePtr;

	};
};

TYPE_AS_CHAR(Framework::Render::Scene);
