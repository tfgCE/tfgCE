#include "shaderSource.h"

#include "..\..\mainSettings.h"
#include "..\..\math\math.h"
#include "..\..\io\xml.h"
#include "..\..\system\core.h"
#include "..\..\system\video\video3d.h"
#include "..\..\tags\tagCondition.h"
#include "..\..\utils\utils_loadingForSystemShader.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace ::System;

//

DEFINE_STATIC_NAME(makeItFaster);
DEFINE_STATIC_NAME(unifiedFog);
DEFINE_STATIC_NAME(specular);
DEFINE_STATIC_NAME(reflections);
DEFINE_STATIC_NAME(allowSkippableReflections);
DEFINE_STATIC_NAME(simpleReflections);
DEFINE_STATIC_NAME(simplifiedBackgroundForReflections);
DEFINE_STATIC_NAME(material);
DEFINE_STATIC_NAME(materialProp);
DEFINE_STATIC_NAME(voxel);
DEFINE_STATIC_NAME(simpleVoxel);
DEFINE_STATIC_NAME(voxelUneveness);
DEFINE_STATIC_NAME(light);
DEFINE_STATIC_NAME(fog);
DEFINE_STATIC_NAME(antiBanding);
DEFINE_STATIC_NAME(emissive);
DEFINE_STATIC_NAME(globalTint);
DEFINE_STATIC_NAME(globalDesaturate);
DEFINE_STATIC_NAME(vertexLight);
DEFINE_STATIC_NAME(vertexMaterial);
DEFINE_STATIC_NAME(vertexEmissive);
DEFINE_STATIC_NAME(vertexVoxel);
DEFINE_STATIC_NAME(alphaDithering);
DEFINE_STATIC_NAME(retro);
DEFINE_STATIC_NAME(autoTextures);
DEFINE_STATIC_NAME(coneLights);
DEFINE_STATIC_NAME(pointLights);
DEFINE_STATIC_NAME(stickPointLights);
DEFINE_STATIC_NAME(simpleConePointLights);
DEFINE_STATIC_NAME(brighterLightsInFog);

//

bool validate_shader_source_code(String const& _code, IO::XML::Node const* _node, tchar const * _what)
{
	if (_code.does_contain(String(TXT("__"))))
	{
		error_loading_xml(_node, TXT("avoid using double underscore in shader code (%S)"), _what);
		return false;
	}
	return true;
}

//

bool ShaderSourceFunction::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"));
	result &= name.is_valid();

	if (IO::XML::Node const * _child = _node->first_child_named(TXT("header")))
	{
		header = _child->get_text_raw();
		header.change_no_break_spaces_into_spaces();
		result &= validate_shader_source_code(header, _child, TXT("function header"));
	}

	if (IO::XML::Node const * _child = _node->first_child_named(TXT("body")))
	{
		body.push_back(_child->get_text_raw());
		body.get_last().change_no_break_spaces_into_spaces();
		result &= validate_shader_source_code(body.get_last(), _child, TXT("function body"));
	}
	else
	{
		body.push_back(String(TXT("// base")));
	}

	return result;
}

String ShaderSourceFunction::get_body() const
{
	String source;

	if (!body.is_empty())
	{
		String CALL_BASE(TXT("CALL_BASE"));
		source = body.get_last();
		int at = body.get_size() - 1;
		while (at > 0)
		{
			if (String::does_contain(source.to_char(), CALL_BASE.to_char()))
			{
				source = source.replace(CALL_BASE, body[at - 1] + TXT("\n"));
				--at;
			}
			else
			{
				break;
			}
		}
		if (String::does_contain(source.to_char(), CALL_BASE.to_char()))
		{
			ScopedOutputLock outputLock;
			output(TXT("function: %S"), name.to_char());
			output(TXT("body"));
			for_every_reverse(b, body)
			{
				output(TXT("[body %i]"), for_everys_index(b));
				output(TXT("%S"), b->to_char());
			}
			output(TXT("source:\n%S"), source.to_char());
			error_stop(TXT("source still contains CALL_BASE"));
		}
	}

	return source;
}

//

void ShaderSourceVersion::clear()
{
	gl = 0;
	gles = 0;
}

void ShaderSourceVersion::setup_default()
{
	todo_note(TXT("expose it somewhere? in a config file?"));
	//gl = 150;
	//gles = 300;
	gl = 400;
	gles = 320;
}

bool ShaderSourceVersion::load_from_xml_child_node(IO::XML::Node const* _node, tchar const * _childName)
{
	bool result = true;

	if (IO::XML::Node const* _child = _node->first_child_named(TXT("version")))
	{
		gl = _child->get_int_attribute_or_from_child(TXT("gl"), gl);
		gles = _child->get_int_attribute_or_from_child(TXT("gles"), gles);
	}

	return result;
}

//

ShaderSourceCustomisation* ShaderSourceCustomisation::s_one = nullptr;

void ShaderSourceCustomisation::initialise_static()
{
	an_assert(!s_one);
	s_one = new ShaderSourceCustomisation();
}

void ShaderSourceCustomisation::close_static()
{
	an_assert(s_one);
	delete_and_clear(s_one);
}

//

void ShaderSource::clear()
{
	version.clear();
	data = String::empty();
	functions.clear();
	mainBody = String::empty();
}

bool ShaderSource::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	if (!_node)
	{
		error(TXT("no source provided!"));
		an_assert(false);
		return false;
	}

	result &= version.load_from_xml_child_node(_node);

	if (IO::XML::Node const * _child = _node->first_child_named(TXT("data")))
	{
		data = _child->get_text_raw();
		data.change_no_break_spaces_into_spaces();
	}

	for_every(functionNode, _node->children_named(TXT("function")))
	{
		Name functionName = functionNode->get_name_attribute(TXT("name"));
		result &= validate_shader_source_code(functionName.to_string(), functionNode, TXT("function name"));
		if (functionName.is_valid())
		{
			ShaderSourceFunction& function = get_function(functionName);
			result &= function.load_from_xml(functionNode);
		}
		else
		{
			error_loading_xml(functionNode, TXT("function without name provided"));
			result = false;
		}
	}

	if (IO::XML::Node const * _child = _node->first_child_named(TXT("mainBody")))
	{
		mainBody = _child->get_text_raw();
		mainBody.change_no_break_spaces_into_spaces();
		result &= validate_shader_source_code(mainBody, _child, TXT("main body"));
	}

	result &= defaultValues.load_from_xml_child_node(_node, TXT("params"));
	result &= defaultValues.load_from_xml_child_node(_node, TXT("values"));
	result &= defaultValues.load_from_xml_child_node(_node, TXT("defaultParams"));
	result &= defaultValues.load_from_xml_child_node(_node, TXT("defaultValues")); // preferred

	for_every(forNode, _node->children_named(TXT("for")))
	{
		if (CoreUtils::Loading::should_load_for_system_or_shader_option(forNode))
		{
			result &= defaultValues.load_from_xml_child_node(forNode, TXT("params"));
			result &= defaultValues.load_from_xml_child_node(forNode, TXT("values"));
			result &= defaultValues.load_from_xml_child_node(forNode, TXT("defaultParams"));
			result &= defaultValues.load_from_xml_child_node(forNode, TXT("defaultValues")); // preferred
		}
	}

	return result;
}

#define SHADER_OPTION_REQUESTED(optionName) \
	::MainSettings::global().get_shader_options().get_tag_as_int(NAME(optionName))

#define USE_SHADER_OPTION(defineName) \
		source += String(TXT("#define ")) + defineName + TXT("\n"); \

#define SHADER_OPTION(optionName, defineName) \
	if (SHADER_OPTION_REQUESTED(optionName)) \
	{ \
		USE_SHADER_OPTION(defineName); \
	}

String ShaderSource::get_source(ShaderType::Type _type) const
{
	String source;

#ifdef AN_GL
	source += String::printf(TXT("#version %i\n"), version.gl);
#else
#ifdef AN_GLES
	source += String::printf(TXT("#version %i es\n"), version.gles);
#else
	todo_important(TXT("implement"));
#endif
#endif
	source += String(TXT("\n"));
	source += String(TXT("// extensions\n"));
#ifdef AN_GL
#else
#ifdef AN_GLES
	source += String(TXT("#extension GL_EXT_clip_cull_distance : enable\n"));
#endif
#endif
	source += String(TXT("\n"));
#ifdef AN_GL
#else
#ifdef AN_GLES
	source += String(TXT("#define GLES\n"));
	source += String(TXT("\n"));
#else
	todo_important(TXT("implement"));
#endif
#endif
	SHADER_OPTION(makeItFaster,							TXT("MAKE_IT_FASTER"));
	SHADER_OPTION(unifiedFog,							TXT("WITH_UNIFIED_FOG"));
	SHADER_OPTION(specular,								TXT("WITH_SPECULAR"));
	SHADER_OPTION(reflections,							TXT("WITH_REFLECTIONS"));
	SHADER_OPTION(allowSkippableReflections,			TXT("ALLOW_SKIPPABLE_REFLECTIONS"));
	SHADER_OPTION(simpleReflections,					TXT("SIMPLE_REFLECTIONS"));
	SHADER_OPTION(simplifiedBackgroundForReflections,	TXT("SIMPLIFIED_BACKGROUND_FOR_REFLECTIONS"));
	SHADER_OPTION(material,								TXT("WITH_MATERIAL"));
	SHADER_OPTION(materialProp,							TXT("WITH_MATERIAL_PROP"));
	SHADER_OPTION(voxel,								TXT("WITH_VOXEL"));
	SHADER_OPTION(simpleVoxel,							TXT("SIMPLE_VOXEL"));
	SHADER_OPTION(voxelUneveness,						TXT("VOXEL_UNEVENESS"));
	SHADER_OPTION(light,								TXT("WITH_LIGHT"));
	SHADER_OPTION(fog,									TXT("WITH_FOG"));
	SHADER_OPTION(antiBanding,							TXT("WITH_ANTI_BANDING"));
	SHADER_OPTION(emissive,								TXT("WITH_EMISSIVE"));
	SHADER_OPTION(globalTint,							TXT("WITH_GLOBAL_TINT"));
	SHADER_OPTION(globalDesaturate,						TXT("WITH_GLOBAL_DESATURATE"));
	SHADER_OPTION(vertexLight,							TXT("VERTEX_LIGHT"));
	SHADER_OPTION(vertexMaterial,						TXT("VERTEX_MATERIAL"));
	SHADER_OPTION(vertexEmissive,						TXT("VERTEX_EMISSIVE"));
	SHADER_OPTION(vertexVoxel,							TXT("VERTEX_VOXEL"));
	SHADER_OPTION(alphaDithering,						TXT("ALPHA_DITHERING"));
	SHADER_OPTION(retro,								TXT("RETRO"));
	SHADER_OPTION(autoTextures,							TXT("AUTO_TEXTURES"));
	SHADER_OPTION(coneLights,							TXT("CONE_LIGHTS"));
	SHADER_OPTION(pointLights,							TXT("POINT_LIGHTS"));
	SHADER_OPTION(stickPointLights,						TXT("STICK_POINT_LIGHTS"));
	SHADER_OPTION(simpleConePointLights,				TXT("SIMPLE_CONE_POINT_LIGHTS"));
	SHADER_OPTION(brighterLightsInFog,					TXT("BRIGHTER_LIGHTS_IN_FOG"));
	if (SHADER_OPTION_REQUESTED(stickPointLights))
	{
		if (!SHADER_OPTION_REQUESTED(pointLights))
		{
			error(TXT("stickPointLights require pointLights"));
		}
	}
	if (SHADER_OPTION_REQUESTED(coneLights) ||
		SHADER_OPTION_REQUESTED(pointLights))
	{
		USE_SHADER_OPTION(TXT("ANY_LIGHTS"));
	}

	if (::System::Video3D::get()->has_limited_support_for_indexing_in_shaders())
	{
		source += String(TXT("#define LIMITED_SUPPORT_FOR_INDEXING_IN_SHADERS\n"));
	}
	
	if (::System::Core::get_system_name() == TXT("pc"))
	{
		source += String(TXT("precision highp float;\n"));
		source += String(TXT("precision highp int;\n"));
		source += String(TXT("\n"));
		source += String(TXT("#define HIGHP highp\n"));
		source += String(TXT("#define MEDIUMP mediump\n"));
		source += String(TXT("#define LOWP lowp\n"));
	}
	else
	{
		//source += String(TXT("layout(early_fragment_tests) in;\n")); // <- this means that the depth buffer test will be early but will also always write to depth buffer, even if fragment gets discarded
		source += String(TXT("\n"));
		source += String(TXT("precision mediump float;\n"));
		source += String(TXT("precision mediump int;\n"));
		source += String(TXT("precision lowp sampler2D;\n"));
		source += String(TXT("\n"));
		source += String(TXT("#define HIGHP highp\n"));
		source += String(TXT("#define MEDIUMP mediump\n"));
		source += String(TXT("#define LOWP lowp\n"));
	}
	source += String(TXT("\n"));
	source += String::printf(TXT("#define CLIP_PLANE_COUNT %i\n"), PLANE_SET_PLANE_LIMIT);
	{
		source += String::printf(TXT("#define APPLY_CLIPPING"));
		for (int i = 0; i < PLANE_SET_PLANE_LIMIT; ++i)
		{
			source += String::printf(TXT(" gl_ClipDistance[%i] = dot(modelViewClipPlanes[%i], clipPosition);"), i, i);
		}
		source += String(TXT("\n"));
	}
	{
		source += String(TXT("#define UNIFORM_MATRIX_ARRAY(arrayName, arraySize) "));
		if (::System::Video3D::get()->should_use_vec4_arrays_instead_of_mat4_arrays())
		{
			source += String(TXT("uniform vec4 arrayName[arraySize * 4] "));
		}
		else
		{
			source += String(TXT("uniform mat4 arrayName[arraySize] "));
		}
		// no semicolon, should be added with source
		source += String(TXT("\n"));
	}
	{
		source += String(TXT("#define READ_FROM_UNIFORM_MATRIX_ARRAY(into, arrayName, arrayIndex) "));
		if (::System::Video3D::get()->should_use_vec4_arrays_instead_of_mat4_arrays())
		{
			source += String(TXT("{ "));
			source += String(TXT("  into[0].xyzw = arrayName[arrayIndex * 4 + 0]; "));
			source += String(TXT("  into[1].xyzw = arrayName[arrayIndex * 4 + 1]; "));
			source += String(TXT("  into[2].xyzw = arrayName[arrayIndex * 4 + 2]; "));
			source += String(TXT("  into[3].xyzw = arrayName[arrayIndex * 4 + 3]; "));
			source += String(TXT("} "));
		}
		else
		{
			source += String(TXT("into = arrayName[arrayIndex]; "));
		}
		source += String(TXT("\n"));
	}

	for_every(macro, ShaderSourceCustomisation::get()->macros)
	{
		source += *macro;
		source += String(TXT("\n"));
	}
	source += String(TXT("\n"));
	source += data;
	source += String(TXT("\n"));
	source += String(TXT("// built-in data\n"));
	for_every(singleData, ShaderSourceCustomisation::get()->data)
	{
		source += *singleData;
		source += String(TXT("\n"));
	}
	if (_type == ShaderType::Fragment)
	{
		for_every(singleData, ShaderSourceCustomisation::get()->dataFragmentShaderOnly)
		{
			source += *singleData;
			source += String(TXT("\n"));
		}
	}
	if (_type == ShaderType::Vertex)
	{
		for_every(singleData, ShaderSourceCustomisation::get()->dataVertexShaderOnly)
		{
			source += *singleData;
			source += String(TXT("\n"));
		}
	}
	source += String(TXT("\n"));
	source += String(TXT("// forward declarations\n"));
	for_every(function, functions)
	{
		if (!function->header.is_empty())
		{
			source += function->header;
			source += String(TXT(";\n"));
		}
	}
	source += String(TXT("\n"));
	source += String(TXT("// definitions\n"));
	for_every(function, ShaderSourceCustomisation::get()->functions)
	{
		source += *function;
		source += String(TXT("\n"));
	}
	if (_type == ShaderType::Fragment)
	{
		for_every(function, ShaderSourceCustomisation::get()->functionsFragmentShaderOnly)
		{
			source += *function;
			source += String(TXT("\n"));
		}
	}
	if (_type == ShaderType::Vertex)
	{
		for_every(function, ShaderSourceCustomisation::get()->functionsVertexShaderOnly)
		{
			source += *function;
			source += String(TXT("\n"));
		}
	}
	source += String(TXT("\n"));
	for_every(function, functions)
	{
		if (function->header.is_empty())
		{
			error(TXT("function \"%S\" does not have header"), function->name.to_char());
		}
		if (!function->header.is_empty())
		{
			source += function->header;
			source += String(TXT("\n{\n"));
			source += function->get_body();
			source += String(TXT("\n}\n"));
		}
		source += String(TXT("\n"));
	}
	source += String(TXT("void main()\n{\n"));
	source += mainBody;
	source += String(TXT("\n}\n"));
	return source;
}

ShaderSourceFunction& ShaderSource::get_function(Name const & _name, bool _addInFrontIfNotExisting)
{
	for_every(function, functions)
	{
		if (function->name == _name)
		{
			return *function;
		}
	}

	ShaderSourceFunction newFunction;
	newFunction.name = _name;
	if (_addInFrontIfNotExisting)
	{
		functions.push_front(newFunction);
		return functions.get_first();
	}
	else
	{
		functions.push_back(newFunction);
		return functions.get_last();
	}
}

ShaderSourceFunction* ShaderSource::get_existing_function(Name const & _name)
{
	for_every(function, functions)
	{
		if (function->name == _name)
		{
			return function;
		}
	}

	return nullptr;
}

void ShaderSource::add_function(Name const & _name, String const & _header, String const & _body)
{
	ShaderSourceFunction& function = get_function(_name);
	function.header = _header;
	function.body.push_back(_body);
}

bool ShaderSource::override_with(ShaderSource const & _extender)
{
	bool result = true;
	if (_extender.version.gl != 0)
	{
		version.gl = _extender.version.gl;
	}
	if (_extender.version.gles != 0)
	{
		version.gles = _extender.version.gles;
	}
	data += _extender.data;
	for_every_reverse(extenderFunction, _extender.functions)
	{
		// no early checks for CALL_BASE as it breaks with "dependsOn"
		ShaderSourceFunction& function = get_function(extenderFunction->name, true); // to fill it
		for_every(b, extenderFunction->body)
		{
			function.body.push_back(*b);
		}
		if (! extenderFunction->header.is_empty())
		{
			function.header = extenderFunction->header;
		}
	}
	if (! _extender.mainBody.is_empty())
	{
		mainBody = _extender.mainBody;
	}

	defaultValues.set_from(_extender.defaultValues);

	return result;
}
