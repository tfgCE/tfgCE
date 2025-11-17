#pragma once

#include "pipelines.h"
#include "..\..\core\system\video\material.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\shaderSource.h"

namespace Framework
{
	class RenderingPipeline
	{
	public:
		static void initialise_static();
		static void setup_static();
		static void close_static();

		static ::System::ShaderSourceVersion const& get_version() { an_assert(s_pipeline); return s_pipeline->version; }
		static ::System::ShaderSource const & get_vertex_shader_source(::System::MaterialShaderUsage::Type _forUsage) { an_assert(s_pipeline); an_assert(_forUsage >= 0 && _forUsage < ::System::MaterialShaderUsage::Num); return s_pipeline->vertexShaderByUsage[_forUsage]; }
		static ::System::ShaderSource const & get_default_vertex_shader_source(::System::MaterialShaderUsage::Type _forUsage) { an_assert(s_pipeline); an_assert(_forUsage >= 0 && _forUsage < ::System::MaterialShaderUsage::Num); return s_pipeline->defaultVertexShaderByUsage[_forUsage]; }
		static ::System::ShaderSource const & get_fragment_shader_source() { an_assert(s_pipeline); return s_pipeline->fragmentShader; }
		static ::System::ShaderSource const & get_default_fragment_shader_source() { an_assert(s_pipeline); return s_pipeline->defaultfragmentShader; }
		static ::System::ShaderSource const & get_use_with_rendering_fragment_shader_source() { an_assert(s_pipeline); return s_pipeline->useWithRenderingFragmentShader; }

		static ::System::RenderTargetSetup const & get_default_output_definition() { an_assert(s_pipeline); return s_pipeline->defaultOutputDefinition; }

		static bool load_from_xml(IO::XML::Node const * _node) { an_assert(s_pipeline); return s_pipeline->load_setup_from_xml(_node); }

	private:
		static RenderingPipeline* s_pipeline;

		bool useDefault;

		::System::ShaderSourceVersion version;

		::System::ShaderSource vertexShaderByUsage[::System::MaterialShaderUsage::Num];
		::System::ShaderSource defaultVertexShaderByUsage[::System::MaterialShaderUsage::Num];
		::System::ShaderSource fragmentShader;
		::System::ShaderSource defaultfragmentShader;
		::System::ShaderSource useWithRenderingFragmentShader;

		::System::RenderTargetSetup defaultOutputDefinition;

		RenderingPipeline();

		void clear();

		void setup();

		bool load_setup_from_xml(IO::XML::Node const * _node);
	};

};
