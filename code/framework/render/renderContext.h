#pragma once

#include "..\..\core\system\video\viewFrustum.h"
#include "..\..\core\system\video\video3d.h"
#include "..\..\core\mesh\mesh3dBuilder.h"
#include "..\..\core\system\video\shaderProgramBindingContext.h"
#include "stats.h"

#ifdef AN_DEVELOPMENT
#define AN_USE_RENDER_CONTEXT_DEBUG
#endif

namespace System
{
	struct MaterialInstance;
};

namespace Framework
{
	namespace Render
	{
		struct ContextDebug
		{
			int dontRenderRoomProxyIndex;
			bool dontRenderRoomProxyInverse;
			
			//

			static ContextDebug* use;

			int renderingRoomProxyIndex;

			ContextDebug()
			: dontRenderRoomProxyIndex(NONE)
			, dontRenderRoomProxyInverse(false)
			{
			}

			void start_rendering()
			{
				renderingRoomProxyIndex = NONE;
			}

			bool should_render_room_proxy()
			{
				return (!dontRenderRoomProxyInverse && dontRenderRoomProxyIndex != renderingRoomProxyIndex) ||
					   (dontRenderRoomProxyInverse && dontRenderRoomProxyIndex == renderingRoomProxyIndex);
			}

			bool should_render_room_alone()
			{
				return dontRenderRoomProxyInverse;
			}

			void next_room_proxy()
			{
				++ renderingRoomProxyIndex;
			}
		};

		/**
		 *	Context holds:
		 *		various information about current state of rendering (depth stencil etc)
		 *		various contexts used for rendering
		 *			mesh rendering context that is used by meshes for rendering
		 *
		 *	Mesh rendering context points at shader program binding context
		 *	Shader program binding context uses default texture provided by this context
		 */
		class Context
		{
		public:
			static void initialise_static();
			static void close_static();

			static ::System::VertexFormat& plane_vertex_format() { an_assert(s_planeVertexFormat);  return *s_planeVertexFormat; }

			Context(::System::Video3D* _v3d);
			~Context();

			inline ::System::Video3D* get_video_3d() const { return video3d; }

			bool get_vr() const { return vr; }
			int get_eye_idx() const { return eyeIdx; }

			void set_vr(bool _vr) { vr = _vr; }
			void set_eye_idx(int _eyeIdx) { eyeIdx = _eyeIdx; }

			Matrix44 const & get_view_matrix(int _eyeIdx) const { return viewMatrix[_eyeIdx]; }
			void swap_view_matrix(int _eyeIdx, Matrix44 & _viewMatrix) { swap(viewMatrix[_eyeIdx], _viewMatrix); }

			uint32 get_current_stencil_depth() const { return currentStencilDepth; }
			void increase_stencil_depth() { ++ currentStencilDepth; }
			void decrease_stencil_depth() { -- currentStencilDepth; }

			void setup_far_plane(float _fov, float _aspectRatio, float _farPlane);
			void setup_background_far_plane(float _fov, float _aspectRatio, float _farPlane);
			void render_far_plane(Optional<Range3> const & _onlyForFlatRange = NP, ::System::MaterialInstance const* _usingMaterial = nullptr);
			void render_background_far_plane(Optional<Range3> const& _onlyForFlatRange = NP, ::System::MaterialInstance const* _usingMaterial = nullptr);

			float get_far_plane() const { return farPlaneValues.farPlane; }
			float get_background_far_plane() const { return backgroundFarPlaneValues.farPlane; }

			void set_mesh_rendering_context(::Meshes::Mesh3DRenderingContext const & _meshRenderingContext);
			::Meshes::Mesh3DRenderingContext const & get_mesh_rendering_context() const { return meshRenderingContext; }
			// no access as depends on shader program binding context

			void set_shader_program_binding_context(::System::ShaderProgramBindingContext const & _shaderProgramBindingContext);
			::System::ShaderProgramBindingContext const & get_shader_program_binding_context() const { return shaderProgramBindingContext; }
			::System::NamedShaderParam& access_shader_param(Name const & _name) { return shaderProgramBindingContext.access_shader_param(_name); }
			// no access as depends on default texture id

			void set_default_texture_id(::System::TextureID const & _textureID);
			::System::TextureID const & get_default_texture_id() const { return defaultTextureID; }

			void set_foreground_scale_out_position(float _foregroundScaleOutPosition) { foregroundScaleOutPosition = _foregroundScaleOutPosition; }
			float get_foreground_scale_out_position() const { return foregroundScaleOutPosition; }

#ifdef AN_USE_RENDER_CONTEXT_DEBUG
			ContextDebug & access_debug() { return debug; }
#endif

		private:
			static ::System::VertexFormat* s_planeVertexFormat;

			::System::Video3D* video3d;

			bool vr = false;
			int eyeIdx = 0;

			Matrix44 viewMatrix[2];

			uint32 currentStencilDepth = 0;

			struct Values
			{
				float fov;
				float farPlane;
				Vector3 farPlaneValues;
			};
			Values farPlaneValues;
			Values backgroundFarPlaneValues;

			::Meshes::Mesh3DRenderingContext meshRenderingContext; // this is general mesh rendering context, shader program binding context in it is used from this class, bones providers are set by individual mesh 3d instances
			::System::ShaderProgramBindingContext shaderProgramBindingContext; // to setup meshRenderingContext, this holds parameters shader by all
			::System::TextureID defaultTextureID;
			float foregroundScaleOutPosition = 1.0f;

#ifdef AN_USE_RENDER_CONTEXT_DEBUG
			ContextDebug debug;
#endif

			void internal_setup_far_plane(Values& fpv, float _fov, float _aspectRatio, float _farPlane);
			void internal_render_far_plane(Values const & fpv, Optional<Range3> const& _onlyForFlatRange, ::System::MaterialInstance const * _usingMaterial);

			Context(Context const & _other); // inaccessible
			Context & operator=(Context const & _other); // inaccessible
		};

	};
};
