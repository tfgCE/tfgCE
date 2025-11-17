#include "displayPipeline.h"

#include "..\..\core\mainSettings.h"
#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\types\names.h"

using namespace Framework;

//

DEFINE_STATIC_NAME(inTexture);
DEFINE_STATIC_NAME(colourise);
DEFINE_STATIC_NAME(colouriseInk);
DEFINE_STATIC_NAME(colourisePaper);

//

DisplayPipeline* DisplayPipeline::s_pipeline = nullptr;

void DisplayPipeline::initialise_static()
{
	an_assert(s_pipeline == nullptr);
	s_pipeline = new DisplayPipeline();
	s_pipeline->setup();
}

void DisplayPipeline::setup_static()
{
	an_assert(s_pipeline);
	s_pipeline->setup();
}

void DisplayPipeline::close_static()
{
	an_assert(s_pipeline);
	delete_and_clear(s_pipeline);
}

DisplayPipeline::DisplayPipeline()
: useDefault(true)
{
}

DisplayPipeline::~DisplayPipeline()
{
	an_assert(!shaderProgram.is_set(), TXT("call release_shaders()?"));
	an_assert(!shaderProgramInstance, TXT("call release_shaders()?"));
}

bool DisplayPipeline::load_setup_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	useDefault = false; // no longer allow "setup" to modify things

	clear();

	result &= version.load_from_xml_child_node(_node);

	result &= vertexShader.load_from_xml(_node->first_child_named(TXT("vertexShader")));
	result &= defaultVertexShader.load_from_xml(_node->first_child_named(TXT("defaultVertexShader")));

	result &= fragmentShader.load_from_xml(_node->first_child_named(TXT("fragmentShader")));
	result &= defaultfragmentShader.load_from_xml(_node->first_child_named(TXT("defaultFragmentShader")));

	defaultOutputDefinition.clear_output_textures();
	result &= defaultOutputDefinition.load_from_xml(_node->first_child_named(TXT("outputDefinition")), ::System::OutputTextureDefinition());

	return true;
}

void DisplayPipeline::clear()
{
	vertexShader.clear();
	defaultVertexShader.clear();
	fragmentShader.clear();
	defaultfragmentShader.clear();
}

::System::ShaderProgramInstance* DisplayPipeline::get_setup_shader_program_instance(::System::TextureID _textureId, bool _colourise, Colour const & _colouriseInk, Colour const & _colourisePaper)
{
	s_pipeline->shaderProgramInstance->set_uniform(s_pipeline->shaderProgram_inTextureIdx, _textureId);
	s_pipeline->shaderProgramInstance->set_uniform(s_pipeline->shaderProgram_colouriseIdx, _colourise ? 1.0f : 0.0f);
	if (_colourise)
	{
		s_pipeline->shaderProgramInstance->set_uniform(s_pipeline->shaderProgram_colouriseInkIdx, _colouriseInk.to_vector4());
		s_pipeline->shaderProgramInstance->set_uniform(s_pipeline->shaderProgram_colourisePaperIdx, _colourisePaper.to_vector4());
	}
	return s_pipeline->shaderProgramInstance;
}

void DisplayPipeline::setup()
{
	if (!useDefault)
	{
		return;
	}

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
		+ String(TXT("// process variables\n"))
		+ String(TXT("vec4 prcPosition;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec2 varUV;\n"))
		+ String(TXT("out vec4 varColour;\n"))
		+ String(TXT("\n"));
	vertexShader.add_function(Name(TXT("process")), String(TXT("void process()")));
	vertexShader.mainBody
		= String(TXT("  // calculate position\n"))
		+ String(TXT("  prcPosition.xy = inAPosition;\n"))
		+ String(TXT("  prcPosition.z = 0.0;\n"))
		+ String(TXT("  prcPosition.w = 1.0;\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  prcPosition = modelViewMatrix * prcPosition;\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  process();\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  // fill output\n"))
		+ String(TXT("  gl_Position = projectionMatrix * prcPosition;\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  // just copy\n"))
		+ String(TXT("  varUV = inOUV;\n"))
		+ String(TXT("  varColour = inOColour;\n"));

	//

	fragmentShader.data
		= String(TXT("\n"))
		+ String(TXT("// uniforms\n"))
		+ String(TXT("uniform sampler2D inTexture;\n"))
		+ String(TXT("uniform float colourise;\n"))
		+ String(TXT("uniform vec4 colouriseInk;\n"))
		+ String(TXT("uniform vec4 colourisePaper;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec2 varUV;\n"))
		+ String(TXT("in vec4 varColour;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// process variables\n"))
		+ String(TXT("vec4 prcColour;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec4 colour;\n"));
	fragmentShader.defaultValues.access<float>(NAME(colourise)) = 0.0f;
	fragmentShader.defaultValues.access<Colour>(NAME(colouriseInk)) = Colour::white;
	fragmentShader.defaultValues.access<Colour>(NAME(colourisePaper)) = Colour::black;
	fragmentShader.add_function(Name(TXT("process")), String(TXT("void process()")));
	fragmentShader.mainBody
		= String(TXT("  prcColour = texture(inTexture, varUV).rgba;\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  {\n"))
		+ String(TXT("    vec4 paperToInk = colouriseInk - colourisePaper;\n"))
		+ String(TXT("    vec4 colourised = colourisePaper + paperToInk * (prcColour - vec4(0, 0, 0, 0));\n"))
		+ String(TXT("    vec4 newColour = colourise * colourised + (1.0 - colourise) * prcColour;\n"))
		+ String(TXT("    prcColour.rgb = newColour.rgb;\n"))
		+ String(TXT("    prcColour.a = newColour.a * prcColour.a; // to keep alpha\n"))
		+ String(TXT("  }\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  prcColour.rgba *= varColour.rgba;\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  process();\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  colour = prcColour;\n"))
		+ String(TXT("#ifdef MAKE_IT_FASTER\n"))
		+ String(TXT("  if (colour.a < 0.5)\n"))
		+ String(TXT("  {\n"))
		+ String(TXT("    discard;\n"))
		+ String(TXT("  }\n"))
		+ String(TXT("#endif\n"));

	//

	defaultOutputDefinition.clear_output_textures();
	defaultOutputDefinition.add_output_texture(::System::OutputTextureDefinition(Names::colour,
		::System::VideoFormat::rgb8,
		::System::BaseVideoFormat::rgba,
		false,
		::System::TextureFiltering::nearest,
		::System::TextureFiltering::nearest));

}

void DisplayPipeline::will_use_shaders()
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

void DisplayPipeline::release_shaders()
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

void DisplayPipeline::create_shaders()
{
	an_assert(s_pipeline);
	an_assert(! s_pipeline->shadersCreated);

	assert_rendering_thread();

	RefCountObjectPtr<::System::ShaderSetup> vertexShaderSetup(::System::ShaderSetup::from_source(::System::ShaderType::Vertex, get_vertex_shader_source()));
	RefCountObjectPtr<::System::ShaderSetup> fragmentShaderSetup(::System::ShaderSetup::from_source(::System::ShaderType::Fragment, get_fragment_shader_source()));

	vertexShaderSetup->fill_version(get_version());
	fragmentShaderSetup->fill_version(get_version());

	fragmentShaderSetup->get_fragment_output_setup()->copy_output_textures_from(get_default_output_definition());

	::System::Shader* vertexShader = new ::System::Shader(vertexShaderSetup.get());
	::System::Shader* fragmentShader = new ::System::Shader(fragmentShaderSetup.get());

	s_pipeline->shaderProgram = new ::System::ShaderProgram(vertexShader, fragmentShader);
	
	s_pipeline->shaderProgramInstance = new ::System::ShaderProgramInstance();
	
	s_pipeline->shaderProgramInstance->set_shader_program(s_pipeline->shaderProgram.get());
	
	if (s_pipeline->shaderProgram.is_set())
	{
		s_pipeline->shaderProgram_inTextureIdx = s_pipeline->shaderProgram->find_uniform_index(NAME(inTexture));
		s_pipeline->shaderProgram_colouriseIdx = s_pipeline->shaderProgram->find_uniform_index(NAME(colourise));
		s_pipeline->shaderProgram_colouriseInkIdx = s_pipeline->shaderProgram->find_uniform_index(NAME(colouriseInk));
		s_pipeline->shaderProgram_colourisePaperIdx = s_pipeline->shaderProgram->find_uniform_index(NAME(colourisePaper));
		
	}
	
	s_pipeline->shadersCreated = true;
}

void DisplayPipeline::destroy_shaders()
{
	an_assert(s_pipeline);
	an_assert(s_pipeline->shadersCreated);

	assert_rendering_thread();

	delete_and_clear(s_pipeline->shaderProgramInstance);

	s_pipeline->shaderProgram = nullptr;

	s_pipeline->shadersCreated = false;
}