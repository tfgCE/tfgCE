#pragma once

#include "pipelines.h"
#include "..\..\core\system\video\material.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\shaderSource.h"
#include "..\..\core\system\video\fragmentShaderOutputSetup.h"

namespace Framework
{
	class DisplayPipeline
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

		static ::System::FragmentShaderOutputSetup const & get_default_output_definition() { an_assert(s_pipeline); return s_pipeline->defaultOutputDefinition; }

		static bool load_from_xml(IO::XML::Node const * _node) { an_assert(s_pipeline); return s_pipeline->load_setup_from_xml(_node); }

		static void will_use_shaders();
		static void release_shaders();

		static int get_shader_program_in_texture_index() { an_assert(s_pipeline); return s_pipeline->shaderProgram_inTextureIdx; }
		static int get_shader_program_colourise_index() { an_assert(s_pipeline); return s_pipeline->shaderProgram_colouriseIdx; }
		static int get_shader_program_colourise_ink_index() { an_assert(s_pipeline); return s_pipeline->shaderProgram_colouriseInkIdx; }
		static int get_shader_program_colourise_paper_index() { an_assert(s_pipeline); return s_pipeline->shaderProgram_colourisePaperIdx; }
		static ::System::ShaderProgramInstance* get_shader_program_instance() { an_assert(s_pipeline); an_assert(s_pipeline->shadersCreated); return s_pipeline->shaderProgramInstance; }

		static ::System::ShaderProgramInstance* get_setup_shader_program_instance(::System::TextureID _textureId, bool _colourise, Colour const & _colouriseInk, Colour const & _colourisePaper);

	private:
		static DisplayPipeline* s_pipeline;

		bool shadersCreated = false;
		volatile int shadersInUseCount = 0;
		::Concurrency::SpinLock shadersLock = Concurrency::SpinLock(TXT("Framework.DisplayPipeline.shadersLock"));

		bool useDefault;

		::System::ShaderSourceVersion version;

		::System::ShaderSource vertexShader;
		::System::ShaderSource defaultVertexShader;
		::System::ShaderSource fragmentShader;
		::System::ShaderSource defaultfragmentShader;

		::System::FragmentShaderOutputSetup defaultOutputDefinition;

		RefCountObjectPtr<::System::ShaderProgram> shaderProgram;
		::System::ShaderProgramInstance* shaderProgramInstance = nullptr;

		int shaderProgram_inTextureIdx = NONE;
		int shaderProgram_colouriseIdx = NONE;
		int shaderProgram_colouriseInkIdx = NONE;
		int shaderProgram_colourisePaperIdx = NONE;

		DisplayPipeline();
		~DisplayPipeline();

		void clear();

		void setup();

		bool load_setup_from_xml(IO::XML::Node const * _node);

		static void create_shaders();
		static void destroy_shaders();
	};

};
