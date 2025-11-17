#pragma once

#include "pipelines.h"
#include "material.h"
#include "renderTarget.h"
#include "shaderSource.h"
#include "fragmentShaderOutputSetup.h"

namespace System
{
	class PrimitivesPipeline
	{
	public:
		static void initialise_static();
		static void setup_static();
		static void close_static();

		static ShaderSourceVersion const& get_version() { an_assert(s_pipeline); return s_pipeline->version; }
		static ShaderSource const & get_vertex_shader_for_2D_source() { an_assert(s_pipeline); return s_pipeline->vertexShader2D; }
		static ShaderSource const & get_vertex_shader_for_3D_source() { an_assert(s_pipeline); return s_pipeline->vertexShader3D; }
		static ShaderSource const & get_default_vertex_shader_source() { an_assert(s_pipeline); return s_pipeline->defaultVertexShader; }
		static ShaderSource const & get_fragment_shader_basic_source() { an_assert(s_pipeline); return s_pipeline->fragmentShaderBasic; }
		static ShaderSource const & get_fragment_shader_with_texture_source() { an_assert(s_pipeline); return s_pipeline->fragmentShaderWithTexture; }
		static ShaderSource const & get_fragment_shader_without_texture_source() { an_assert(s_pipeline); return s_pipeline->fragmentShaderWithoutTexture; }
		static ShaderSource const & get_fragment_shader_with_texture_discards_blending_source() { an_assert(s_pipeline); return s_pipeline->fragmentShaderWithTextureDiscardsBlending; }
		static ShaderSource const & get_fragment_shader_without_texture_discards_blending_source() { an_assert(s_pipeline); return s_pipeline->fragmentShaderWithoutTextureDiscardsBlending; }
		static ShaderSource const& get_use_with_primitives_fragment_shader_source() { an_assert(s_pipeline); return s_pipeline->useWithPrimitivesFragmentShader; }

		static ::System::RenderTargetSetup const& get_default_output_definition() { an_assert(s_pipeline); return s_pipeline->defaultOutputDefinition; }

		static bool load_from_xml(IO::XML::Node const * _node) { an_assert(s_pipeline); return s_pipeline->load_setup_from_xml(_node); }

	private:
		static PrimitivesPipeline* s_pipeline;

		bool useDefault;

		ShaderSourceVersion version;

		ShaderSource vertexShader2D;
		ShaderSource vertexShader3D;
		ShaderSource defaultVertexShader;
		ShaderSource fragmentShaderBasic; // copies colour only
		ShaderSource fragmentShaderWithTexture;
		ShaderSource fragmentShaderWithoutTexture;
		ShaderSource fragmentShaderWithTextureDiscardsBlending;
		ShaderSource fragmentShaderWithoutTextureDiscardsBlending;
		ShaderSource useWithPrimitivesFragmentShader;

		::System::RenderTargetSetup defaultOutputDefinition;

		PrimitivesPipeline();

		void clear();

		void setup();

		bool load_setup_from_xml(IO::XML::Node const * _node);
	};

};
