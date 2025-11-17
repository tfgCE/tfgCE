#pragma once

#include "..\..\core\system\video\viewFrustum.h"
#include "..\..\core\system\video\viewPlanes.h"
#include "..\..\core\functionParamsStruct.h"
#include "stats.h"

#include "renderCamera.h"
#include "renderEnums.h"

namespace Meshes
{
	class Mesh3DInstance;
};

namespace System
{
	struct MaterialInstance;
};

namespace Framework
{
	class PresenceLink;
	class DoorInRoom;

	namespace Render
	{
		class Scene;
		class SceneBuildContext;

#ifdef RENDER_SCENE_WITH_ON_SHOULD_ADD_TO_RENDER_SCENE
		typedef std::function<bool(Framework::PresenceLink const* _presenceLink, Framework::Render::SceneBuildContext const & _context)> OnShouldAddToRenderScene;
#endif		
#ifdef RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
		typedef std::function<void(REF_ Meshes::Mesh3DInstance& _mesh, Framework::Room const* _inRoom, Transform const& _placement)> OnPrepareToRenderOnRenderScene;
#endif

		struct SceneBuildRequest
		{
			bool noRoomMeshes = false;
			bool noEnvironmentMeshes = false;
			bool noForeground = false; // at all (includes owner and camera)
			bool noForegroundOwnerLocal = false;
			bool noForegroundCameraLocal = false;
			bool noForegroundCameraLocalOwnerOrientation = false;
#ifdef RENDER_SCENE_WITH_ON_SHOULD_ADD_TO_RENDER_SCENE
			OnShouldAddToRenderScene on_should_add_to_render_scene = nullptr;
#endif
#ifdef RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
			OnPrepareToRenderOnRenderScene on_prepare_to_render = nullptr;
#endif
			void * userData = nullptr;

			SceneBuildRequest& no_room_meshes(bool _noRoomMeshes = true) { noRoomMeshes = _noRoomMeshes; return *this; }
			SceneBuildRequest& no_environment_meshes(bool _noEnvironmentMeshes = true) { noEnvironmentMeshes = _noEnvironmentMeshes; return *this; }
			SceneBuildRequest& no_foreground(bool _noForeground = true) { noForeground = _noForeground; return *this; }
			SceneBuildRequest& no_foreground_owner_local(bool _noForegroundOwnerLocal = true) { noForegroundOwnerLocal = _noForegroundOwnerLocal; return *this; }
			SceneBuildRequest& no_foreground_camera_local(bool _noForegroundCameraLocal = true) { noForegroundCameraLocal = _noForegroundCameraLocal; return *this; }
			SceneBuildRequest& no_foreground_camera_owner_orientation(bool _noForegroundCameraLocalOwnerOrientation = true) { noForegroundCameraLocalOwnerOrientation = _noForegroundCameraLocalOwnerOrientation; return *this; }
#ifdef RENDER_SCENE_WITH_ON_SHOULD_ADD_TO_RENDER_SCENE
			SceneBuildRequest& with_on_should_add_to_render_scene(OnShouldAddToRenderScene _on_should_add_to_render_scene) { on_should_add_to_render_scene = _on_should_add_to_render_scene; return *this; }
#endif
#ifdef RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
			SceneBuildRequest& with_on_prepare_to_render(OnPrepareToRenderOnRenderScene _on_prepare_to_render) { on_prepare_to_render = _on_prepare_to_render; return *this; }
#endif
			SceneBuildRequest& with_user_data(void * _data) { userData = _data; return *this; }
		};


		class SceneBuildContext
		{
		public:
			SceneBuildContext() {}
			~SceneBuildContext();

			void setup(REF_ Scene & _scene, Camera const & _camera);

			SceneBuildRequest& access_build_request() { return buildRequest; }
			SceneBuildRequest const & get_build_request() const { return buildRequest; }

			Scene* get_scene() const  { return scene; }
			SceneView::Type get_scene_view() const { return sceneView; }
			Camera const & get_camera() const { return camera; } // this is camera used for building! do not use during rendering as might be off

			::System::ViewFrustum const * get_frustum() const { return viewFrustum; }
			void swap_frustum(::System::ViewFrustum * & _viewFrustum) { swap(viewFrustum, _viewFrustum); }

			::System::ViewPlanes const * get_planes() const { return &viewPlanes; }
			void set_planes(::System::ViewPlanes const& _planes) { viewPlanes = _planes; }

			Matrix44 const & get_view_matrix() const { return viewMatrix; }
			void swap_view_matrix(Matrix44 & _viewMatrix) { swap(viewMatrix, _viewMatrix); }

			Matrix44 const & get_through_door_matrix() const { return throughDoorMatrix; }
			void swap_through_door_matrix(Matrix44 & _throughDoorMatrix) { swap(throughDoorMatrix, _throughDoorMatrix); }

			Range const & get_near_far_plane() const { return planes; }
			float get_near_plane() const { return nearPlane; }
			float get_max_door_dist() const { return maxDoorDist; }

			float get_aspect_ratio() const { return aspectRatio; }
			Vector2 const & get_fov_size() const { return fovSize; }

			bool increase_depth() { if (depth < depthLimit) { ++depth; return true; } else { return false; } }
			void decrease_depth() { --depth; }
			int get_depth() const { return depth; }

			Name const & get_appearance_name_to_build() const { return appearanceNameToBuild; }
			void set_appearance_name_to_build(Name const & _appearanceNameToBuild) { appearanceNameToBuild = _appearanceNameToBuild; }

			// preferred

			Name const & get_appearance_name_for_owner_to_build() const { return appearanceNameForOwnerToBuild; }
			void set_appearance_name_for_owner_to_build(Name const & _appearanceNameForOwnerToBuild) { appearanceNameForOwnerToBuild = _appearanceNameForOwnerToBuild; }

			Name const & get_appearance_name_for_non_owner_to_build() const { return appearanceNameForNonOwnerToBuild; }
			void set_appearance_name_for_non_owner_to_build(Name const & _appearanceNameForNonOwnerToBuild) { appearanceNameForNonOwnerToBuild = _appearanceNameForNonOwnerToBuild; }

		public:
			void reset_custom();

			// this is to create copy of a presence link using given material to render it twice or render replacing a material
			// setup
			struct AdditionalRenderOfParams
			{
				ADD_FUNCTION_PARAM_PLAIN_INITIAL(AdditionalRenderOfParams, IModulesOwner const*, imo, for_imo, nullptr);
				ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(AdditionalRenderOfParams, bool, forDoorInRooms, for_door_in_rooms, false, true);
				ADD_FUNCTION_PARAM_PLAIN(AdditionalRenderOfParams, Name, tagged, for_tagged);
				ADD_FUNCTION_PARAM_PLAIN_INITIAL(AdditionalRenderOfParams, Room const *, notInRoom, for_not_in_room, nullptr);
				ADD_FUNCTION_PARAM_PLAIN_INITIAL(AdditionalRenderOfParams, ::System::MaterialInstance const *, useMaterial, use_material, nullptr);
			};
			void set_additional_render_of(AdditionalRenderOfParams const & _params);
			void set_override_material_render_of(IModulesOwner const * _owner, ::System::MaterialInstance const* _useMaterial, int _partsAsBits = 0xffffffff);

			// using
			void for_additional_render_of(Room const* _inRoom, IModulesOwner const* _owner, std::function<void(::System::MaterialInstance const* _material)> _do) const;
			void for_additional_render_of(Room const* _inRoom, DoorInRoom const* _doorInRoom, std::function<void(::System::MaterialInstance const* _material)> _do) const;
			void for_override_material_render_of(Room const* _inRoom, IModulesOwner const * _owner, std::function<void(::System::MaterialInstance const* _material, int _partsAsBits)> _do) const;

		public:
#ifdef AN_RENDERER_STATS
			Stats & access_stats() { return stats; }
			Stats const & get_stats() const { return stats; }
#endif

#ifdef AN_DEVELOPMENT
		public:
			int roomProxyCount = 0;
			int presenceLinkProxyCount = 0;
			int doorInRoomProxyCount = 0;
#endif
		private:
			SceneBuildRequest buildRequest;

			Scene* scene = nullptr;
			SceneView::Type sceneView;
			Camera camera;
			::System::ViewFrustum* viewFrustum;
			::System::ViewPlanes viewPlanes;
			Matrix44 viewMatrix;
			Matrix44 throughDoorMatrix;
			Range planes;
			float aspectRatio;
			Vector2 fovSize;
			float nearPlane;
			float maxDoorDist;
			int depth;
			static int depthLimit;
			// more important
			Name appearanceNameForOwnerToBuild; // chosen for presence link owner (fpp/tpp)
			Name appearanceNameForNonOwnerToBuild; // chosen for non presence link owner (seen from far)
			// if ones above fail, this is chosen
			Name appearanceNameToBuild;
			// if this fails too, the first visible is chosen

#ifdef AN_RENDERER_STATS
			Stats stats;
#endif

			struct AdditionalRenderOf
			{
				IModulesOwner const * imo = nullptr;
				bool forDoorInRooms = false;
				Name tagged;
				Room const* notInRoom = nullptr;
				::System::MaterialInstance const* useMaterial = nullptr;
			};
			Array<AdditionalRenderOf> additionalRendersOf;

			struct OverrideMaterialRenderOf
			{
				IModulesOwner const * imo;
				::System::MaterialInstance const* useMaterial;
				int partsAsBits = 0xfffffff; // all
			};
			Array<OverrideMaterialRenderOf> overrideMaterialRendersOf;

		private: // inaccessible, don't copy viewFrustum!
			SceneBuildContext(SceneBuildContext const & _other);
			SceneBuildContext & operator =(SceneBuildContext const & _other);
		};

	};
};
