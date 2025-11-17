#include "uiPipeline.h"

#include "..\..\core\mainSettings.h"
#include "..\..\core\types\names.h"

using namespace Framework;

UIPipeline* UIPipeline::s_pipeline = nullptr;

void UIPipeline::initialise_static()
{
	an_assert(s_pipeline == nullptr);
	s_pipeline = new UIPipeline();
	s_pipeline->setup();
}

void UIPipeline::setup_static()
{
	an_assert(s_pipeline);
	s_pipeline->setup();
}

void UIPipeline::close_static()
{
	an_assert(s_pipeline);
	delete_and_clear(s_pipeline);
}

UIPipeline::UIPipeline()
: useDefault(true)
{
}

bool UIPipeline::load_setup_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	useDefault = false; // no longer allow "setup" to modify things

	clear();

	result &= version.load_from_xml_child_node(_node);

	result &= vertexShader.load_from_xml(_node->first_child_named(TXT("vertexShader")));
	result &= defaultVertexShader.load_from_xml(_node->first_child_named(TXT("defaultVertexShader")));

	result &= fragmentShader.load_from_xml(_node->first_child_named(TXT("fragmentShader")));
	result &= defaultfragmentShader.load_from_xml(_node->first_child_named(TXT("defaultFragmentShader")));
	result &= useWithUIFragmentShader.load_from_xml(_node->first_child_named(TXT("useWithUIFragmentShader")));

	defaultOutputDefinition.clear_output_textures();
	todo_note(TXT("load depth stencil from xml"));
	result &= defaultOutputDefinition.load_from_xml(_node->first_child_named(TXT("outputDefinition")), ::System::OutputTextureDefinition());

	return true;
}

void UIPipeline::clear()
{
	vertexShader.clear();
	defaultVertexShader.clear();
	fragmentShader.clear();
	defaultfragmentShader.clear();
	useWithUIFragmentShader.clear();
}

void UIPipeline::setup()
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
		+ String(TXT("uniform mat4 projectionMatrix;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec3 inAPosition;\n"))
		+ String(TXT("in vec2 inOUV;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// process variables\n"))
		+ String(TXT("vec4 prcPosition;\n"))
		+ String(TXT("vec2 prcUV;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec3 varPosition;\n"))
		+ String(TXT("out vec2 varUV;\n"))
		+ String(TXT("\n"));
	vertexShader.add_function(Name(TXT("process")), String(TXT("void process()")));
	vertexShader.mainBody
		= String(TXT("  // ready data for processing\n"))
		+ String(TXT("  prcPosition.xyz = inAPosition;\n"))
		+ String(TXT("  prcPosition.w = 1.0;\n"))
		+ String(TXT("  prcUV = inOUV;\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  // process\n"))
		+ String(TXT("  process();\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  // fill output\n"))
		+ String(TXT("  varPosition = prcPosition.xyz;\n"))
		+ String(TXT("  gl_Position = projectionMatrix * prcPosition;\n"))
		+ String(TXT("  varUV = prcUV;\n"));

	//

	fragmentShader.data
		= String(TXT("\n"))
		+ String(TXT("// uniforms\n"))
		+ String(TXT("uniform sampler2D inTexture;\n"))
		+ String(TXT("uniform vec2 inTextureTexelSize;\n"))
		+ String(TXT("uniform vec2 inTextureSize;\n"))
		+ String(TXT("uniform vec2 inTextureAspectRatio;\n"))
		+ String(TXT("uniform vec2 inOutputTexelSize;\n"))
		+ String(TXT("uniform vec2 inOutputSize;\n"))
		+ String(TXT("uniform vec2 inOutputAspectRatio;\n"))
		+ String(TXT("uniform mat4 projectionMatrix3D;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec3 varPosition;\n"))
		+ String(TXT("in vec2 varUV;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// process variables\n"))
		+ String(TXT("vec4 materialDiffuse;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec4 colour;\n"))
		+ String(TXT("\n"));
	fragmentShader.add_function(Name(TXT("process")), String(TXT("void process()")));
	fragmentShader.mainBody
		= String(TXT("  materialDiffuse = texture(inTexture, varUV);\n"))
		+ String(TXT("  process(); \n"))
		+ String(TXT("  colour = materialDiffuse;\n"));

	//

	useWithUIFragmentShader.data
		= String(TXT("\n"))
		+ String(TXT("// uniforms\n"))
		+ String(TXT("uniform vec2 inTextureTexelSize;\n"))
		+ String(TXT("uniform vec2 inTextureSize;\n"))
		+ String(TXT("uniform vec2 inTextureAspectRatio;\n"))
		+ String(TXT("uniform vec2 inOutputTexelSize;\n"))
		+ String(TXT("uniform vec2 inOutputSize;\n"))
		+ String(TXT("uniform vec2 inOutputAspectRatio;\n"))
		+ String(TXT("uniform mat4 projectionMatrix3D;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec3 varPosition;\n"))
		+ String(TXT("in vec2 varUV;\n"))
		+ String(TXT("\n"));

	//

	defaultOutputDefinition.clear_output_textures();
	defaultOutputDefinition.add_output_texture(::System::OutputTextureDefinition(Names::colour,
		::System::VideoFormat::rgba8,
		::System::BaseVideoFormat::rgba));
}
