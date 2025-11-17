#include "renderingPipeline.h"

#include "..\..\core\mainSettings.h"
#include "..\..\core\types\names.h"

using namespace Framework;

RenderingPipeline* RenderingPipeline::s_pipeline = nullptr;

void RenderingPipeline::initialise_static()
{
	an_assert(s_pipeline == nullptr);
	s_pipeline = new RenderingPipeline();
	s_pipeline->setup();
}

void RenderingPipeline::setup_static()
{
	an_assert(s_pipeline);
	s_pipeline->setup();
}

void RenderingPipeline::close_static()
{
	an_assert(s_pipeline);
	delete s_pipeline;
	s_pipeline = nullptr;
}

RenderingPipeline::RenderingPipeline()
: useDefault(true)
{
}

bool RenderingPipeline::load_setup_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	useDefault = false; // no longer allow "setup" to modify things

	clear();

	result &= version.load_from_xml_child_node(_node);

	result &= vertexShaderByUsage[::System::MaterialShaderUsage::Static].load_from_xml(_node->first_child_named(TXT("vertexShaderStatic")));
	result &= vertexShaderByUsage[::System::MaterialShaderUsage::Skinned].load_from_xml(_node->first_child_named(TXT("vertexShaderSkinned")));
	result &= vertexShaderByUsage[::System::MaterialShaderUsage::SkinnedToSingleBone].load_from_xml(_node->first_child_named(TXT("vertexShaderSkinnedToSingleBone")));
	result &= defaultVertexShaderByUsage[::System::MaterialShaderUsage::Static].load_from_xml(_node->first_child_named(TXT("defaultVertexShaderStatic")));
	result &= defaultVertexShaderByUsage[::System::MaterialShaderUsage::Skinned].load_from_xml(_node->first_child_named(TXT("defaultVertexShaderSkinned")));
	result &= defaultVertexShaderByUsage[::System::MaterialShaderUsage::SkinnedToSingleBone].load_from_xml(_node->first_child_named(TXT("defaultVertexShaderSkinnedToSingleBone")));

	result &= fragmentShader.load_from_xml(_node->first_child_named(TXT("fragmentShader")));
	result &= defaultfragmentShader.load_from_xml(_node->first_child_named(TXT("defaultFragmentShader")));
	result &= useWithRenderingFragmentShader.load_from_xml(_node->first_child_named(TXT("useWithRenderingFragmentShader")));

	defaultOutputDefinition.clear_output_textures();
	todo_note(TXT("load depth stencil from xml"));
	result &= defaultOutputDefinition.load_from_xml(_node->first_child_named(TXT("outputDefinition")), ::System::OutputTextureDefinition());

	return true;
}

void RenderingPipeline::clear()
{
	for_count(int, idx, ::System::MaterialShaderUsage::Num)
	{
		vertexShaderByUsage[idx].clear();
		defaultVertexShaderByUsage[idx].clear();
	}
	fragmentShader.clear();
	defaultfragmentShader.clear();
	useWithRenderingFragmentShader.clear();
}

static void add_fragment_shader_functions(REF_::System::ShaderSource & _fragmentShader)
{
	{
		_fragmentShader.add_function(Name(TXT("calculate_onto_observer")), String(TXT("float calculate_onto_observer(vec3 _position, vec3 _normal)")),
			  String(TXT("  vec3 normalisedPosition = normalize(_position);\n"))
			+ String(TXT("  return dot(normalisedPosition, -_normal);\n")));
	}

	{
		String body;
		if (MainSettings::global().rendering_pipeline_should_use_alpha_channel_for_depth())
		{
			body += String(TXT("  colour.a = _position.z;\n"));
		}
		if (MainSettings::global().rendering_pipeline_should_use_separate_buffers_for_position_and_normal() &&
			!MainSettings::global().rendering_pipeline_should_use_alpha_channel_for_depth())
		{
			body +=	String(TXT("  // store position and normal\n"))
				+	String(TXT("  position.rgb = _position.xyz;\n"))
				+	String(TXT("  position.a = 1.0;\n"))
				+	String(TXT("  normal.rgb = _normal.xyz;\n"))
				+	String(TXT("  normal.a = 1.0;\n"));
		}
		
		_fragmentShader.add_function(Name(TXT("fill_position_normal_output")), String(TXT("void fill_position_normal_output(vec3 _position, vec3 _normal)")), body);
	}
}

void RenderingPipeline::setup()
{
	if (!useDefault)
	{
		return;
	}

	an_assert(!MainSettings::global().rendering_pipeline_should_use_separate_buffers_for_position_and_normal() ||
		   !MainSettings::global().rendering_pipeline_should_use_alpha_channel_for_depth(), TXT("just one option at the time"));

	todo_future(TXT("get source for this from game? no extra processing"));

	version.setup_default();

	//

	clear();

	//

	vertexShaderByUsage[::System::MaterialShaderUsage::Static].data
		= String(TXT("\n"))
		+ String(TXT("// uniforms\n"))
		+ String(TXT("uniform mat3 normalMatrix;\n"))
		+ String(TXT("uniform mat4 modelViewMatrix;\n"))
		+ String(TXT("uniform mat4 projectionMatrix;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec3 inAPosition;\n"))
		+ String(TXT("in vec3 inONormal;\n"))
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("in vec3 inOColour;\n")) : String::empty())
		+ String(TXT("in vec2 inOUV;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// process variables\n"))
		+ String(TXT("vec4 prcPosition;\n"))
		+ String(TXT("vec4 prcNormal;\n"))
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("vec4 prcColour;\n")) : String::empty())
		+ String(TXT("vec2 prcUV;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec3 varNormal;\n"))
		+ String(TXT("centroid out vec3 varPosition;\n"))
		+ String(TXT("centroid out vec4 varProjectedPosition;\n"))
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("out vec4 varColour;\n")) : String::empty())
		+ String(TXT("out vec2 varUV;\n"))
		+ String(TXT("\n"));
	vertexShaderByUsage[::System::MaterialShaderUsage::Static].add_function(Name(TXT("process")), String(TXT("void process()")));
	vertexShaderByUsage[::System::MaterialShaderUsage::Static].mainBody
		+ String(TXT("  // ready data for processing\n"))
		+ String(TXT("  prcPosition.xyz = inAPosition;\n"))
		+ String(TXT("  prcPosition.w = 1.0;\n"))
		+ String(TXT("  prcNormal.xyz = inONormal;\n"))
		+ String(TXT("  prcNormal.w = 0.0;\n")) // no location affected
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("  prcColour.rgb = inOColour;\n")) : String::empty())
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("  prcColour.w = 1.0;\n")) : String::empty())
		+ String(TXT("  prcUV = inOUV;\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  // process\n"))
		+ String(TXT("  process();\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  // fill output\n"))
		+ String(TXT("  prcPosition = modelViewMatrix * prcPosition;\n"))
		+ String(TXT("  varPosition = prcPosition.xyz;\n"))
		+ String(TXT("  gl_Position = projectionMatrix * (prcPosition * scaleOutPosition);\n"))
		+ String(TXT("  varNormal = normalMatrix * normalize(prcNormal.xyz);\n"))
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("  varColour = prcColour;\n")) : String::empty())
		+ String(TXT("  varUV = prcUV;\n"));

	vertexShaderByUsage[::System::MaterialShaderUsage::Skinned].data
		= String(TXT("\n"))
		+ String(TXT("// uniforms\n"))
		+ String(TXT("uniform mat3 normalMatrix;\n"))
		+ String(TXT("uniform mat4 modelViewMatrix;\n"))
		+ String(TXT("uniform mat4 projectionMatrix;\n"))
		+ String(TXT("\n"))
		+ String(TXT("uniform mat4 inSkinningBones[200];\n"))
		+ String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec3 inAPosition;\n"))
		+ String(TXT("in vec3 inONormal;\n"))
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("in vec3 inOColour;\n")) : String::empty())
		+ String(TXT("in vec2 inOUV;\n"))
		//+ String(TXT("in int inSkinningNum;\n"))
		+ String(TXT("in ivec4 inOSkinningIndices;\n"))
		+ String(TXT("in vec4 inOSkinningWeights;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// process variables\n"))
		+ String(TXT("vec4 prcPosition;\n"))
		+ String(TXT("vec4 prcNormal;\n"))
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("vec4 prcColour;\n")) : String::empty())
		+ String(TXT("vec2 prcUV;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec3 varNormal;\n"))
		+ String(TXT("centroid out vec3 varPosition;\n"))
		+ String(TXT("centroid out vec4 varProjectedPosition;\n"))
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("out vec4 varColour;\n")) : String::empty())
		+ String(TXT("out vec2 varUV;\n"))
		+ String(TXT("\n"));
	vertexShaderByUsage[::System::MaterialShaderUsage::Skinned].add_function(Name(TXT("process")), String(TXT("void process()")));
	vertexShaderByUsage[::System::MaterialShaderUsage::Skinned].mainBody
		+ String(TXT("  // ready data for processing\n"))
		+ String(TXT("  prcPosition.xyz = inAPosition;\n"))
		+ String(TXT("  prcPosition.w = 1.0;\n"))
		+ String(TXT("  prcNormal.xyz = inONormal;\n"))
		+ String(TXT("  prcNormal.w = 0.0;\n")) // no location affected
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("  prcColour.rgb = inOColour;\n")) : String::empty())
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("  prcColour.w = 1.0;\n")) : String::empty())
		+ String(TXT("  prcUV = inOUV;\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  // process\n"))
		+ String(TXT("  process();\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  // do skinning (do it after process, as we may want to process values in model space?)\n"))
		+ String(TXT("  ivec4 curIndices = inOSkinningIndices;\n"))
		+ String(TXT("  vec4 curWeights = inOSkinningWeights;\n"))
		+ String(TXT("  prcPosition = curWeights.x * (inSkinningBones[curIndices.x] * prcPosition)\n"))
		+ String(TXT("              + curWeights.y * (inSkinningBones[curIndices.y] * prcPosition)\n"))
		+ String(TXT("              + curWeights.z * (inSkinningBones[curIndices.z] * prcPosition)\n"))
		+ String(TXT("              + curWeights.w * (inSkinningBones[curIndices.w] * prcPosition);\n"))
		+ String(TXT("  prcNormal = curWeights.x * (inSkinningBones[curIndices.x] * prcNormal)\n"))
		+ String(TXT("            + curWeights.y * (inSkinningBones[curIndices.y] * prcNormal)\n"))
		+ String(TXT("            + curWeights.z * (inSkinningBones[curIndices.z] * prcNormal)\n"))
		+ String(TXT("            + curWeights.w * (inSkinningBones[curIndices.w] * prcNormal);\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  // fill output\n"))
		+ String(TXT("  prcPosition = modelViewMatrix * prcPosition;\n"))
		+ String(TXT("  varPosition = prcPosition.xyz;\n"))
		+ String(TXT("  gl_Position = projectionMatrix * (prcPosition * scaleOutPosition);\n"))
		+ String(TXT("  varNormal = normalMatrix * normalize(prcNormal.xyz);\n"))
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("  varColour = prcColour;\n")) : String::empty())
		+ String(TXT("  varUV = prcUV;\n"));

	vertexShaderByUsage[::System::MaterialShaderUsage::SkinnedToSingleBone].data
		= String(TXT("\n"))
		+ String(TXT("// uniforms\n"))
		+ String(TXT("uniform mat3 normalMatrix;\n"))
		+ String(TXT("uniform mat4 modelViewMatrix;\n"))
		+ String(TXT("uniform mat4 projectionMatrix;\n"))
		+ String(TXT("\n"))
		+ String(TXT("uniform mat4 inSkinningBones[200];\n"))
		+ String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec3 inAPosition;\n"))
		+ String(TXT("in vec3 inONormal;\n"))
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("in vec3 inOColour;\n")) : String::empty())
		+ String(TXT("in vec2 inOUV;\n"))
		//+ String(TXT("in int inSkinningNum;\n"))
		+ String(TXT("in int inOSkinningIndex;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// process variables\n"))
		+ String(TXT("vec4 prcPosition;\n"))
		+ String(TXT("vec4 prcNormal;\n"))
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("vec4 prcColour;\n")) : String::empty())
		+ String(TXT("vec2 prcUV;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec3 varNormal;\n"))
		+ String(TXT("centroid out vec3 varPosition;\n"))
		+ String(TXT("centroid out vec4 varProjectedPosition;\n"))
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("out vec4 varColour;\n")) : String::empty())
		+ String(TXT("out vec2 varUV;\n"))
		+ String(TXT("\n"));
	vertexShaderByUsage[::System::MaterialShaderUsage::SkinnedToSingleBone].add_function(Name(TXT("process")), String(TXT("void process()")));
	vertexShaderByUsage[::System::MaterialShaderUsage::SkinnedToSingleBone].mainBody
		+ String(TXT("  // ready data for processing\n"))
		+ String(TXT("  prcPosition.xyz = inAPosition;\n"))
		+ String(TXT("  prcPosition.w = 1.0;\n"))
		+ String(TXT("  prcNormal.xyz = inONormal;\n"))
		+ String(TXT("  prcNormal.w = 0.0;\n")) // no location affected
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("  prcColour.rgb = inOColour;\n")) : String::empty())
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("  prcColour.w = 1.0;\n")) : String::empty())
		+ String(TXT("  prcUV = inOUV;\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  // process\n"))
		+ String(TXT("  process();\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  // do skinning (do it after process, as we may want to process values in model space?)\n"))
		+ String(TXT("  prcPosition = inSkinningBones[inOSkinningIndex] * prcPosition;\n"))
		+ String(TXT("  prcNormal = inSkinningBones[inOSkinningIndex] * prcNormal;\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  // fill output\n"))
		+ String(TXT("  prcPosition = modelViewMatrix * prcPosition;\n"))
		+ String(TXT("  varPosition = prcPosition.xyz;\n"))
		+ String(TXT("  gl_Position = projectionMatrix * (prcPosition * scaleOutPosition);\n"))
		+ String(TXT("  varNormal = normalMatrix * normalize(prcNormal.xyz);\n"))
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("  varColour = prcColour;\n")) : String::empty())
		+ String(TXT("  varUV = prcUV;\n"));

	//

	fragmentShader.data
		= String(TXT("\n"))
		+ String(TXT("// uniforms\n"))
		+ String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec3 varNormal;\n"))
		+ String(TXT("centroid in vec3 varPosition;\n"))
		+ String(TXT("centroid in vec4 varProjectedPosition;\n"))
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("in vec4 varColour;\n")) : String::empty())
		+ String(TXT("in vec2 varUV;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// process variables\n"))
		+ String(TXT("vec4 materialDiffuse;\n"))
		+ String(TXT("vec3 pointPosition;\n"))
		+ String(TXT("vec3 pointNormal;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec4 colour;\n"));
	if (MainSettings::global().rendering_pipeline_should_use_separate_buffers_for_position_and_normal() &&
		!MainSettings::global().rendering_pipeline_should_use_alpha_channel_for_depth())
	{
		fragmentShader.data
		   += String(TXT("out vec4 position;\n"))
			+ String(TXT("out vec4 normal;\n"));
	}
	add_fragment_shader_functions(REF_ fragmentShader);
	fragmentShader.mainBody
		= String(TXT("  // setup temporary variables\n"))
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("  materialDiffuse = varColour;\n")) : String(TXT("  materialDiffuse = vec4(1,1,1,1);\n")))
		+ String(TXT("  pointPosition = varPosition;\n"))
		+ String(TXT("  pointNormal = varNormal;\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  // process\n"))
		+ String(TXT("  process(); \n"))
		+ String(TXT("  \n"))
		+ String(TXT("  // calculate light\n"))
		+ String(TXT("  float light = calculate_onto_observer(pointPosition, pointNormal) * 0.3 + 0.7;\n"))
		+ String(TXT("  \n"))
		+ String(TXT("  // apply light\n"))
		+ String(TXT("	colour.rgb = materialDiffuse.rgb * light;\n"));
	if (! MainSettings::global().rendering_pipeline_should_use_alpha_channel_for_depth())
	{
		fragmentShader.mainBody
		   += String(TXT("  colour.a = materialDiffuse.a;\n"));
	}
	fragmentShader.mainBody
	   += String(TXT("  fill_position_normal_output(pointPosition, pointNormal);\n"));

	//

	useWithRenderingFragmentShader.data
		= String(TXT("\n"))
		+ String(TXT("// uniforms\n"))
		+ String(TXT("\n"))
		+ String(TXT("// input\n"))
		+ String(TXT("in vec3 varNormal;\n"))
		+ String(TXT("centroid in vec3 varPosition;\n"))
		+ String(TXT("centroid in vec4 varProjectedPosition;\n")) 
		+ (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes() ? String(TXT("in vec4 varColour;\n")) : String::empty())
		+ String(TXT("in vec2 varUV;\n"))
		+ String(TXT("\n"))
		+ String(TXT("// output\n"))
		+ String(TXT("out vec4 colour;\n"));
	if (MainSettings::global().rendering_pipeline_should_use_separate_buffers_for_position_and_normal() &&
		!MainSettings::global().rendering_pipeline_should_use_alpha_channel_for_depth())
	{
		useWithRenderingFragmentShader.data
		   += String(TXT("out vec4 position;\n"))
			+ String(TXT("out vec4 normal;\n"));
	}
	add_fragment_shader_functions(REF_ useWithRenderingFragmentShader);

	//

	defaultOutputDefinition.clear_output_textures();
	if (MainSettings::global().rendering_pipeline_should_use_alpha_channel_for_depth())
	{
		defaultOutputDefinition.add_output_texture(::System::OutputTextureDefinition(Names::colour,
			::System::VideoFormat::rgba16f,
			::System::BaseVideoFormat::rgba));
	}
	else
	{
		defaultOutputDefinition.add_output_texture(::System::OutputTextureDefinition(Names::colour,
			::System::VideoFormat::rgba8,
			::System::BaseVideoFormat::rgba));
		if (MainSettings::global().rendering_pipeline_should_use_separate_buffers_for_position_and_normal())
		{
			defaultOutputDefinition.add_output_texture(::System::OutputTextureDefinition(Names::position,
				::System::VideoFormat::rgb16f,
				::System::BaseVideoFormat::rgb));
			defaultOutputDefinition.add_output_texture(::System::OutputTextureDefinition(Names::normal,
				::System::VideoFormat::rgb16f,
				::System::BaseVideoFormat::rgb));
		}
	}
}
