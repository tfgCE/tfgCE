#pragma once

#include "..\..\core\containers\array.h"
#include "..\..\core\memory\refCountObject.h"

#include <functional>

#define LIGHTS_PER_ROOM_ONLY

struct Matrix44;

namespace System
{
	class RenderTarget;
	class ShaderProgram;
	struct RenderTargetSetup;
};

namespace Framework
{
	class EnvironmentType;
	class Library;
	class ShaderProgram;

	namespace Render
	{
		class DoorInRoomProxy;
		class PresenceLinkProxy;

		class System
		{
		public:
			static void initialise_static();
			static void close_static();

			static System* get() { an_assert(s_system); return s_system; }
			
			static ::System::RenderTarget* get_render_target_for(::System::RenderTarget* _renderTarget, OUT_ bool* _newOne = nullptr, std::function<void(::System::RenderTargetSetup& _setup)> _alterSetup = nullptr);

			static ::System::ShaderProgram* get_less_pixel_prepare_stencil_shader_program();
			static ::System::ShaderProgram* get_less_pixel_prepare_resolve_shader_program();
			static ::System::ShaderProgram* get_less_pixel_resolve_shader_program();
			static ::System::ShaderProgram* get_less_pixel_test_resolve_shader_program();

			static EnvironmentType* get_default_fallback_environment_type();

		public:
			std::function<void(::System::ShaderProgram*, Matrix44 const& _renderingBufferModelViewMatrixForRendering, Matrix44 const& _placement, ::Framework::Render::PresenceLinkProxy const *)> setup_shader_program_on_bound_for_presence_link = nullptr;
			std::function<void(::System::ShaderProgram*, Matrix44 const& _renderingBufferModelViewMatrixForRendering, ::Framework::Render::DoorInRoomProxy const*)> setup_shader_program_on_bound_for_door_in_room = nullptr;

		private:
			static System* s_system;

			struct RenderTarget
			{
				::System::RenderTarget* forRenderTarget;
				RefCountObjectPtr<::System::RenderTarget> renderTarget;
			};
			Array<RenderTarget> renderTargets;

			Library* forLibrary = nullptr;
			ShaderProgram* lessPixelPrepareStencilShaderProgram = nullptr;
			ShaderProgram* lessPixelPrepareResolveShaderProgram = nullptr;
			ShaderProgram* lessPixelResolveShaderProgram = nullptr;
			ShaderProgram* lessPixelTestResolveShaderProgram = nullptr;

			EnvironmentType* defaultFallbackEnvironmentType = nullptr; // to fill all missing params

			void clean_up_for_library(Framework::Library* _library);
		};
	};
};
