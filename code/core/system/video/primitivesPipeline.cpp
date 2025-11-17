#include "primitivesPipeline.h"

#include "..\..\mainSettings.h"
#include "..\..\types\names.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace System;

//

PrimitivesPipeline* PrimitivesPipeline::s_pipeline = nullptr;

void PrimitivesPipeline::initialise_static()
{
	an_assert(s_pipeline == nullptr);
	s_pipeline = new PrimitivesPipeline();
	s_pipeline->setup();
}

void PrimitivesPipeline::setup_static()
{
	an_assert(s_pipeline);
	s_pipeline->setup();
}

void PrimitivesPipeline::close_static()
{
	an_assert(s_pipeline);
	delete_and_clear(s_pipeline);
}

PrimitivesPipeline::PrimitivesPipeline()
: useDefault(true)
{
}

bool PrimitivesPipeline::load_setup_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	useDefault = false; // no longer allow "setup" to modify things

	clear();

	result &= version.load_from_xml_child_node(_node);

	result &= vertexShader2D.load_from_xml(_node->first_child_named(TXT("vertexShader2D")));
	result &= vertexShader3D.load_from_xml(_node->first_child_named(TXT("vertexShader3D")));
	result &= defaultVertexShader.load_from_xml(_node->first_child_named(TXT("defaultVertexShader")));

	result &= fragmentShaderBasic.load_from_xml(_node->first_child_named(TXT("fragmentShaderBasic")));
	result &= fragmentShaderWithTexture.load_from_xml(_node->first_child_named(TXT("fragmentShaderWithTexture")));
	result &= fragmentShaderWithoutTexture.load_from_xml(_node->first_child_named(TXT("fragmentShaderWithoutTexture")));
	result &= fragmentShaderWithTextureDiscardsBlending.load_from_xml(_node->first_child_named(TXT("fragmentShaderWithTextureDiscardsBlending")));
	result &= fragmentShaderWithoutTextureDiscardsBlending.load_from_xml(_node->first_child_named(TXT("fragmentShaderWithoutTextureDiscardsBlending")));
	result &= useWithPrimitivesFragmentShader.load_from_xml(_node->first_child_named(TXT("useWithPrimitivesFragmentShader")));

	defaultOutputDefinition.clear_output_textures();
	result &= defaultOutputDefinition.load_from_xml(_node->first_child_named(TXT("outputDefinition")), ::System::OutputTextureDefinition());

	return true;
}

void PrimitivesPipeline::clear()
{
	vertexShader2D.clear();
	vertexShader3D.clear();
	defaultVertexShader.clear();
	fragmentShaderBasic.clear();
	fragmentShaderWithTexture.clear();
	fragmentShaderWithoutTexture.clear();
	fragmentShaderWithTextureDiscardsBlending.clear();
	fragmentShaderWithoutTextureDiscardsBlending.clear();
	useWithPrimitivesFragmentShader.clear();
}

void PrimitivesPipeline::setup()
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

	vertexShader2D.data
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
	vertexShader2D.mainBody
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

	vertexShader3D.data
		= String(TXT("\n"))
		+ String(TXT("// uniforms\n"))
		+ String(TXT("uniform mat4 modelViewMatrix;\n"))
		+ String(TXT("uniform vec3 modelViewOffset;\n"))
		+ String(TXT("uniform mat4 projectionMatrix;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec3 inAPosition;\n"))
		+ String(TXT("in vec2 inOUV;\n"))
		+ String(TXT("in vec4 inOColour;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec2 varUV;\n"))
		+ String(TXT("out vec4 varColour;\n"));
	vertexShader3D.mainBody
		+ String(TXT("  // calculate position\n"))
		+ String(TXT("  vec4 prcPosition;\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  prcPosition.xyz = inAPosition;\n"))
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

	fragmentShaderBasic.data
		= String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec4 varColour;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec4 colour; \n"))
		+ String(TXT("\n"));
	fragmentShaderBasic.mainBody
		= String(TXT("  colour = varColour;\n"));

	//

	fragmentShaderWithTexture.data
		= String(TXT("\n"))
		+ String(TXT("// uniforms\n"))
		+ String(TXT("uniform sampler2D inTexture;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec2 varUV;\n"))
		+ String(TXT("in vec4 varColour;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec4 colour; \n"))
		+ String(TXT("\n"));
	fragmentShaderWithTexture.mainBody
		= String(TXT("  vec4 prcColour = texture(inTexture, varUV).rgba;\n"))
		+ String(TXT("  prcColour.rgba *= varColour.rgba;\n"))
		+ String(TXT("  colour = prcColour;\n"));

	//

	fragmentShaderWithoutTexture.data
		= String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec4 varColour;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec4 colour; \n"))
		+ String(TXT("\n"));
	fragmentShaderWithoutTexture.mainBody
		= String(TXT("  colour = varColour;\n"));

	//

	fragmentShaderWithTextureDiscardsBlending.data
		= String(TXT("\n"))
		+ String(TXT("// uniforms\n"))
		+ String(TXT("uniform sampler2D inTexture;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec2 varUV;\n"))
		+ String(TXT("in vec4 varColour;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec4 colour; \n"))
		+ String(TXT("\n"));
	fragmentShaderWithTextureDiscardsBlending.mainBody
		= String(TXT("  vec4 prcColour = texture(inTexture, varUV).rgba;\n"))
		+ String(TXT("  prcColour.rgba *= varColour.rgba;\n"))
		+ String(TXT("  colour = prcColour;\n"))
		+ String(TXT("  if (colour.a < 0.5) discard;\n"));

	//

	fragmentShaderWithoutTextureDiscardsBlending.data
		= String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec4 varColour;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec4 colour; \n"))
		+ String(TXT("\n"));
	fragmentShaderWithoutTextureDiscardsBlending.mainBody
		= String(TXT("  colour = varColour;\n"))
		+ String(TXT("  if (colour.a < 0.5) discard;\n"));

	//

	useWithPrimitivesFragmentShader.data
		= String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec4 varColour;\n"))
		+ String(TXT("\n"));

	//

	defaultOutputDefinition.clear_output_textures();
	defaultOutputDefinition.add_output_texture(::System::OutputTextureDefinition(Names::colour,
		::System::VideoFormat::rgba8,
		::System::BaseVideoFormat::rgba));
}
