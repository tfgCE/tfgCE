#include "shader.h"
#include "video3d.h"

#include "..\..\io\file.h"
#include "..\..\io\xml.h"

#include "..\..\types\string.h"
#include "..\..\containers\list.h"

#include "..\..\other\parserUtils.h"

#include "fragmentShaderOutputSetup.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace ::System;

String get_shader_log(ShaderID _id)
{
	assert_rendering_thread();
	String log;
	GLint infoLen = 0;
	DIRECT_GL_CALL_ glGetShaderiv(_id, GL_INFO_LOG_LENGTH, &infoLen); AN_GL_CHECK_FOR_ERROR

	if (infoLen > 1)
	{
		char8* infoLog = new char8[infoLen];
		DIRECT_GL_CALL_ glGetShaderInfoLog(_id, infoLen, NULL, infoLog); AN_GL_CHECK_FOR_ERROR
		log = String::from_char8(infoLog);
		delete[] infoLog;
	}
	return log;
}

ShaderID compile_shader(ShaderType::Type _type, String const & _source)
{
	assert_rendering_thread();
	DIRECT_GL_CALL_ ShaderID shader = glCreateShader(_type); AN_GL_CHECK_FOR_ERROR
	an_assert(shader != 0);
	Array<char8> sourceCh8 = _source.to_char8_array();
	char8 const * source = sourceCh8.get_data();
	DIRECT_GL_CALL_ glShaderSource(shader, 1, &source, NULL); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glCompileShader(shader); AN_GL_CHECK_FOR_ERROR
	GLint result;
	DIRECT_GL_CALL_ glGetShaderiv(shader, GL_COMPILE_STATUS, &result); AN_GL_CHECK_FOR_ERROR
	if (result == 0)
	{
		output_colour(1, 0, 0, 1);
		output(TXT("shader compilation error"));
		output(get_shader_log(shader).to_char());
		output_colour();
		Array<int> errorLines;
		Array<String> errorInfos;
		{
			List<String> tokens;
			get_shader_log(shader).split(String(TXT("\n")), tokens);
			for_every(token, tokens)
			{
#ifdef AN_GL
				int startsAt = token->find_first_of('(');
				int endsAt = token->find_first_of(')');
#else
#ifdef AN_GLES
				int startsAt = token->find_first_of(':', 8);
				int endsAt = token->find_first_of(':', 9);
#else
				int startsAt = NONE;
				int endsAt = NONE;
#endif
#endif
				if (startsAt != NONE && endsAt != NONE)
				{
					int line = ParserUtils::parse_int(token->get_sub(startsAt + 1, endsAt - startsAt - 1));
					errorLines.push_back(line);
					errorInfos.push_back(*token);
				}
			}
		}
		List<String> tokens;
		_source.split(String(TXT("\n")), tokens);
		int lineIndex = 1;
		for_every(token, tokens)
		{
			auto iError = errorLines.find(lineIndex);
			if (iError)
			{
				output_colour(1, 1, 1, 1);
			}
			output(TXT("%04i : %S"), lineIndex, token->to_char());
			if (iError)
			{
				output_colour(1, 0, 0, 1);
				output(TXT("     : %S"), errorInfos[(int)(iError - errorLines.begin())].to_char());
			}
			output_colour();
			++lineIndex;
		}
		error_stop(TXT("shader compilation error!"));
	}
	return shader;
}

//

ShaderSetup::ShaderSetup()
{
}

ShaderSetup::~ShaderSetup()
{
	delete_and_clear(fragmentOutputSetup);
}

FragmentShaderOutputSetup* ShaderSetup::get_fragment_output_setup()
{
	if (fragmentOutputSetup == nullptr)
	{
		fragmentOutputSetup = new FragmentShaderOutputSetup();
	}
	return fragmentOutputSetup;
}

bool ShaderSetup::load_from_xml(IO::XML::Node const * _node, OutputTextureDefinition const & _defaultOutputTextureDefinition)
{
	bool result = true;
	if (type == ShaderType::Fragment)
	{
		if (!fragmentOutputSetup)
		{
			fragmentOutputSetup = new FragmentShaderOutputSetup();
		}

		if (IO::XML::Node const * outputsNode = _node->first_child_named(TXT("outputs")))
		{
			result &= fragmentOutputSetup->load_from_xml(outputsNode, _defaultOutputTextureDefinition);
		}
	}
	return result;
}

void ShaderSetup::fill_version(ShaderSourceVersion const& _version)
{
	if (shaderSource.version.gl == 0)
	{
		shaderSource.version.gl = _version.gl;
	}
	if (shaderSource.version.gles == 0)
	{
		shaderSource.version.gles = _version.gles;
	}
}

bool ShaderSetup::fill_source_with_base(ShaderSource const & _base)
{
	ShaderSource copy = shaderSource;
	shaderSource = _base;
	// we reverse action: instead of filling holes in shaderSource with _base we are overriding what's in _base with what we have in shaderSource, but effect is the same
	// imagine shaderSource has A..DE.G and _base has abcdefg
	// if we override_ with things from shaderSource we will end up with AbcDEfG which same as we would be filling holes (.) with _base's things
	// oh, and it adds data!
	return shaderSource.override_with(copy);
}

//

Shader::Shader(ShaderSetup const * _setup)
: shader( 0 )
{
	an_assert(Video3D::get());
	Video3D::get()->add(this);

	init(*_setup);
}

Shader::Shader(ShaderSetup const& _setup)
	: shader(0)
{
	an_assert(Video3D::get());
	Video3D::get()->add(this);

	init(_setup);
}

Shader::~Shader()
{
	close();

	an_assert(Video3D::get());
	if (!Video3D::get())
	{
		return;
	}
	Video3D::get()->remove(this);
}

void Shader::init(ShaderSetup const & _setup)
{
	setup = &_setup;
	isSourceAssembled = false;
	isCompiled = false;
	type = _setup.type;
	defaultValues = _setup.shaderSource.defaultValues; // copy default values, if we use binary (ie. we're not assembling source) we still want them
	if (type == ShaderType::Fragment)
	{
		fragmentOutputSetup = new FragmentShaderOutputSetup();
		if (_setup.fragmentOutputSetup)
		{
			*fragmentOutputSetup = *_setup.fragmentOutputSetup;
		}
	}
}

String const& Shader::get_source()
{
	assemble_source();
	return source;
}

void Shader::assemble_source()
{
	if (isSourceAssembled)
	{
		return;
	}
	if (!setup.is_set())
	{
		error(TXT("setup not available!"));
		return;
	}
	auto& _setup = *setup.get();
	// load shaders
	if (!_setup.shaderFile.is_empty())
	{
		if (IO::File* shaderFile = new IO::File())
		{
			shaderFile->open(_setup.shaderFile);
			shaderFile->set_type(IO::InterfaceType::Text);
			// TODO file not read?
			source = String::empty();
			shaderFile->read_into_string(source);
			delete shaderFile;
		}
	}
	else if (!_setup.shaderString.is_empty())
	{
		source = _setup.shaderString;
	}
	else
	{
		source = _setup.shaderSource.get_source(type);
	}
	if (source.is_empty())
	{
		an_assert(false, TXT("no shader source"));
	}
	isSourceAssembled = true;
	setup.clear();
}

void Shader::make_sure_shader_is_compiled()
{
	if (!isCompiled)
	{
		shader = compile_shader(type, get_source());
		isCompiled = true;
	}
}

void Shader::close()
{
	delete_and_clear(fragmentOutputSetup);
	if (shader != 0)
	{
		assert_rendering_thread();
		DIRECT_GL_CALL_ glDeleteShader(shader); AN_GL_CHECK_FOR_ERROR
		shader = 0;
	}
}

