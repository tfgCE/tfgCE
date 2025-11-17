#include "renderSystem.h"

#include "..\library\library.h"

#include "..\..\core\utils.h"
#include "..\..\core\system\video\renderTarget.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME_STR(lessPixelPrepareStencil, TXT("less pixel prepare stencil"));
DEFINE_STATIC_NAME_STR(lessPixelPrepareResolve, TXT("less pixel prepare resolve"));
DEFINE_STATIC_NAME_STR(lessPixelResolve, TXT("less pixel resolve"));
DEFINE_STATIC_NAME_STR(lessPixelTestResolve, TXT("less pixel test resolve"));
DEFINE_STATIC_NAME_STR(defaultFallbackEnvironment, TXT("default fallback environment"));

//

Framework::Render::System* Framework::Render::System::s_system = nullptr;

void Framework::Render::System::initialise_static()
{
	an_assert(!s_system);
	s_system = new Framework::Render::System();
}

void Framework::Render::System::close_static()
{
	an_assert(s_system);
	delete_and_clear(s_system);
}

::System::RenderTarget* Framework::Render::System::get_render_target_for(::System::RenderTarget* _renderTarget, OUT_ bool * _newOne, std::function<void(::System::RenderTargetSetup & _setup)> _alterSetup)
{
	for_every(rt, s_system->renderTargets)
	{
		if (rt->forRenderTarget == _renderTarget)
		{
			assign_optional_out_param(_newOne, false);
			return rt->renderTarget.get();
		}
	}

	RenderTarget rt;
	rt.forRenderTarget = _renderTarget;
	rt.renderTarget = _renderTarget->create_copy(_alterSetup);
	s_system->renderTargets.push_back(rt);
	assign_optional_out_param(_newOne, true);
	return rt.renderTarget.get();
}

void Framework::Render::System::clean_up_for_library(Framework::Library* _library)
{
	forLibrary = _library;
	lessPixelPrepareStencilShaderProgram = nullptr;
	lessPixelPrepareResolveShaderProgram = nullptr;
	lessPixelResolveShaderProgram = nullptr;
	lessPixelTestResolveShaderProgram = nullptr;
	defaultFallbackEnvironmentType = nullptr;
}

::System::ShaderProgram* Framework::Render::System::get_less_pixel_prepare_stencil_shader_program()
{
	if (Framework::Library::get_current() != s_system->forLibrary)
	{
		s_system->clean_up_for_library(Framework::Library::get_current());
	}
	if (!s_system->lessPixelPrepareStencilShaderProgram)
	{
		Framework::LibraryName lessPixelPrepareStencilName = Framework::Library::get_current()->get_global_references().get_name(NAME(lessPixelPrepareStencil), true);
		s_system->lessPixelPrepareStencilShaderProgram = Framework::Library::get_current()->get_shader_programs().find(lessPixelPrepareStencilName);
	}
	return s_system->lessPixelPrepareStencilShaderProgram ? s_system->lessPixelPrepareStencilShaderProgram->get_shader_program() : nullptr;
}

::System::ShaderProgram* Framework::Render::System::get_less_pixel_resolve_shader_program()
{
	if (Framework::Library::get_current() != s_system->forLibrary)
	{
		s_system->clean_up_for_library(Framework::Library::get_current());
	}
	if (!s_system->lessPixelResolveShaderProgram)
	{
		Framework::LibraryName lessPixelResolveName = Framework::Library::get_current()->get_global_references().get_name(NAME(lessPixelResolve), true);
		s_system->lessPixelResolveShaderProgram = Framework::Library::get_current()->get_shader_programs().find(lessPixelResolveName);
	}
	return s_system->lessPixelResolveShaderProgram ? s_system->lessPixelResolveShaderProgram->get_shader_program() : nullptr;
}

::System::ShaderProgram* Framework::Render::System::get_less_pixel_test_resolve_shader_program()
{
	if (Framework::Library::get_current() != s_system->forLibrary)
	{
		s_system->clean_up_for_library(Framework::Library::get_current());
	}
	if (!s_system->lessPixelTestResolveShaderProgram)
	{
		Framework::LibraryName lessPixelTestResolveName = Framework::Library::get_current()->get_global_references().get_name(NAME(lessPixelTestResolve), true);
		s_system->lessPixelTestResolveShaderProgram = Framework::Library::get_current()->get_shader_programs().find(lessPixelTestResolveName);
	}
	return s_system->lessPixelTestResolveShaderProgram ? s_system->lessPixelTestResolveShaderProgram->get_shader_program() : nullptr;
}

::System::ShaderProgram* Framework::Render::System::get_less_pixel_prepare_resolve_shader_program()
{
	if (Framework::Library::get_current() != s_system->forLibrary)
	{
		s_system->clean_up_for_library(Framework::Library::get_current());
	}
	if (!s_system->lessPixelPrepareResolveShaderProgram)
	{
		Framework::LibraryName lessPixelPrepareResolveName = Framework::Library::get_current()->get_global_references().get_name(NAME(lessPixelPrepareResolve), true);
		s_system->lessPixelPrepareResolveShaderProgram = Framework::Library::get_current()->get_shader_programs().find(lessPixelPrepareResolveName);
	}
	return s_system->lessPixelPrepareResolveShaderProgram ? s_system->lessPixelPrepareResolveShaderProgram->get_shader_program() : nullptr;
}

EnvironmentType* Framework::Render::System::get_default_fallback_environment_type()
{
	if (Framework::Library::get_current() != s_system->forLibrary)
	{
		s_system->clean_up_for_library(Framework::Library::get_current());
	}
	if (!s_system->defaultFallbackEnvironmentType)
	{
		Framework::LibraryName defaultFallbackEnvironmentName = Framework::Library::get_current()->get_global_references().get_name(NAME(defaultFallbackEnvironment), true);
		if (defaultFallbackEnvironmentName.is_valid())
		{
			s_system->defaultFallbackEnvironmentType = Framework::Library::get_current()->get_environment_types().find_may_fail(defaultFallbackEnvironmentName);
		}
	}
	return s_system->defaultFallbackEnvironmentType;
}