#pragma once

#include "pipelines.h"
#include "..\..\core\system\video\material.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\shaderSource.h"
#include "..\..\core\system\video\fragmentShaderOutputSetup.h"

namespace Framework
{
	class UIPipeline
	{
	public:
		static void initialise_static();
		static void setup_static();
		static void close_static();

		static ::System::ShaderSourceVersion const& get_version() { an_assert(s_pipeline); return s_pipeline->version; }
		static ::System::ShaderSource const & get_vertex_shader_source() { an_assert(s_pipeline); return s_pipeline->vertexShader; }
		static ::System::ShaderSource const & get_default_vertex_shader_source() { an_assert(s_pipeline); return s_pipeline->defaultVertexShader; }
		static ::System::ShaderSource const & get_fragment_shader_source() { an_assert(s_pipeline); return s_pipeline->fragmentShader; }
		static ::System::ShaderSource const & get_default_fragment_shader_source() { an_assert(s_pipeline); return s_pipeline->defaultfragmentShader; }
		static ::System::ShaderSource const & get_use_with_ui_fragment_shader_source() { an_assert(s_pipeline); return s_pipeline->useWithUIFragmentShader; }

		static ::System::FragmentShaderOutputSetup const & get_default_output_definition() { an_assert(s_pipeline); return s_pipeline->defaultOutputDefinition; }

		static bool load_from_xml(IO::XML::Node const * _node) { an_assert(s_pipeline); return s_pipeline->load_setup_from_xml(_node); }

	private:
		static UIPipeline* s_pipeline;

		bool useDefault;

		::System::ShaderSourceVersion version;

		::System::ShaderSource vertexShader;
		::System::ShaderSource defaultVertexShader;
		::System::ShaderSource fragmentShader;
		::System::ShaderSource defaultfragmentShader;
		::System::ShaderSource useWithUIFragmentShader;

		::System::FragmentShaderOutputSetup defaultOutputDefinition;

		UIPipeline();

		void clear();

		void setup();

		bool load_setup_from_xml(IO::XML::Node const * _node);
	};

};
