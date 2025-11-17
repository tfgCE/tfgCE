#pragma once

#include "pipelines.h"
#include "..\..\core\system\video\material.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\shaderSource.h"
#include "..\..\core\system\video\fragmentShaderOutputSetup.h"
#include "..\..\core\system\video\renderTargetPtr.h"

namespace Framework
{
	/**
	 *	How it works?
	 *		We render to two render targets
	 *			a) graphics - same resolution as our final image
	 *				one texture with one channel
	 *			b) colour grid - resolution smaller, depends on grid size, default should be 8x8
	 *				one texture for ink colour
	 *				one texture for paper colour
	 *				one texture with one channel for brightness
	 *		Then we combine when rendering somewhere else
	 *			from a) we take pixels value, we only have one channel
	 *			if it is 1 we take ink colour
	 *			if it is 0 we take paper colour
	 *			then we take brightness and multpiply (with base)
	 */
	class ColourClashPipeline
	{
	public:
		static void initialise_static();
		static void setup_static();
		static void close_static();

		static ::System::ShaderSourceVersion const& get_version() { an_assert(s_pipeline); return s_pipeline->version; }
		static ::System::ShaderSource const & get_vertex_shader_source() { an_assert(s_pipeline); return s_pipeline->vertexShader; }
		static ::System::ShaderSource const & get_fragment_shader_graphics_source() { an_assert(s_pipeline); return s_pipeline->fragmentShaderGraphics; }
		static ::System::ShaderSource const & get_fragment_shader_colour_grid_source() { an_assert(s_pipeline); return s_pipeline->fragmentShaderColourGrid; }
		static ::System::ShaderSource const & get_fragment_shader_copy_source() { an_assert(s_pipeline); return s_pipeline->fragmentShaderCopy; }
		static ::System::ShaderSource const & get_fragment_shader_combine_source() { an_assert(s_pipeline); return s_pipeline->fragmentShaderCombine; }
		
		static ::System::FragmentShaderOutputSetup const & get_default_graphics_output_definition() { an_assert(s_pipeline); return s_pipeline->defaultGraphicsOutputDefinition; }
		static ::System::FragmentShaderOutputSetup const & get_default_colour_grid_output_definition() { an_assert(s_pipeline); return s_pipeline->defaultColourGridOutputDefinition; }
		static ::System::FragmentShaderOutputSetup const & get_default_combine_output_definition() { an_assert(s_pipeline); return s_pipeline->defaultCombineOutputDefinition; }

		static int get_graphics_shader_program_in_texture_index() { an_assert(s_pipeline); return s_pipeline->graphicsShaderProgram_inTextureIdx; }
		static ::System::ShaderProgramInstance* get_graphics_shader_program_instance() { an_assert(s_pipeline); an_assert(s_pipeline->shadersCreated); return s_pipeline->graphicsShaderProgramInstance; }

		static ::System::VertexFormat const & get_vertex_format() { an_assert(s_pipeline); return s_pipeline->vertexFormat; }

		static bool load_from_xml(IO::XML::Node const * _node) { an_assert(s_pipeline); return s_pipeline->load_setup_from_xml(_node); }

		static void will_use_shaders();
		static void release_shaders();

		static bool are_shaders_created() { return s_pipeline && s_pipeline->shadersCreated; }

		static void create_render_targets(VectorInt2 const & _res, OUT_::System::RenderTargetPtr & _graphicsRT, OUT_::System::RenderTargetPtr & _colourGridRT);

		// _a and _b in graphics space, not colour grid!
		static void draw_rect_on_colour_grid(VectorInt2 const & _a, VectorInt2 const & _b, Optional<Colour> const & _inkColour = NP, Optional<Colour> const & _paperColour = NP, Optional<Colour> const & _brightnessIndicator = NP);
		static void draw_graphics_line(VectorInt2 const & _a, VectorInt2 const & _b, Colour const & _colour = Colour::white);
		static void draw_graphics_point(VectorInt2 const & _a, Colour const & _colour = Colour::white);
		static void draw_graphics_points(VectorInt2 const * _a, int _count, Colour const & _colour = Colour::white);
		static void draw_graphics_fill_rect(VectorInt2 const & _a, VectorInt2 const & _b, Colour const & _colour = Colour::white);

		static void copy(::System::RenderTarget* _srcGraphicsRT, ::System::RenderTarget* _srcColourGridRT, ::System::RenderTarget* _destGraphicsRT, ::System::RenderTarget* _destColourGridRT);
			
		static void combine(VectorInt2 const & _at, VectorInt2 const & _size, ::System::RenderTarget* _graphicsRT, ::System::RenderTarget* _colourGridRT);

	private:
		static ColourClashPipeline* s_pipeline;

		bool shadersCreated = false;
		volatile int shadersInUseCount = 0;

		bool useDefault;

		::System::ShaderSourceVersion version;

		::System::ShaderSource vertexShader;
		::System::ShaderSource fragmentShaderGraphics;
		::System::ShaderSource fragmentShaderColourGrid;
		::System::ShaderSource fragmentShaderCopy;
		::System::ShaderSource fragmentShaderCombine;

		::System::FragmentShaderOutputSetup defaultGraphicsOutputDefinition;
		::System::FragmentShaderOutputSetup defaultColourGridOutputDefinition;
		::System::FragmentShaderOutputSetup defaultCombineOutputDefinition;

		RefCountObjectPtr<::System::ShaderProgram> graphicsShaderProgram;
		RefCountObjectPtr<::System::ShaderProgram> colourGridShaderProgram;
		RefCountObjectPtr<::System::ShaderProgram> copyShaderProgram;
		RefCountObjectPtr<::System::ShaderProgram> combineShaderProgram;
		::System::ShaderProgramInstance* graphicsShaderProgramInstance = nullptr;
		::System::ShaderProgramInstance* colourGridShaderProgramInstance = nullptr;
		::System::ShaderProgramInstance* copyShaderProgramInstance = nullptr;
		::System::ShaderProgramInstance* combineShaderProgramInstance = nullptr;

		::System::VertexFormat vertexFormat; // used by copy and combine

		int graphicsShaderProgram_inTextureIdx = NONE;

		int copyShaderProgram_inInkColourTextureIdx = NONE;
		int copyShaderProgram_inPaperColourTextureIdx = NONE;
		int copyShaderProgram_inBrightnessIndicatorTextureIdx = NONE;

		int colourGridShaderProgram_inInkColourIdx = NONE;
		int colourGridShaderProgram_inPaperColourIdx = NONE;
		int colourGridShaderProgram_inBrightnessIndicatorIdx = NONE;
		
		int combineShaderProgram_inGraphicsTextureIdx = NONE;
		int combineShaderProgram_inGraphicsTextureTexelSizeIdx = NONE;
		int combineShaderProgram_inInkColourTextureIdx = NONE;
		int combineShaderProgram_inPaperColourTextureIdx = NONE;
		int combineShaderProgram_inBrightnessIndicatorTextureIdx = NONE;

		::Concurrency::SpinLock shadersLock = Concurrency::SpinLock(TXT("Framework.ColourClashPipeline.shadersLock"));

		ColourClashPipeline();
		~ColourClashPipeline();

		void clear();

		void setup();

		bool load_setup_from_xml(IO::XML::Node const * _node);

		static void create_shaders();
		static void destroy_shaders();
	};

};
