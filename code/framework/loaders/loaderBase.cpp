#include "loaderBase.h"

#include "..\..\core\system\video\fragmentShaderOutputSetup.h"
#include "..\..\core\system\video\renderTarget.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Loader;

//

DEFINE_STATIC_NAME(colour);
DEFINE_STATIC_NAME(inData);

//

Base::Base()
{
	construct_rt();

	construct_fxaa_shaders();
}

Base::~Base()
{
	fxaaShaderProgram = nullptr;
	fxaaVertexShader = nullptr;
	fxaaFragmentShader = nullptr;
	rt = nullptr;
}

void Base::construct_rt()
{
	::System::RenderTargetSetup rtSetup = ::System::RenderTargetSetup(
		::System::Video3D::get()->get_screen_size(),
		MainConfig::global().get_video().get_msaa_samples());
	// we use ordinary rgb 8bit textures
	rtSetup.add_output_texture(::System::OutputTextureDefinition(NAME(colour),
		::System::VideoFormat::rgba8,
		::System::BaseVideoFormat::rgba));
	RENDER_TARGET_CREATE_INFO(TXT("loader base"));
	rt = new ::System::RenderTarget(rtSetup);
}

void Base::construct_fxaa_shaders()
{
	if (! MainConfig::global().get_video().does_use_msaa() || MainConfig::global().get_video().fxaa)
	{	// fxaa
		String vertexShaderSource
#ifdef AN_GL
			= String(TXT("#version 150\n"))
#endif
#ifdef AN_GLES
			= String(TXT("#version 300 es\n"))
#endif
			+ String(TXT("\n"))
			+ String(TXT("// uniforms\n"))
			+ String(TXT("uniform mat4 modelViewMatrix;\n"))
			+ String(TXT("uniform mat4 projectionMatrix;\n"))
			+ String(TXT("\n"))
			+ String(TXT("in vec2 inAPosition;\n"))
			+ String(TXT("in vec2 inOUV;\n"))
			+ String(TXT("\n"))
			+ String(TXT("// output\n"))
			+ String(TXT("out vec2 varUV;\n"))
			+ String(TXT("\n"))
			+ String(TXT("void main()\n"))
			+ String(TXT("{\n"))
			+ String(TXT("	varUV = inOUV;\n"))
			+ String(TXT("	vec4 prcPosition;\n"))
			+ String(TXT("	\n"))
			+ String(TXT("	prcPosition.xy = inAPosition;\n"))
			+ String(TXT("	prcPosition.z = 0.0;\n"))
			+ String(TXT("	prcPosition.w = 1.0;\n"))
			+ String(TXT("	\n"))
			+ String(TXT("	prcPosition = modelViewMatrix * prcPosition;\n"))
			+ String(TXT("	\n"))
			+ String(TXT("	// fill output\n"))
			+ String(TXT("	gl_Position = projectionMatrix * prcPosition;\n"))
			+ String(TXT("}\n"))
			;

		String fragmentShaderSource
#ifdef AN_GL
			= String(TXT("#version 150\n"))
#endif
#ifdef AN_GLES
			= String(TXT("#version 300 es\n"))
#endif
			+ String(TXT("\n"))
			+ String(TXT("#ifdef GL_ES\n"))
			+ String(TXT("precision mediump float;\n"))
			+ String(TXT("precision mediump int;\n"))
			+ String(TXT("#endif\n"))
			+ String(TXT("\n"))
			+ String(TXT("#define PROCESSING_TEXTURE_SHADER\n"))
			+ String(TXT("\n"))
			+ String(TXT("// uniform\n"))
			+ String(TXT("uniform sampler2D inData;\n"))
			+ String(TXT("uniform vec2 inOutputTexelSize;\n"))
			+ String(TXT("\n"))
			+ String(TXT("// input\n"))
			+ String(TXT("in vec2 varUV;\n"))
			+ String(TXT("\n"))
			+ String(TXT("// output\n"))
			+ String(TXT("out vec4 colour;\n"))
			+ String(TXT("\n"))
			+ String(TXT("void main()\n"))
			+ String(TXT("{\n"))
			+ String(TXT("    float FXAA_SPAN_MAX = 8.0;\n"))
			+ String(TXT("    float FXAA_REDUCE_MUL = 1.0/8.0;\n"))
			+ String(TXT("    float FXAA_REDUCE_MIN = (1.0/128.0);\n"))
			+ String(TXT("    \n"))
			+ String(TXT("    vec3 rgbNW = texture(inData, varUV + (vec2(-1.0, -1.0) * inOutputTexelSize)).rgb;\n"))
			+ String(TXT("    vec3 rgbNE = texture(inData, varUV + (vec2(+1.0, -1.0) * inOutputTexelSize)).rgb;\n"))
			+ String(TXT("    vec3 rgbSW = texture(inData, varUV + (vec2(-1.0, +1.0) * inOutputTexelSize)).rgb;\n"))
			+ String(TXT("    vec3 rgbSE = texture(inData, varUV + (vec2(+1.0, +1.0) * inOutputTexelSize)).rgb;\n"))
			+ String(TXT("    vec3 rgbM  = texture(inData, varUV).rgb;\n"))
			+ String(TXT("    \n"))
			+ String(TXT("    vec3 luma = vec3(0.299, 0.587, 0.114);\n"))
			+ String(TXT("    float lumaNW = dot(rgbNW, luma);\n"))
			+ String(TXT("    float lumaNE = dot(rgbNE, luma);\n"))
			+ String(TXT("    float lumaSW = dot(rgbSW, luma);\n"))
			+ String(TXT("    float lumaSE = dot(rgbSE, luma);\n"))
			+ String(TXT("    float lumaM  = dot(rgbM,  luma);\n"))
			+ String(TXT("    \n"))
			+ String(TXT("    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));\n"))
			+ String(TXT("    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));\n"))
			+ String(TXT("    \n"))
			+ String(TXT("    vec2 dir;\n"))
			+ String(TXT("    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));\n"))
			+ String(TXT("    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));\n"))
			+ String(TXT("    \n"))
			+ String(TXT("    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);\n"))
			+ String(TXT("    \n"))
			+ String(TXT("    float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);\n"))
			+ String(TXT("    \n"))
			+ String(TXT("    dir = min(vec2(FXAA_SPAN_MAX,  FXAA_SPAN_MAX), \n"))
			+ String(TXT("          max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin)) * inOutputTexelSize;\n"))
			+ String(TXT("    \n"))
			+ String(TXT("    vec3 rgbA = (1.0/2.0) * (\n"))
			+ String(TXT("                texture(inData, varUV + dir * (1.0/3.0 - 0.5)).rgb +\n"))
			+ String(TXT("                texture(inData, varUV + dir * (2.0/3.0 - 0.5)).rgb);\n"))
			+ String(TXT("    vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (\n"))
			+ String(TXT("                texture(inData, varUV + dir * (0.0/3.0 - 0.5)).rgb +\n"))
			+ String(TXT("                texture(inData, varUV + dir * (3.0/3.0 - 0.5)).rgb);\n"))
			+ String(TXT("    float lumaB = dot(rgbB, luma);\n"))
			+ String(TXT("    \n"))
			+ String(TXT("    if((lumaB < lumaMin) || (lumaB > lumaMax)){\n"))
			+ String(TXT("        colour.rgb = rgbA;\n"))
			+ String(TXT("    } else {\n"))
			+ String(TXT("        colour.rgb = rgbB;\n"))
			+ String(TXT("    }\n"))
			+ String(TXT("    colour.a = 1.0;\n"))
			+ String(TXT("}\n"));

		fxaaVertexShader = new System::Shader(System::ShaderSetup::from_string(System::ShaderType::Vertex, vertexShaderSource));
		fxaaFragmentShader = new System::Shader(System::ShaderSetup::from_string(System::ShaderType::Fragment, fragmentShaderSource));

		fxaaFragmentShader->access_fragment_shader_output_setup().allow_default_output();

		if (fxaaVertexShader.get() && fxaaFragmentShader.get())
		{
			fxaaShaderProgram = new System::ShaderProgram(fxaaVertexShader.get(), fxaaFragmentShader.get());
		}
	}
}

void Base::render_rt(::System::Video3D* _v3d)
{
	_v3d->set_default_viewport();
	_v3d->set_near_far_plane(0.02f, 100.0f);
	_v3d->setup_for_2d_display();
	_v3d->set_2d_projection_matrix_ortho();

	_v3d->access_model_view_matrix_stack().clear();
	_v3d->access_model_view_matrix_stack().force_requires_send_all();

	if (fxaaShaderProgram.get())
	{
		fxaaShaderProgram->bind();
		fxaaShaderProgram->set_build_in_uniform_in_texture_size_related_uniforms(rt->get_size().to_vector2(), ::System::Video3D::get()->get_screen_size().to_vector2());
		fxaaShaderProgram->set_uniform(NAME(inData), rt->get_data_texture_id(0));
		rt->render_for_shader_program(_v3d, Vector2::zero, rt->get_size().to_vector2(), NP, fxaaShaderProgram.get());
		fxaaShaderProgram->unbind();
	}
	else
	{
		rt->render(0, _v3d, Vector2::zero, rt->get_size().to_vector2());
	}
}

void Base::update_vr_centre(Vector3 const& _eyeLoc)
{
	Vector3 eyeFlat = _eyeLoc * Vector3::xy;
	if (!vrCentre.is_set())
	{
		vrCentre = eyeFlat;
	}
	else
	{
		float dist = (vrCentre.get() - eyeFlat).length();
		float const maxDist = 0.5f;
		if (dist > maxDist)
		{
			vrCentre = eyeFlat + (vrCentre.get() - eyeFlat).normal() * maxDist;
		}
	}
}
