#include "colourClashPipeline.h"

#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\mainSettings.h"
#include "..\..\core\mesh\mesh3d.h"
#include "..\..\core\types\names.h"

#include "..\..\core\system\video\video3dPrimitives.h"

using namespace Framework;

//

DEFINE_STATIC_NAME(inkColour);
DEFINE_STATIC_NAME(paperColour);
DEFINE_STATIC_NAME(brightnessIndicator);
DEFINE_STATIC_NAME(inTexture);
DEFINE_STATIC_NAME(inInkColourTexture);
DEFINE_STATIC_NAME(inPaperColourTexture);
DEFINE_STATIC_NAME(inBrightnessIndicatorTexture);
DEFINE_STATIC_NAME(inInkColour);
DEFINE_STATIC_NAME(inPaperColour);
DEFINE_STATIC_NAME(inBrightnessIndicator);
DEFINE_STATIC_NAME(inGraphicsTexture);
DEFINE_STATIC_NAME(inGraphicsTextureTexelSize);

//

ColourClashPipeline* ColourClashPipeline::s_pipeline = nullptr;

void ColourClashPipeline::initialise_static()
{
	an_assert(s_pipeline == nullptr);
	s_pipeline = new ColourClashPipeline();
	s_pipeline->setup();
}

void ColourClashPipeline::setup_static()
{
	an_assert(s_pipeline);
	s_pipeline->setup();
}

void ColourClashPipeline::close_static()
{
	an_assert(s_pipeline);
	delete_and_clear(s_pipeline);
}

ColourClashPipeline::ColourClashPipeline()
: useDefault(true)
{
}

ColourClashPipeline::~ColourClashPipeline()
{
	an_assert(!graphicsShaderProgram.is_set(), TXT("call release_shaders()?"));
	an_assert(!colourGridShaderProgram.is_set(), TXT("call release_shaders()?"));
	an_assert(!copyShaderProgram.is_set(), TXT("call release_shaders()?"));
	an_assert(!combineShaderProgram.is_set(), TXT("call release_shaders()?"));
	an_assert(!graphicsShaderProgramInstance, TXT("call release_shaders()?"));
	an_assert(!colourGridShaderProgramInstance, TXT("call release_shaders()?"));
	an_assert(!copyShaderProgramInstance, TXT("call release_shaders()?"));
	an_assert(!combineShaderProgramInstance, TXT("call release_shaders()?"));
}

bool ColourClashPipeline::load_setup_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	useDefault = false; // no longer allow "setup" to modify things

	clear();

	result &= version.load_from_xml_child_node(_node);

	result &= vertexShader.load_from_xml(_node->first_child_named(TXT("vertexShader")));

	result &= fragmentShaderGraphics.load_from_xml(_node->first_child_named(TXT("fragmentShaderGraphics")));
	result &= fragmentShaderColourGrid.load_from_xml(_node->first_child_named(TXT("fragmentShaderColourGrid")));
	result &= fragmentShaderCopy.load_from_xml(_node->first_child_named(TXT("fragmentShaderCopy")));
	result &= fragmentShaderCombine.load_from_xml(_node->first_child_named(TXT("fragmentShaderCombine")));

	defaultGraphicsOutputDefinition.clear_output_textures();
	result &= defaultGraphicsOutputDefinition.load_from_xml(_node->first_child_named(TXT("graphicsOutputDefinition")), ::System::OutputTextureDefinition());

	defaultColourGridOutputDefinition.clear_output_textures();
	result &= defaultColourGridOutputDefinition.load_from_xml(_node->first_child_named(TXT("colourGridOutputDefinition")), ::System::OutputTextureDefinition());

	defaultCombineOutputDefinition.clear_output_textures();
	result &= defaultCombineOutputDefinition.load_from_xml(_node->first_child_named(TXT("combineOutputDefinition")), ::System::OutputTextureDefinition());

	return true;
}

void ColourClashPipeline::clear()
{
	vertexShader.clear();
	fragmentShaderGraphics.clear();
	fragmentShaderColourGrid.clear();
	fragmentShaderCopy.clear();
	fragmentShaderCombine.clear();
}

void ColourClashPipeline::setup()
{
	if (!useDefault)
	{
		return;
	}

	vertexFormat.with_texture_uv().with_texture_uv_type(::System::DataFormatValueType::Float).no_padding().calculate_stride_and_offsets();

	todo_future(TXT("get source for this from game? no extra processing"));

	version.setup_default();

	//

	clear();

	//

	vertexShader.data
		= String(TXT("\n"))
		+ String(TXT("// uniforms\n"))
		+ String(TXT("uniform mat4 modelViewMatrix;\n"))
		+ String(TXT("uniform mat4 projectionMatrix;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec2 inAPosition;\n"))
		+ String(TXT("in vec2 inOUV;\n"))
		+ String(TXT("in vec4 inOColour;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec2 varUV;\n"))
		+ String(TXT("out vec4 varColour;\n"));
	vertexShader.mainBody
		= String(TXT("// calculate position\n"))
		+ String(TXT("  vec4 prcPosition;\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  prcPosition.xy = inAPosition;\n"))
		+ String(TXT("  prcPosition.z = 0.0;\n"))
		+ String(TXT("  prcPosition.w = 1.0;\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  prcPosition = modelViewMatrix * prcPosition;\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  // fill output\n"))
		+ String(TXT("  gl_Position = projectionMatrix * prcPosition;\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  // just copy\n"))
		+ String(TXT("  varUV = inOUV;\n"))
		+ String(TXT("  varColour = inOColour;\n"))
		+ String(TXT("  \n"));

	//

	fragmentShaderGraphics.data
		= String(TXT("\n"))
		+ String(TXT("// uniforms\n"))
		+ String(TXT("uniform sampler2D inTexture;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec2 varUV;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec4 colour; \n"))
		+ String(TXT("\n"));
	fragmentShaderGraphics.mainBody
		= String(TXT("  vec4 prcColour = texture(inTexture, varUV).rgba;\n"))
		+ String(TXT("  prcColour.r = max(prcColour.r, max(prcColour.g, prcColour.b)) >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  prcColour.g = prcColour.r;\n"))
		+ String(TXT("  prcColour.b = prcColour.r;\n"))
		+ String(TXT("  colour = prcColour;\n"));

	//

	fragmentShaderColourGrid.data
		= String(TXT("\n"))
		+ String(TXT("// uniforms\n"))
		+ String(TXT("uniform vec4 inInkColour;\n"))
		+ String(TXT("uniform vec4 inPaperColour;\n"))
		+ String(TXT("uniform vec4 inBrightnessIndicator;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec4 inkColour; \n"))
		+ String(TXT("out vec4 paperColour; \n"))
		+ String(TXT("out vec4 brightnessIndicator; \n"))
		+ String(TXT("\n"));
	fragmentShaderColourGrid.mainBody
		= String(TXT("  inkColour.r = inInkColour.r >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  inkColour.g = inInkColour.g >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  inkColour.b = inInkColour.b >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  inkColour.a = inInkColour.a;\n"))
		+ String(TXT("  paperColour.r = inPaperColour.r >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  paperColour.g = inPaperColour.g >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  paperColour.b = inPaperColour.b >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  paperColour.a = inPaperColour.a;\n"))
		+ String(TXT("  brightnessIndicator.r = inBrightnessIndicator.r >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  brightnessIndicator.g = inBrightnessIndicator.g >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  brightnessIndicator.b = inBrightnessIndicator.b >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  brightnessIndicator.a = inBrightnessIndicator.a;\n"));
	
	//

	fragmentShaderCopy.data
		= String(TXT("\n"))
		+ String(TXT("// uniforms\n"))
		+ String(TXT("uniform sampler2D inInkColourTexture;\n"))
		+ String(TXT("uniform sampler2D inPaperColourTexture;\n"))
		+ String(TXT("uniform sampler2D inBrightnessIndicatorTexture;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec2 varUV;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec4 inkColour; \n"))
		+ String(TXT("out vec4 paperColour; \n"))
		+ String(TXT("out vec4 brightnessIndicator; \n"))
		+ String(TXT("\n"));
	fragmentShaderCopy.mainBody
		= String(TXT("  inkColour = texture(inInkColourTexture, varUV);\n"))
		+ String(TXT("  paperColour = texture(inPaperColourTexture, varUV);\n"))
		+ String(TXT("  brightnessIndicator = texture(inBrightnessIndicatorTexture, varUV);\n"))
		+ String(TXT("  inkColour.r = inkColour.r >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  inkColour.g = inkColour.g >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  inkColour.b = inkColour.b >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  inkColour.a = inkColour.a;\n"))
		+ String(TXT("  paperColour.r = paperColour.r >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  paperColour.g = paperColour.g >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  paperColour.b = paperColour.b >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  paperColour.a = paperColour.a;\n"))
		+ String(TXT("  brightnessIndicator.r = brightnessIndicator.r >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  brightnessIndicator.g = brightnessIndicator.g >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  brightnessIndicator.b = brightnessIndicator.b >= 0.5? 1.0 : 0.0;\n"))
		+ String(TXT("  brightnessIndicator.a = brightnessIndicator.a;\n"));

	//

	fragmentShaderCombine.data
		= String(TXT("\n"))
		+ String(TXT("// uniforms\n"))
		+ String(TXT("uniform sampler2D inGraphicsTexture;\n"))
		+ String(TXT("uniform vec2 inGraphicsTextureTexelSize;\n"))
		+ String(TXT("uniform sampler2D inInkColourTexture;\n"))
		+ String(TXT("uniform sampler2D inPaperColourTexture;\n"))
		+ String(TXT("uniform sampler2D inBrightnessIndicatorTexture;\n"))
		+ String(TXT("uniform float inNotBrightMin = 0.0;\n"))
		+ String(TXT("uniform float inNotBrightMax = 0.8;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec2 varUV;\n"))
		+ String(TXT("in vec4 varColour;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec4 colour; \n"))
		+ String(TXT("\n"));
	fragmentShaderCombine.mainBody
		= String(TXT("  vec2 varUVCG = varUV;\n"))
		+ String(TXT("  // get brightness\n"))
		+ String(TXT("  float brightnessIndicator = texture(inBrightnessIndicatorTexture, varUVCG).r;\n"))
		+ String(TXT("  vec4 notBrightBlack = vec4(inNotBrightMin, inNotBrightMin, inNotBrightMin, 1.0);\n"))
		+ String(TXT("  // get colours and apply brightness\n"))
		+ String(TXT("  vec4 inkColour = texture(inInkColourTexture, varUVCG).rgba;\n"))
		+ String(TXT("  vec4 paperColour = texture(inPaperColourTexture, varUVCG).rgba;\n"))
		+ String(TXT("  inkColour = inkColour * brightnessIndicator + (1.0 - brightnessIndicator) * (inkColour * inNotBrightMax + notBrightBlack);\n"))
		+ String(TXT("  paperColour = paperColour * brightnessIndicator + (1.0 - brightnessIndicator) * (paperColour * inNotBrightMax + notBrightBlack);\n"))
		+ String(TXT("  // get graphics and apply colour\n"))
		+ String(TXT("  vec4 graphicsColour = texture(inGraphicsTexture, varUV).rgba;\n"))
		+ String(TXT("  graphicsColour.rgb = graphicsColour.r * inkColour.rgb + (1.0 - graphicsColour.r) * paperColour.rgb;\n"))
		+ String(TXT("  colour = graphicsColour;\n"))
		+ String(TXT("  colour.a = 1.0;\n"));
	
	//

	defaultGraphicsOutputDefinition.clear_output_textures();
	defaultGraphicsOutputDefinition.add_output_texture(::System::OutputTextureDefinition(Names::colour,
		::System::VideoFormat::rgb8,
		::System::BaseVideoFormat::rgba,
		false,
		::System::TextureFiltering::nearest,
		::System::TextureFiltering::nearest));

	//

	defaultColourGridOutputDefinition.clear_output_textures();
	defaultColourGridOutputDefinition.add_output_texture(::System::OutputTextureDefinition(NAME(inkColour),
		::System::VideoFormat::rgba8,
		::System::BaseVideoFormat::rgba,
		false,
		::System::TextureFiltering::nearest,
		::System::TextureFiltering::nearest));
	defaultColourGridOutputDefinition.add_output_texture(::System::OutputTextureDefinition(NAME(paperColour),
		::System::VideoFormat::rgba8,
		::System::BaseVideoFormat::rgba,
		false,
		::System::TextureFiltering::nearest,
		::System::TextureFiltering::nearest));
	defaultColourGridOutputDefinition.add_output_texture(::System::OutputTextureDefinition(NAME(brightnessIndicator),
		::System::VideoFormat::rgba8,
		::System::BaseVideoFormat::rgba,
		false,
		::System::TextureFiltering::nearest,
		::System::TextureFiltering::nearest));

	//

	defaultCombineOutputDefinition.clear_output_textures();
	defaultCombineOutputDefinition.add_output_texture(::System::OutputTextureDefinition(Names::colour,
		::System::VideoFormat::rgba8,
		::System::BaseVideoFormat::rgba,
		false,
		::System::TextureFiltering::nearest,
		::System::TextureFiltering::nearest));

	//

}

void ColourClashPipeline::will_use_shaders()
{
	if (!s_pipeline)
	{
		return;
	}

	::Concurrency::ScopedSpinLock lock(s_pipeline->shadersLock);

	if (s_pipeline->shadersInUseCount == 0)
	{
		create_shaders();
	}

	++s_pipeline->shadersInUseCount;
}

void ColourClashPipeline::release_shaders()
{
	if (!s_pipeline)
	{
		return;
	}

	::Concurrency::ScopedSpinLock lock(s_pipeline->shadersLock);

	--s_pipeline->shadersInUseCount;

	if (s_pipeline->shadersInUseCount == 0)
	{
		destroy_shaders();
	}
}

void ColourClashPipeline::create_shaders()
{
	an_assert(s_pipeline);
	an_assert(! s_pipeline->shadersCreated);

	assert_rendering_thread();

	RefCountObjectPtr<::System::ShaderSetup> vertexShaderSetup(::System::ShaderSetup::from_source(::System::ShaderType::Vertex, get_vertex_shader_source()));
	RefCountObjectPtr<::System::ShaderSetup> graphicsShaderSetup(::System::ShaderSetup::from_source(::System::ShaderType::Fragment, get_fragment_shader_graphics_source()));
	RefCountObjectPtr<::System::ShaderSetup> colourGridShaderSetup(::System::ShaderSetup::from_source(::System::ShaderType::Fragment, get_fragment_shader_colour_grid_source()));
	RefCountObjectPtr<::System::ShaderSetup> copyShaderSetup(::System::ShaderSetup::from_source(::System::ShaderType::Fragment, get_fragment_shader_copy_source()));
	RefCountObjectPtr<::System::ShaderSetup> combineShaderSetup(::System::ShaderSetup::from_source(::System::ShaderType::Fragment, get_fragment_shader_combine_source()));

	vertexShaderSetup->fill_version(get_version());
	graphicsShaderSetup->fill_version(get_version());
	colourGridShaderSetup->fill_version(get_version());
	copyShaderSetup->fill_version(get_version());
	combineShaderSetup->fill_version(get_version());

	graphicsShaderSetup->get_fragment_output_setup()->copy_output_textures_from(get_default_graphics_output_definition());
	colourGridShaderSetup->get_fragment_output_setup()->copy_output_textures_from(get_default_colour_grid_output_definition());
	copyShaderSetup->get_fragment_output_setup()->copy_output_textures_from(get_default_colour_grid_output_definition());
	combineShaderSetup->get_fragment_output_setup()->copy_output_textures_from(get_default_combine_output_definition());

	::System::Shader* vertexShader = new ::System::Shader(vertexShaderSetup.get());
	::System::Shader* graphicsShader = new ::System::Shader(graphicsShaderSetup.get());
	::System::Shader* colourGridShader = new ::System::Shader(colourGridShaderSetup.get());
	::System::Shader* copyShader = new ::System::Shader(copyShaderSetup.get());
	::System::Shader* combineShader = new ::System::Shader(combineShaderSetup.get());

	s_pipeline->graphicsShaderProgram = new ::System::ShaderProgram(vertexShader, graphicsShader);
	s_pipeline->colourGridShaderProgram = new ::System::ShaderProgram(vertexShader, colourGridShader);
	s_pipeline->copyShaderProgram = new ::System::ShaderProgram(vertexShader, copyShader);
	s_pipeline->combineShaderProgram = new ::System::ShaderProgram(vertexShader, combineShader);
	
	s_pipeline->graphicsShaderProgramInstance = new ::System::ShaderProgramInstance();
	s_pipeline->colourGridShaderProgramInstance = new ::System::ShaderProgramInstance();
	s_pipeline->copyShaderProgramInstance = new ::System::ShaderProgramInstance();
	s_pipeline->combineShaderProgramInstance = new ::System::ShaderProgramInstance();

	s_pipeline->graphicsShaderProgramInstance->set_shader_program(s_pipeline->graphicsShaderProgram.get());
	s_pipeline->colourGridShaderProgramInstance->set_shader_program(s_pipeline->colourGridShaderProgram.get());
	s_pipeline->copyShaderProgramInstance->set_shader_program(s_pipeline->copyShaderProgram.get());
	s_pipeline->combineShaderProgramInstance->set_shader_program(s_pipeline->combineShaderProgram.get());
	
	if (s_pipeline->graphicsShaderProgram.is_set())
	{
		s_pipeline->graphicsShaderProgram_inTextureIdx = s_pipeline->graphicsShaderProgram->find_uniform_index(NAME(inTexture));
	}

	if (s_pipeline->copyShaderProgram.is_set())
	{
		s_pipeline->copyShaderProgram_inInkColourTextureIdx = s_pipeline->copyShaderProgram->find_uniform_index(NAME(inInkColourTexture));
		s_pipeline->copyShaderProgram_inPaperColourTextureIdx = s_pipeline->copyShaderProgram->find_uniform_index(NAME(inPaperColourTexture));
		s_pipeline->copyShaderProgram_inBrightnessIndicatorTextureIdx = s_pipeline->copyShaderProgram->find_uniform_index(NAME(inBrightnessIndicatorTexture));
	}

	if (s_pipeline->colourGridShaderProgram.is_set())
	{
		s_pipeline->colourGridShaderProgram_inInkColourIdx = s_pipeline->colourGridShaderProgram->find_uniform_index(NAME(inInkColour));
		s_pipeline->colourGridShaderProgram_inPaperColourIdx = s_pipeline->colourGridShaderProgram->find_uniform_index(NAME(inPaperColour));
		s_pipeline->colourGridShaderProgram_inBrightnessIndicatorIdx = s_pipeline->colourGridShaderProgram->find_uniform_index(NAME(inBrightnessIndicator));
	}

	if (s_pipeline->combineShaderProgram.is_set())
	{
		s_pipeline->combineShaderProgram_inGraphicsTextureIdx = s_pipeline->combineShaderProgram->find_uniform_index(NAME(inGraphicsTexture));
		s_pipeline->combineShaderProgram_inGraphicsTextureTexelSizeIdx = s_pipeline->combineShaderProgram->find_uniform_index(NAME(inGraphicsTextureTexelSize));
		s_pipeline->combineShaderProgram_inInkColourTextureIdx = s_pipeline->combineShaderProgram->find_uniform_index(NAME(inInkColourTexture));
		s_pipeline->combineShaderProgram_inPaperColourTextureIdx = s_pipeline->combineShaderProgram->find_uniform_index(NAME(inPaperColourTexture));
		s_pipeline->combineShaderProgram_inBrightnessIndicatorTextureIdx = s_pipeline->combineShaderProgram->find_uniform_index(NAME(inBrightnessIndicatorTexture));
	}

	s_pipeline->shadersCreated = true;
}

void ColourClashPipeline::destroy_shaders()
{
	an_assert(s_pipeline);
	an_assert(s_pipeline->shadersCreated);

	assert_rendering_thread();

	delete_and_clear(s_pipeline->graphicsShaderProgramInstance);
	delete_and_clear(s_pipeline->colourGridShaderProgramInstance);
	delete_and_clear(s_pipeline->copyShaderProgramInstance);
	delete_and_clear(s_pipeline->combineShaderProgramInstance);

	s_pipeline->graphicsShaderProgram = nullptr;
	s_pipeline->colourGridShaderProgram = nullptr;
	s_pipeline->copyShaderProgram = nullptr;
	s_pipeline->combineShaderProgram = nullptr;

	s_pipeline->shadersCreated = false;
}

void ColourClashPipeline::create_render_targets(VectorInt2 const & _res, OUT_::System::RenderTargetPtr & _graphicsRT, OUT_::System::RenderTargetPtr & _colourGridRT)
{
	an_assert(s_pipeline);
	an_assert(! _graphicsRT.is_set());
	an_assert(! _colourGridRT.is_set());
	an_assert((_res.x % 8) == 0, TXT("only 8x8 are supported for time being"));
	an_assert((_res.y % 8) == 0, TXT("only 8x8 are supported for time being"));

	::System::RenderTargetSetup graphicsRTSetup;
	graphicsRTSetup.copy_output_textures_from(s_pipeline->defaultGraphicsOutputDefinition);
	graphicsRTSetup.set_size(_res);
	_graphicsRT = new ::System::RenderTarget(graphicsRTSetup);

	::System::RenderTargetSetup colourGridRTSetup;
	colourGridRTSetup.copy_output_textures_from(s_pipeline->defaultColourGridOutputDefinition);
	colourGridRTSetup.set_size(_res / 8);
	_colourGridRT = new ::System::RenderTarget(colourGridRTSetup);
}

void ColourClashPipeline::draw_graphics_line(VectorInt2 const & _a, VectorInt2 const & _b, Colour const & _colour)
{
	Vector2 a = _a.to_vector2() + Vector2::half;
	Vector2 b = _b.to_vector2() + Vector2::half;
	::System::Video3DPrimitives::line_2d(_colour, a, b, false);
}

void ColourClashPipeline::draw_graphics_point(VectorInt2 const & _a, Colour const & _colour)
{
	::System::Video3DPrimitives::point_2d(_colour, _a.to_vector2() + Vector2::half, false);
}

void ColourClashPipeline::draw_graphics_points(VectorInt2 const * _a, int _count, Colour const & _colour)
{
	ARRAY_STACK(Vector2, points, _count);
	VectorInt2 const * src = _a;
	for_count(int, i, _count)
	{
		points.push_back(src->to_vector2() + Vector2::half);
		++src;
	}
	::System::Video3DPrimitives::points_2d(_colour, points.get_data(), _count, false);
}

void ColourClashPipeline::draw_graphics_fill_rect(VectorInt2 const & _a, VectorInt2 const & _b, Colour const & _colour)
{
	Vector2 a = _a.to_vector2();
	Vector2 b = _b.to_vector2(); // we handle extra line later
	::System::Video3DPrimitives::fill_rect_2d(_colour, a, b, false);
}

void ColourClashPipeline::draw_rect_on_colour_grid(VectorInt2 const & _a, VectorInt2 const & _b, Optional<Colour> const & _inkColour, Optional<Colour> const & _paperColour, Optional<Colour> const & _brightnessIndicator)
{
	an_assert(s_pipeline);
	an_assert(s_pipeline->shadersCreated);
	assert_rendering_thread();
	an_assert(s_pipeline->colourGridShaderProgramInstance);

	if (s_pipeline->colourGridShaderProgram_inInkColourIdx != NONE)
	{
		s_pipeline->colourGridShaderProgramInstance->set_uniform(s_pipeline->colourGridShaderProgram_inInkColourIdx, (_inkColour.is_set() ? _inkColour.get() : Colour::none).to_vector4());
	}
	if (s_pipeline->colourGridShaderProgram_inPaperColourIdx != NONE)
	{
		s_pipeline->colourGridShaderProgramInstance->set_uniform(s_pipeline->colourGridShaderProgram_inPaperColourIdx, (_paperColour.is_set() ? _paperColour.get() : Colour::none).to_vector4());
	}
	if (s_pipeline->colourGridShaderProgram_inBrightnessIndicatorIdx != NONE)
	{
		s_pipeline->colourGridShaderProgramInstance->set_uniform(s_pipeline->colourGridShaderProgram_inBrightnessIndicatorIdx, (_brightnessIndicator.is_set() ? _brightnessIndicator.get() : Colour::none).to_vector4());
	}

	VectorInt2 a = _a;
	VectorInt2 b = _b;
	order(a.x, b.x);
	order(a.y, b.y);
	System::Video3D* v3d = System::Video3D::get();
	v3d->mark_enable_blend();
	::System::Video3DPrimitives::fill_rect_2d(s_pipeline->colourGridShaderProgramInstance, floor(a.to_vector2() / 8.0f), floor(b.to_vector2() / 8.0f));
	v3d->mark_disable_blend();
}

struct ColourClashPipelineVertex
{
	Vector3 location;
	Vector2 uv;

	ColourClashPipelineVertex() {}
	ColourClashPipelineVertex(Vector2 const & _uv, Vector2 const & _loc) : location(Vector3(_loc.x, _loc.y, 0.0f)), uv(_uv) {}
};

void ColourClashPipeline::combine(VectorInt2 const & _at, VectorInt2 const & _size, ::System::RenderTarget* _graphicsRT, ::System::RenderTarget* _colourGridRT)
{
	an_assert(s_pipeline);
	an_assert(s_pipeline->shadersCreated);
	assert_rendering_thread();
	an_assert(s_pipeline->combineShaderProgramInstance);

	if (s_pipeline->combineShaderProgram_inGraphicsTextureIdx != NONE)
	{
		an_assert(_graphicsRT->is_resolved());
		s_pipeline->combineShaderProgramInstance->set_uniform(s_pipeline->combineShaderProgram_inGraphicsTextureIdx, _graphicsRT->get_data_texture_id(0));
	}
	if (s_pipeline->combineShaderProgram_inGraphicsTextureTexelSizeIdx != NONE)
	{
		an_assert(_graphicsRT->is_resolved());
		s_pipeline->combineShaderProgramInstance->set_uniform(s_pipeline->combineShaderProgram_inGraphicsTextureTexelSizeIdx, _graphicsRT->get_size().to_vector2().inverted());
	}
	if (s_pipeline->combineShaderProgram_inInkColourTextureIdx != NONE)
	{
		an_assert(_colourGridRT->is_resolved());
		s_pipeline->combineShaderProgramInstance->set_uniform(s_pipeline->combineShaderProgram_inInkColourTextureIdx, _colourGridRT->get_data_texture_id(0));
	}
	if (s_pipeline->combineShaderProgram_inPaperColourTextureIdx != NONE)
	{
		an_assert(_colourGridRT->is_resolved());
		s_pipeline->combineShaderProgramInstance->set_uniform(s_pipeline->combineShaderProgram_inPaperColourTextureIdx, _colourGridRT->get_data_texture_id(1));
	}
	if (s_pipeline->combineShaderProgram_inBrightnessIndicatorTextureIdx != NONE)
	{
		an_assert(_colourGridRT->is_resolved());
		s_pipeline->combineShaderProgramInstance->set_uniform(s_pipeline->combineShaderProgram_inBrightnessIndicatorTextureIdx, _colourGridRT->get_data_texture_id(2));
	}

	Vector2 a = (_at).to_vector2();
	Vector2 size = _size.to_vector2();
	Vector2 b = (_at + _size).to_vector2(); // yes, because it (opengl) for some reason does not render last line
	ArrayStatic<ColourClashPipelineVertex, 4> vertices; SET_EXTRA_DEBUG_INFO(vertices, TXT("ColourClashPipeline::combine.vertices"));
	Range2 range(Range(a.x, b.x), Range(a.y, b.y));
	Range2 uvRange(Range(0.0f, size.x / _graphicsRT->get_size().x), Range(0.0f, size.y / _graphicsRT->get_size().y));

	vertices.push_back(ColourClashPipelineVertex(Vector2(uvRange.x.min, uvRange.y.min), Vector2(range.x.min, range.y.min)));
	vertices.push_back(ColourClashPipelineVertex(Vector2(uvRange.x.min, uvRange.y.max), Vector2(range.x.min, range.y.max)));
	vertices.push_back(ColourClashPipelineVertex(Vector2(uvRange.x.max, uvRange.y.max), Vector2(range.x.max, range.y.max)));
	vertices.push_back(ColourClashPipelineVertex(Vector2(uvRange.x.max, uvRange.y.min), Vector2(range.x.max, range.y.min)));

	System::Video3D* v3d = System::Video3D::get();

	v3d->mark_disable_blend();

	System::VideoMatrixStack& modelViewStack = v3d->access_model_view_matrix_stack();
	modelViewStack.push_set(Matrix44::identity);
	Meshes::Mesh3D::render_data(v3d, s_pipeline->combineShaderProgramInstance, nullptr, Meshes::Mesh3DRenderingContext::none(),
		vertices.get_data(), ::Meshes::Primitive::TriangleFan, 2, s_pipeline->vertexFormat);
	modelViewStack.pop();
}

void ColourClashPipeline::copy(::System::RenderTarget* _srcGraphicsRT, ::System::RenderTarget* _srcColourGridRT, ::System::RenderTarget* _destGraphicsRT, ::System::RenderTarget* _destColourGridRT)
{
	an_assert(s_pipeline);
	an_assert(s_pipeline->shadersCreated);
	assert_rendering_thread();
	an_assert(s_pipeline->combineShaderProgramInstance);

	System::Video3D* v3d = System::Video3D::get();

	v3d->mark_disable_blend();

	_destGraphicsRT->bind();
	_srcGraphicsRT->render(0, v3d, Vector2::zero, _srcGraphicsRT->get_size().to_vector2());
	_destGraphicsRT->unbind();

	_destColourGridRT->bind();

	if (s_pipeline->copyShaderProgram_inInkColourTextureIdx != NONE)
	{
		an_assert(_srcColourGridRT->is_resolved());
		s_pipeline->copyShaderProgramInstance->set_uniform(s_pipeline->copyShaderProgram_inInkColourTextureIdx, _srcColourGridRT->get_data_texture_id(0));
	}
	if (s_pipeline->copyShaderProgram_inPaperColourTextureIdx != NONE)
	{
		an_assert(_srcColourGridRT->is_resolved());
		s_pipeline->copyShaderProgramInstance->set_uniform(s_pipeline->copyShaderProgram_inPaperColourTextureIdx, _srcColourGridRT->get_data_texture_id(1));
	}
	if (s_pipeline->copyShaderProgram_inBrightnessIndicatorTextureIdx != NONE)
	{
		an_assert(_srcColourGridRT->is_resolved());
		s_pipeline->copyShaderProgramInstance->set_uniform(s_pipeline->copyShaderProgram_inBrightnessIndicatorTextureIdx, _srcColourGridRT->get_data_texture_id(2));
	}

	ArrayStatic<ColourClashPipelineVertex, 4> vertices; SET_EXTRA_DEBUG_INFO(vertices, TXT("ColourClashPipeline::copy.vertices"));
	Range2 range(Range(0.0f, (float)_destColourGridRT->get_size().x), Range(0.0f, (float)_destColourGridRT->get_size().y));
	Range2 uvRange(Range(0.0f, 1.0f), Range(0.0f, 1.0f));

	vertices.push_back(ColourClashPipelineVertex(Vector2(uvRange.x.min, uvRange.y.min), Vector2(range.x.min, range.y.min)));
	vertices.push_back(ColourClashPipelineVertex(Vector2(uvRange.x.min, uvRange.y.max), Vector2(range.x.min, range.y.max)));
	vertices.push_back(ColourClashPipelineVertex(Vector2(uvRange.x.max, uvRange.y.max), Vector2(range.x.max, range.y.max)));
	vertices.push_back(ColourClashPipelineVertex(Vector2(uvRange.x.max, uvRange.y.min), Vector2(range.x.max, range.y.min)));

	System::VideoMatrixStack& modelViewStack = v3d->access_model_view_matrix_stack();
	modelViewStack.push_set(Matrix44::identity);
	Meshes::Mesh3D::render_data(v3d, s_pipeline->copyShaderProgramInstance, nullptr, Meshes::Mesh3DRenderingContext::none(),
		vertices.get_data(), ::Meshes::Primitive::TriangleFan, 2, s_pipeline->vertexFormat);
	modelViewStack.pop();

	_destColourGridRT->unbind();
}
